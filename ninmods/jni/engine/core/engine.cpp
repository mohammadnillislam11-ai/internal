#include "engine.hpp"
#include "memory.hpp"
#include "game_data.hpp"
#include "esp.hpp"
#include "aimbot.hpp"
#include "game_security.hpp"
#include "offsets.hpp"
#include "menu/config.hpp"
#include "menu/menu.hpp"
#include "engine/bridge/shared_bridge.hpp"
#include "oxorany.h"
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <algorithm>

namespace engine {

    static bool g_attached = false;
    static int s_bridge_fd = -1;
    static bridge::BridgeData* s_bridge_ptr = nullptr;

    /** @return Name of the target game package. */
    const char* get_target_package() {
        return g_active_package;
    }

    /** @return Region string identifier. */
    const char* get_target_region() {
        return TARGET_REGION;
    }

    /** @return Currently targeted game process ID, or -1 if unattached. */
    int get_target_pid() {
        return mem::g_pid;
    }

    bool is_attached() {
        return g_attached && mem::g_pid > 0 && mem::g_lib_base > 0;
    }

    /**
     * @brief Synchronizes state from background root memory daemon via shared memory bridge.
     * @return true if bridge is active and fresh, false if daemon is offline.
     */
    static bool sync_from_daemon_bridge() {
        if (!s_bridge_ptr) {
            s_bridge_fd = open(bridge::BRIDGE_PATH, O_RDWR);
            if (s_bridge_fd < 0) return false;

            void* m = mmap(nullptr, sizeof(bridge::BridgeData), PROT_READ | PROT_WRITE, MAP_SHARED, s_bridge_fd, 0);
            if (m == MAP_FAILED) {
                close(s_bridge_fd);
                s_bridge_fd = -1;
                return false;
            }
            s_bridge_ptr = static_cast<bridge::BridgeData*>(m);
        }

        if (s_bridge_ptr->magic != bridge::BRIDGE_MAGIC) {
            return false;
        }

        uint64_t cur_ms = bridge::now_ms();
        if (cur_ms < s_bridge_ptr->heartbeat_ms || (cur_ms - s_bridge_ptr->heartbeat_ms) > 3500) {
            return false;
        }

        // Bridge is active and fresh: sync telemetry to UI
        mem::g_pid = s_bridge_ptr->pid;
        mem::g_lib_base = s_bridge_ptr->lib_base;
        proc::pid = s_bridge_ptr->pid;
        proc::lib = s_bridge_ptr->lib_base;
        data::g_in_match.store(s_bridge_ptr->in_match);
        data::g_local_player.store(s_bridge_ptr->local_player);
        data::g_view_matrix = s_bridge_ptr->view_matrix;

        // Synchronize players for ESP rendering
        {
            std::lock_guard<std::mutex> lk(data::g_players_mutex);
            size_t cnt = std::min(static_cast<size_t>(s_bridge_ptr->player_count), bridge::MAX_BRIDGE_PLAYERS);
            data::g_players.resize(cnt);
            for (size_t i = 0; i < cnt; i++) {
                data::g_players[i] = s_bridge_ptr->players[i];
            }
        }

        // Send UI options to daemon
        s_bridge_ptr->ui_cfg.aim_enabled = cfg::aim::enabled;
        s_bridge_ptr->ui_cfg.target_bone = cfg::aim::target_bone;
        s_bridge_ptr->ui_cfg.aim_fov = cfg::aim::fov_radius;
        s_bridge_ptr->ui_cfg.aim_dist = cfg::aim::distance;
        s_bridge_ptr->ui_cfg.aim_smooth = cfg::aim::smooth;
        s_bridge_ptr->ui_cfg.aim_silent = cfg::aim::aim_silent;
        s_bridge_ptr->ui_cfg.aim_nobita = cfg::aim::nobita_mode;
        s_bridge_ptr->ui_cfg.speed_hack = cfg::misc::speed_hack;
        s_bridge_ptr->ui_cfg.fast_switch = cfg::misc::fast_switch;
        s_bridge_ptr->ui_cfg.max_range = cfg::esp::max_range;

        g_attached = (mem::g_pid > 0 && mem::g_lib_base > 0);
        return true;
    }

    /**
     * @brief Detects and attaches strictly to the running Free Fire process (com.dts.freefireth).
     *        Safely verifies libil2cpp.so module base before starting worker threads.
     * @return true if successfully attached, false if game is not running or still launching.
     */
    bool init() {
        if (g_attached && mem::g_pid > 0 && kill(mem::g_pid, 0) == 0 && mem::g_lib_base > 0) {
            return true;
        }

        if (mem::g_mem_fd >= 0) {
            close(mem::g_mem_fd);
            mem::g_mem_fd = -1;
        }

        static const char* packages[] = { TARGET_PACKAGE, TARGET_PACKAGE_MAX };
        int found_pid = -1;
        const char* matched_package = TARGET_PACKAGE;
        for (const char* pkg : packages) {
            found_pid = mem::find_pid_by_name(pkg);
            if (found_pid > 0) {
                matched_package = pkg;
                break;
            }
        }
        if (found_pid <= 0) {
            return false;
        }

        g_active_package = matched_package;
        mem::g_pid = found_pid;

        // Locate libil2cpp.so module base
        mem::g_lib_base = mem::find_module_base(mem::g_pid, oxorany("libil2cpp.so"));
        if (mem::g_lib_base == 0) {
            // Process exists but libil2cpp.so is still initializing (splash screen)
            return false;
        }

        // Initialize passive security verification
        security::init(mem::g_pid);

        g_attached = true;
        proc::pid = mem::g_pid;
        proc::lib = (uintptr_t)mem::g_lib_base;
        data::start_worker();
        aim::init();

        printf("\033[92m[*] Attached: \033[93mcom.gemini.cli\033[0m (PID: \033[92m%d\033[0m)\033[0m\n", mem::g_pid);
        return true;
    }

    /**
     * @brief Periodic engine heartbeat. Prioritizes shared memory daemon bridge,
     *        falling back to local memory engine if daemon is not running.
     */
    void update() {
        if (sync_from_daemon_bridge()) {
            return;
        }

        if (!g_attached || mem::g_pid <= 0) {
            static int check_counter = 0;
            if (++check_counter % 60 == 0) {
                init();
            }
            return;
        }

        // Verify game PID is still alive
        if (kill(mem::g_pid, 0) != 0) {
            shutdown();
            return;
        }

        if (!security::is_active()) {
            security::init(mem::g_pid);
        }

        if (cfg::aim::enabled) {
            aim::update();
        }

        uint64_t local = data::g_local_player.load(std::memory_order_relaxed);
        if (mem::is_valid_addr(local) && data::g_in_match.load(std::memory_order_relaxed)) {
            if (!mem::read<bool>(local + offsets::Player_IsDead)) {
                if (cfg::misc::speed_hack) {
                    Vector3 vel = mem::read<Vector3>(local + offsets::Player_Velocity);
                    if (std::isfinite(vel.x) && std::isfinite(vel.z) && (vel.x != 0.0f || vel.z != 0.0f)) {
                        vel.x *= 1.35f;
                        vel.z *= 1.35f;
                        mem::write<Vector3>(local + offsets::Player_Velocity, vel);
                    }
                }

                if (cfg::misc::fast_switch) {
                    uint64_t weapon = mem::read<uint64_t>(local + offsets::Player_Weapon);
                    if (mem::is_valid_addr(weapon)) {
                        mem::write<float>(weapon + offsets::Weapon_Recoil, 0.0f);
                    }
                }
            }
        }
    }

    /** @brief Dispatches ESP and FOV rendering to active drawlists when enabled. */
    void render_esp() {
        esp::render_all();
        if (cfg::aim::draw_fov) {
            aim::render_fov();
        }
    }

    /** @brief Safely shuts down worker thread and resets all memory handles. */
    void shutdown() {
        if (s_bridge_ptr) {
            munmap(s_bridge_ptr, sizeof(bridge::BridgeData));
            s_bridge_ptr = nullptr;
        }
        if (s_bridge_fd >= 0) {
            close(s_bridge_fd);
            s_bridge_fd = -1;
        }

        data::stop_worker();
        g_attached = false;
        mem::g_pid = -1;
        mem::g_lib_base = 0;
        if (mem::g_mem_fd >= 0) {
            close(mem::g_mem_fd);
            mem::g_mem_fd = -1;
        }
        proc::pid = -1;
        proc::lib = 0;
    }

} // namespace engine
