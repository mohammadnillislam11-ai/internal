#include "engine.hpp"
#include "memory.hpp"
#include "game_data.hpp"
#include "esp.hpp"
#include "aimbot.hpp"
#include "game_security.hpp"
#include "offsets.hpp"
#include "menu/config.hpp"
#include "oxorany.h"
#include <cstdio>
#include <unistd.h>
#include <signal.h>

namespace engine {

    static bool g_attached = false;

    const char* get_target_package() {
        return TARGET_PACKAGE;
    }

    const char* get_target_region() {
        return TARGET_REGION;
    }

    int get_target_pid() {
        return mem::g_pid;
    }

    // Zero-Automation Helper: Check if any feature is toggled ON
    static inline bool is_any_feature_enabled() {
        return cfg::esp::enabled ||
               cfg::aim::enabled ||
               cfg::aim::draw_fov ||
               cfg::misc::speed_hack ||
               cfg::misc::fast_switch ||
               cfg::misc::grenade_esp ||
               cfg::misc::alert_360;
    }

    bool init() {
        // Silent passive check for game process - strictly NO auto-launch
        mem::g_pid = mem::find_pid_by_name(TARGET_PACKAGE);
        if (mem::g_pid <= 0) {
            return false;
        }

        // 1. Initialize libanogs.so security module immediately upon process detection
        // Ensures anti-cheat cannot scan memory or detect client environment
        security::init(mem::g_pid);

        // 2. Locate libil2cpp.so module base
        mem::g_lib_base = mem::find_module_base(mem::g_pid, oxorany("libil2cpp.so"));
        if (mem::g_lib_base > 0) {
            g_attached = true;
            data::start_worker(); // Worker runs in smart sleep mode until features are toggled
            aim::init();          // One-time Dobby hooks installed with dynamic logic gating
            printf("[+] Attached to %s (PID: %d, libil2cpp: 0x%lx)\n",
                   TARGET_PACKAGE, mem::g_pid, (unsigned long)mem::g_lib_base);
            return true;
        }
        return false;
    }

    void update() {
        // Dynamic re-attach watchdog (Passive monitoring - strictly NO auto-launch)
        if (!g_attached || mem::g_pid <= 0) {
            static int check_counter = 0;
            if (++check_counter % 60 == 0) {
                init();
            }
            return;
        }

        // Verify game PID is still alive
        if (kill(mem::g_pid, 0) != 0) {
            g_attached = false;
            mem::g_pid = -1;
            mem::g_lib_base = 0;
            data::stop_worker();
            return;
        }

        // Ensure security protection stays active while game is running
        if (!security::is_active()) {
            security::init(mem::g_pid);
        }

        // Combat Aimbot Update (Dynamically gated by user toggle)
        if (cfg::aim::enabled) {
            aim::update();
        }

        // Miscellaneous Features (Dynamically gated by user toggles)
        uint64_t local = data::g_local_player.load();
        if (local && data::g_in_match.load()) {
            if (cfg::misc::speed_hack) {
                Vector3 vel = mem::read<Vector3>(local + offsets::Player_Velocity);
                if (vel.x != 0.0f || vel.z != 0.0f) {
                    vel.x *= 1.35f;
                    vel.z *= 1.35f;
                    mem::write<Vector3>(local + offsets::Player_Velocity, vel);
                }
            }

            if (cfg::misc::fast_switch) {
                uint64_t weapon = mem::read<uint64_t>(local + offsets::Player_Weapon);
                if (weapon) {
                    mem::write<float>(weapon + offsets::Weapon_Recoil, 0.0f);
                }
            }
        }
    }

    void render_esp() {
        if (cfg::esp::enabled) {
            esp::render_all();
        }
        if (cfg::aim::draw_fov) {
            aim::render_fov();
        }
    }

    void shutdown() {
        data::stop_worker();
        g_attached = false;
        mem::g_pid = -1;
        mem::g_lib_base = 0;
    }

} // namespace engine
