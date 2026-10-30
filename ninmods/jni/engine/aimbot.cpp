#define IMGUI_DEFINE_MATH_OPERATORS
#include "aimbot.hpp"
#include "game_data.hpp"
#include "offsets.hpp"
#include "memory.hpp"
#include "menu/config.hpp"
#include "display_metrics.h"
#include "imgui.h"
#include "dobby.h"
#include "glm.hpp"
#include <cmath>
#include <android/log.h>

#define LOG_TAG "NINMODS_AIM"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace engine::aim {

    static uint64_t g_current_target = 0;
    static bool g_hook_installed = false;

    // ─────────────────────────────────────────────────────────────
    // TakeDamage Hook (9-param) - Silent Aim & Nobita Headshot Redirect
    // ─────────────────────────────────────────────────────────────
    static int (*orig_OnCalcDamage_9)(void* player, int damage, unsigned int damagerID,
                                      void* damageInfo, int hitGroup,
                                      Vector3 hitPos, Vector3 hitNormal,
                                      void* checkParams, void* weaponInfo, unsigned int vehicleID) = nullptr;

    static int hook_OnCalcDamage_9(void* player, int damage, unsigned int damagerID,
                                   void* damageInfo, int hitGroup,
                                   Vector3 hitPos, Vector3 hitNormal,
                                   void* checkParams, void* weaponInfo, unsigned int vehicleID) {
        if ((cfg::aim::silent || cfg::aim::nobita) && damageInfo != nullptr && g_current_target != 0) {
            uint64_t head_node = mem::read<uint64_t>(g_current_target + offsets::Bone_Head);
            if (head_node) {
                Vector3 enemy_head_world = data::get_node_position(head_node);
                if (enemy_head_world != Vector3::Zero()) {
                    Vector3 my_pos = Vector3::Zero();
                    uint64_t local = data::g_local_player.load(std::memory_order_relaxed);
                    if (local) {
                        uint64_t my_head_node = mem::read<uint64_t>(local + offsets::Bone_Head);
                        if (my_head_node) my_pos = data::get_node_position(my_head_node);
                    }

                    Vector3 direction = hitNormal;
                    if (my_pos != Vector3::Zero()) {
                        glm::vec3 p1(my_pos.x, my_pos.y, my_pos.z);
                        glm::vec3 p2(enemy_head_world.x, enemy_head_world.y, enemy_head_world.z);
                        glm::vec3 diff = p2 - p1;
                        if (glm::length(diff) > 0.001f) {
                            diff = glm::normalize(diff);
                            direction = Vector3(diff.x, diff.y, diff.z);
                        }
                    }

                    *(short*)((uintptr_t)damageInfo + offsets::DmgInfo_HitGroup) = 1; // Headshot
                    *(Vector3*)((uintptr_t)damageInfo + offsets::DmgInfo_HitLocation) = enemy_head_world;
                    *(Vector3*)((uintptr_t)damageInfo + offsets::DmgInfo_HitNormal) = direction;
                    *(Vector3*)((uintptr_t)damageInfo + offsets::DmgInfo_RayDir) = direction;

                    hitGroup = 1;
                    hitPos = enemy_head_world;
                    hitNormal = direction;
                }
            }
        }

        if (orig_OnCalcDamage_9) {
            return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                      hitGroup, hitPos, hitNormal,
                                      checkParams, weaponInfo, vehicleID);
        }
        return 0;
    }

    // ─────────────────────────────────────────────────────────────
    // TakeDamage Hook (4-param) - Silent Aim Secondary Redirect
    // ─────────────────────────────────────────────────────────────
    static void (*orig_OnCalcDamage_4)(void* player, void* damageInfo,
                                       void* weaponInfo, void* checkParams,
                                       unsigned int vehicleID) = nullptr;

    static void hook_OnCalcDamage_4(void* player, void* damageInfo,
                                    void* weaponInfo, void* checkParams,
                                    unsigned int vehicleID) {
        if ((cfg::aim::silent || cfg::aim::nobita) && damageInfo != nullptr && g_current_target != 0) {
            uint64_t head_node = mem::read<uint64_t>(g_current_target + offsets::Bone_Head);
            if (head_node) {
                Vector3 head_world = data::get_node_position(head_node);
                if (head_world != Vector3::Zero()) {
                    *(short*)((uintptr_t)damageInfo + offsets::DmgInfo_HitGroup) = 1; // Headshot
                    *(Vector3*)((uintptr_t)damageInfo + offsets::DmgInfo_HitLocation) = head_world;
                }
            }
        }

        if (orig_OnCalcDamage_4) {
            orig_OnCalcDamage_4(player, damageInfo, weaponInfo, checkParams, vehicleID);
        }
    }

    void init() {
        g_current_target = 0;

        if (!g_hook_installed && mem::g_lib_base != 0) {
            void* target_fn9 = reinterpret_cast<void*>(mem::g_lib_base + offsets::Player_TakeDamage_9);
            DobbyHook(target_fn9, reinterpret_cast<void*>(hook_OnCalcDamage_9), reinterpret_cast<void**>(&orig_OnCalcDamage_9));

            void* target_fn4 = reinterpret_cast<void*>(mem::g_lib_base + offsets::Player_TakeDamage_4);
            DobbyHook(target_fn4, reinterpret_cast<void*>(hook_OnCalcDamage_4), reinterpret_cast<void**>(&orig_OnCalcDamage_4));

            g_hook_installed = true;
            LOGI("[+] TakeDamage SilentAim Hooks (9-param & 4-param) installed via Dobby");
        }
    }

    void render_fov() {
        if (!cfg::aim::enabled && !cfg::aim::draw_fov) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        float cx = static_cast<float>(display::width > 0 ? display::width : 2400) * 0.5f;
        float cy = static_cast<float>(display::height > 0 ? display::height : 1080) * 0.5f;
        float radius = cfg::aim::fov; // Dynamic FOV size controlled by slider

        if (radius <= 5.0f) return;

        // Clean Thread-Thin Aim Circle (Adjustable radius)
        // If a target is currently locked inside the circle, color shifts to green; otherwise neon purple
        ImU32 circle_col = (g_current_target != 0)
                           ? IM_COL32(50, 230, 90, 210)
                           : IM_COL32(174, 89, 250, 190);

        dl->AddCircle(ImVec2(cx, cy), radius, circle_col, 128, 1.2f);
    }

    static const data::PlayerData* find_best_target(float screen_cx, float screen_cy) {
        std::lock_guard<std::mutex> lk(data::g_players_mutex);

        const data::PlayerData* best = nullptr;
        float min_dist_crosshair = cfg::aim::fov; // Must be strictly within the Circle Aim radius

        for (const auto& p : data::g_players) {
            if (p.curHP <= 0 || p.distance > cfg::aim::distance) continue;

            Vector3 target_pos = p.Head;
            if (cfg::aim::target_bone == 1) target_pos = p.Breast;
            else if (cfg::aim::target_bone == 2) target_pos = p.Hip;

            if (target_pos.x <= 0.0f || target_pos.y <= 0.0f) continue;

            float dx = target_pos.x - screen_cx;
            float dy = target_pos.y - screen_cy;
            float dist_to_center = sqrtf(dx * dx + dy * dy);

            // True real-time targeting: only targets within the circle
            if (dist_to_center <= cfg::aim::fov && dist_to_center < min_dist_crosshair) {
                min_dist_crosshair = dist_to_center;
                best = &p;
            }
        }
        return best;
    }

    void update() {
        if (!cfg::aim::enabled || !data::g_in_match.load()) {
            g_current_target = 0;
            return;
        }

        float cx = static_cast<float>(display::width > 0 ? display::width : 2400) * 0.5f;
        float cy = static_cast<float>(display::height > 0 ? display::height : 1080) * 0.5f;

        const data::PlayerData* target = find_best_target(cx, cy);
        if (target) {
            g_current_target = target->addr;

            // Combat calculation: Aim Assist direction
            uint64_t local = data::g_local_player.load();
            if (local && (cfg::aim::silent || cfg::aim::nobita)) {
                uint64_t aim_info = mem::read<uint64_t>(local + offsets::Player_AimInfo);
                if (aim_info) {
                    Vector3 my_head = mem::read<Vector3>(local + offsets::Player_HeadTF);
                    glm::vec3 p1(my_head.x, my_head.y, my_head.z);
                    glm::vec3 p2(target->Head.x, target->Head.y, target->Head.z);
                    glm::vec3 dir = p2 - p1;
                    if (glm::length(dir) > 0.001f) {
                        dir = glm::normalize(dir);
                        Vector3 dir_out(dir.x, dir.y, dir.z);
                        mem::write<Vector3>(aim_info + offsets::AimInfo_Direction, dir_out);
                    }
                }
            }
        } else {
            g_current_target = 0;
        }
    }

    uint64_t get_current_target() {
        return g_current_target;
    }

} // namespace engine::aim
