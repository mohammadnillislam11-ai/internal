#include "engine.hpp"
#include "memory.hpp"
#include "game_data.hpp"
#include "esp.hpp"
#include "aimbot.hpp"
#include "game_security.hpp"
#include "offsets.hpp"
#include "menu/config.hpp"
#include "menu/menu.hpp"
#include "libraries/storage/logger.h"
#include "oxorany.h"
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <ctime>

namespace engine {

    static bool g_attached = false;

    /** @return Name of the target game package when detected & attached, or nullptr. */
    const char* get_target_package() {
        if (g_attached && mem::g_pid > 0 && g_detected_package[0] != '\0') {
            return g_detected_package;
        }
        return nullptr;
    }

    /** @return Region string identifier. */
    const char* get_target_region() {
        return TARGET_REGION;
    }

    /** @return Currently targeted game process ID, or -1 if unattached. */
    int get_target_pid() {
        if (g_attached && mem::g_pid > 0 && kill(mem::g_pid, 0) == 0) {
            return mem::g_pid;
        }
        return -1;
    }

    bool is_attached() {
        return g_attached && mem::g_pid > 0 && mem::g_lib_base > 0;
    }

    /**
     * @brief Detects and attaches strictly to the Normal Free Fire process (com.dts.freefireth).
     *        Operates 100% in-memory via direct kernel assembly syscalls.
     * @return true if successfully attached, false if game is not running or still launching.
     */
    bool init() {
        static bool s_first_run = true;
        if (s_first_run) {
            s_first_run = false;
            logger::log_info("[ENGINE] Memory subsystem initialized. Targeting: %s", TARGET_PACKAGE);
            logger::log_info("[MEM] Syscall driver: Direct Kernel Assembly (SVC #0)");
        }

        if (g_attached && mem::g_pid > 0 && kill(mem::g_pid, 0) == 0 && mem::g_lib_base > 0) {
            return true;
        }

        if (mem::g_mem_fd >= 0) {
            close(mem::g_mem_fd);
            mem::g_mem_fd = -1;
        }
        g_attached = false;
        mem::g_pid = -1;
        mem::g_lib_base = 0;
        g_detected_package[0] = '\0';
        proc::pid = -1;
        proc::lib = 0;

        int found_pid = mem::find_pid_by_name(TARGET_PACKAGE);
        if (found_pid <= 0 || found_pid == getpid()) {
            static double s_last_fail_log = 0.0;
            double now = (double)time(nullptr);
            if (now - s_last_fail_log >= 8.0) {
                s_last_fail_log = now;
                logger::log_warn("[SCAN] Target '%s' not detected. Standby...", TARGET_PACKAGE);
            }
            return false;
        }

        logger::log_info("[+] Process found: %s (PID: %d)", TARGET_PACKAGE, found_pid);

        // Locate libil2cpp.so module base
        uintptr_t lib_base = mem::find_module_base(found_pid, oxorany("libil2cpp.so"));
        if (lib_base == 0) {
            logger::log_warn("[*] Process %d active, awaiting libil2cpp.so load...", found_pid);
            return false;
        }

        // Both process and module base are verified
        mem::g_pid = found_pid;
        mem::g_lib_base = lib_base;
        snprintf(g_detected_package, sizeof(g_detected_package), "%s", TARGET_PACKAGE);

        // Initialize passive security verification
        security::init(mem::g_pid);

        g_attached = true;
        proc::pid = mem::g_pid;
        proc::lib = (uintptr_t)mem::g_lib_base;
        data::start_worker();
        aim::init();

        logger::log_success("[+] ATTACHED: %s", g_detected_package);
        logger::log_success("[+] PID: %d | Base: 0x%lx", mem::g_pid, (unsigned long)mem::g_lib_base);
        logger::log_success("[+] Direct Kernel SVC #0 active. Telemetry linked.");

        return true;
    }

    /**
     * @brief Periodic engine heartbeat. Runs re-attach watchdog if game restarts,
     *        and updates aimbot calculations when enabled by user.
     */
    void update() {
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

        // Aimbot update and state restoration safety
        aim::update();

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
        if (g_attached) {
            logger::log_warn("[!] Target process stopped or exited. Detaching engine.");
        }
        aim::restore_state();
        data::stop_worker();
        g_attached = false;
        mem::g_pid = -1;
        mem::g_lib_base = 0;
        g_detected_package[0] = '\0';
        if (mem::g_mem_fd >= 0) {
            close(mem::g_mem_fd);
            mem::g_mem_fd = -1;
        }
        proc::pid = -1;
        proc::lib = 0;
    }

} // namespace engine
