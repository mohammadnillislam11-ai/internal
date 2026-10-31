#define IMGUI_DEFINE_MATH_OPERATORS
#include "aimbot.hpp"
#include "Quaternion.h"
#include "game_data.hpp"
#include "offsets.hpp"
#include "memory.hpp"
#include "menu/config.hpp"
#include "menu/bar.hpp"
#include "display_metrics.h"
#include "imgui.h"
#include <cmath>
#include <android/log.h>

#define LOG_TAG "NINMODS_AIM"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace engine::aim {

    static uint64_t g_current_target = 0;
    static bool g_hook_installed = false;

    /**
     * @brief 9-parameter damage intercept hook for silent aim headshot redirection.
     *        Rewrites hit group, hit location, hit normal, and ray direction to target head.
     */
    static int (*orig_OnCalcDamage_9)(void* player, int damage, unsigned int damagerID,
                                      void* damageInfo, int hitGroup,
                                      Vector3 hitPos, Vector3 hitNormal,
                                      void* checkParams, void* weaponInfo, unsigned int vehicleID) = nullptr;

    static int hook_OnCalcDamage_9(void* player, int damage, unsigned int damagerID,
                                   void* damageInfo, int hitGroup,
                                   Vector3 hitPos, Vector3 hitNormal,
                                   void* checkParams, void* weaponInfo, unsigned int vehicleID) {
        if ((cfg::aim::silent || cfg::aim::nobita) && damageInfo != nullptr && g_current_target != 0) {
            uint64_t head_node = rpm<uint64_t>(g_current_target + offsets::Bone_Head);
            if (head_node) {
                Vector3 enemy_head_world = data::get_node_position(head_node);
                if (enemy_head_world != Vector3::Zero()) {
                    Vector3 my_pos = Vector3::Zero();
                    uint64_t local = data::g_local_player.load(std::memory_order_relaxed);
                    if (local) {
                        uint64_t my_head_node = rpm<uint64_t>(local + offsets::Bone_Head);
                        if (my_head_node) my_pos = data::get_node_position(my_head_node);
                    }

                    Vector3 direction = hitNormal;
                    if (my_pos != Vector3::Zero()) {
                        float dx = enemy_head_world.x - my_pos.x;
                        float dy = enemy_head_world.y - my_pos.y;
                        float dz = enemy_head_world.z - my_pos.z;
                        float len = sqrtf(dx * dx + dy * dy + dz * dz);
                        if (len > 0.001f) {
                            direction = Vector3(dx / len, dy / len, dz / len);
                        }
                    }

                    *reinterpret_cast<short*>(reinterpret_cast<uintptr_t>(damageInfo) + offsets::DmgInfo_HitGroup) = 1;
                    *reinterpret_cast<Vector3*>(reinterpret_cast<uintptr_t>(damageInfo) + offsets::DmgInfo_HitLocation) = enemy_head_world;
                    *reinterpret_cast<Vector3*>(reinterpret_cast<uintptr_t>(damageInfo) + offsets::DmgInfo_HitNormal) = direction;
                    *reinterpret_cast<Vector3*>(reinterpret_cast<uintptr_t>(damageInfo) + offsets::DmgInfo_RayDir) = direction;

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

    /**
     * @brief 4-parameter damage intercept hook secondary path.
     */
    static void (*orig_OnCalcDamage_4)(void* player, void* damageInfo,
                                       void* weaponInfo, void* checkParams,
                                       unsigned int vehicleID) = nullptr;

    static void hook_OnCalcDamage_4(void* player, void* damageInfo,
                                    void* weaponInfo, void* checkParams,
                                    unsigned int vehicleID) {
        if ((cfg::aim::silent || cfg::aim::nobita) && damageInfo != nullptr && g_current_target != 0) {
            uint64_t head_node = rpm<uint64_t>(g_current_target + offsets::Bone_Head);
            if (head_node) {
                Vector3 head_world = data::get_node_position(head_node);
                if (head_world != Vector3::Zero()) {
                    *reinterpret_cast<short*>(reinterpret_cast<uintptr_t>(damageInfo) + offsets::DmgInfo_HitGroup) = 1;
                    *reinterpret_cast<Vector3*>(reinterpret_cast<uintptr_t>(damageInfo) + offsets::DmgInfo_HitLocation) = head_world;
                }
            }
        }

        if (orig_OnCalcDamage_4) {
            orig_OnCalcDamage_4(player, damageInfo, weaponInfo, checkParams, vehicleID);
        }
    }

    void init() {
        g_current_target = 0;
        if (!g_hook_installed && engine::mem::g_lib_base != 0) {
            g_hook_installed = true;
            LOGI("[+] Aimbot initialized (Direct memory vector redirection active)");
        }
    }

    /**
     * @brief Renders 2D FOV circle overlay centered on the screen.
     */
    void render_fov() {
        if (!cfg::aim::draw_fov || ui::bar::g_open) return;

        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        if (!dl) return;

        float cx = static_cast<float>(display::width > 0 ? display::width : 2400) * 0.5f;
        float cy = static_cast<float>(display::height > 0 ? display::height : 1080) * 0.5f;
        float radius = cfg::aim::fov;

        if (radius <= 5.0f) return;

        ImU32 circle_col = (g_current_target != 0)
                           ? IM_COL32(50, 230, 90, 220)
                           : IM_COL32(174, 89, 250, 190);

        dl->AddCircle(ImVec2(cx, cy), radius, circle_col, 128, 1.5f);
        dl->AddCircleFilled(ImVec2(cx, cy), 2.0f, circle_col);
    }

    /**
     * @brief Finds closest enemy to crosshair strictly within 2D FOV circle on screen.
     *        No off-screen 3D distance lock; pure clean circle aiming.
     */
    static bool find_best_target_fov(float screen_cx, float screen_cy, data::PlayerData& out_target) {
        if (cfg::aim::target_bone < 0) return false;
        std::lock_guard<std::mutex> lk(data::g_players_mutex);

        bool found = false;
        float min_dist_crosshair = cfg::aim::fov;

        for (const auto& p : data::g_players) {
            if (!p.isOnScreen) continue;
            if (p.curHP <= 0 || p.distance > cfg::aim::distance) continue;

            Vector3 target_pos = p.Head;
            if (cfg::aim::target_bone == 1) target_pos = p.Breast;
            else if (cfg::aim::target_bone == 2) target_pos = p.Hip;

            if (target_pos.x <= 0.0f || target_pos.y <= 0.0f) continue;

            float dx = target_pos.x - screen_cx;
            float dy = target_pos.y - screen_cy;
            float dist_to_center = sqrtf(dx * dx + dy * dy);

            if (dist_to_center <= cfg::aim::fov && dist_to_center < min_dist_crosshair) {
                min_dist_crosshair = dist_to_center;
                out_target = p;
                found = true;
            }
        }
        return found;
    }

    /**
     * @brief Per-frame aim assist and memory redirection update.
     *        Strictly targets entities within 2D FOV circle.
     */
    void update() {
        bool aim_active = cfg::aim::silent || cfg::aim::nobita;
        if (!aim_active || !data::g_in_match.load(std::memory_order_relaxed)) {
            g_current_target = 0;
            return;
        }

        uint64_t local = data::g_local_player.load(std::memory_order_relaxed);
        if (!engine::mem::addr_valid(local)) {
            g_current_target = 0;
            return;
        }

        // Trigger condition check (On Fire, On Scope, or Always)
        if (cfg::aim::trigger_type == 1) { // On Fire
            bool is_firing = rpm<bool>(local + offsets::Player_IsFiring) || rpm<bool>(local + 0x848);
            if (!is_firing) {
                g_current_target = 0;
                return;
            }
        } else if (cfg::aim::trigger_type == 2) { // On Scope
            bool is_sight = rpm<bool>(local + offsets::Player_IsSight);
            if (!is_sight) {
                g_current_target = 0;
                return;
            }
        }

        // Resolve AimInfo with multi-version fallback (0xE58, 0xE50, 0xD80)
        uint64_t aim_info = rpm<uint64_t>(local + offsets::Player_AimInfo);
        if (!engine::mem::addr_valid(aim_info)) {
            aim_info = rpm<uint64_t>(local + 0xE50);
        }
        if (!engine::mem::addr_valid(aim_info)) {
            aim_info = rpm<uint64_t>(local + 0xD80);
        }

        float cx = static_cast<float>(display::width > 0 ? display::width : 2400) * 0.5f;
        float cy = static_cast<float>(display::height > 0 ? display::height : 1080) * 0.5f;

        // Strictly target enemy within the 2D FOV circle (safe copy by value)
        data::PlayerData target;
        if (find_best_target_fov(cx, cy, target)) {
            g_current_target = target.addr;

            Vector3 enemy_bone_world = target.HeadWorld;
            if (cfg::aim::target_bone == 1) {
                uint64_t breast_node = rpm<uint64_t>(target.addr + offsets::Bone_Breast);
                if (breast_node) enemy_bone_world = data::get_node_position(breast_node);
            } else if (cfg::aim::target_bone == 2) {
                uint64_t hip_node = rpm<uint64_t>(target.addr + offsets::Bone_Hip);
                if (hip_node) enemy_bone_world = data::get_node_position(hip_node);
            }

            if (enemy_bone_world != Vector3::Zero()) {
                // 1. Silent Aim (RayDir vector redirection - verified in STORM_MODS & NEXOMODSROOT)
                if (cfg::aim::silent && engine::mem::addr_valid(aim_info)) {
                    Vector3 start_pos = rpm<Vector3>(aim_info + offsets::AimInfo_StartPosition);
                    if (start_pos == Vector3::Zero()) {
                        uint64_t my_head_node = rpm<uint64_t>(local + offsets::Bone_Head);
                        if (!my_head_node) my_head_node = rpm<uint64_t>(local + 0x6A0);
                        start_pos = (my_head_node) ? data::get_node_position(my_head_node) : Vector3::Zero();
                    }
                    if (start_pos != Vector3::Zero()) {
                        Vector3 dir = enemy_bone_world - start_pos;
                        wpm<Vector3>(aim_info + offsets::AimInfo_Direction, dir);
                    }
                }

                // 2. Nobita Mode / Aim Lock (Rotation quaternion write - verified in STORM_MODS)
                if (cfg::aim::nobita) {
                    uint64_t follow = rpm<uint64_t>(local + offsets::Player_Camera);
                    if (!follow) follow = rpm<uint64_t>(local + 0x690);
                    uint64_t cam = follow ? rpm<uint64_t>(follow + offsets::Camera_Follow) : 0;
                    if (!cam && follow) cam = rpm<uint64_t>(follow + 0x30);
                    uint64_t int_ptr = cam ? rpm<uint64_t>(cam + offsets::Camera_IntPtr) : 0;
                    if (!int_ptr && cam) int_ptr = rpm<uint64_t>(cam + 0x10);

                    Vector3 cam_pos = Vector3::Zero();
                    if (int_ptr) {
                        Matrix4x4 mat = rpm<Matrix4x4>(int_ptr + offsets::Camera_Matrix);
                        cam_pos = Vector3(mat.m[3][0], mat.m[3][1], mat.m[3][2]);
                    }
                    if (cam_pos == Vector3::Zero()) {
                        uint64_t my_head = rpm<uint64_t>(local + offsets::Bone_Head);
                        if (!my_head) my_head = rpm<uint64_t>(local + 0x6A0);
                        cam_pos = my_head ? data::get_node_position(my_head) : Vector3::Zero();
                    }

                    if (cam_pos != Vector3::Zero()) {
                        Vector3 dir = enemy_bone_world - cam_pos;
                        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                        if (len > 0.001f) {
                            dir.x /= len; dir.y /= len; dir.z /= len;
                            Quaternion rot = Quaternion::LookRotation(dir, Vector3(0.0f, 1.0f, 0.0f));
                            wpm<Quaternion>(local + offsets::Player_Rotation, rot);
                            wpm<Quaternion>(local + 0x614, rot);
                        }
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
