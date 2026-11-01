#define IMGUI_DEFINE_MATH_OPERATORS
#include "aimbot.hpp"
#include "game_data.hpp"
#include "Quaternion.h"
#include "offsets.hpp"
#include "memory.hpp"
#include "menu/config.hpp"
#include "menu/bar.hpp"
#include "display_metrics.h"
#include "imgui.h"
#include <cmath>
#include <android/log.h>

#define LOG_TAG "NINMODS_AIM"
#define LOGI(...) ((void)0)

namespace engine::aim {

    static uint64_t g_current_target = 0;
    static bool g_hook_installed = false;

    // Safety state tracking for reversible memory restoration
    static bool s_silent_modified = false;
    static uint64_t s_last_aim_info = 0;
    static Vector3 s_saved_dir = Vector3::Zero();

    static bool s_nobita_modified = false;
    static uint64_t s_last_local = 0;
    static Quaternion s_saved_rot{};

    /**
     * @brief Restores original game memory vectors and rotations when aim is turned off or target is lost.
     */
    void restore_state() {
        if (s_silent_modified && engine::mem::addr_valid(s_last_aim_info)) {
            if (s_saved_dir != Vector3::Zero()) {
                wpm<Vector3>(s_last_aim_info + offsets::AimInfo_Direction, s_saved_dir);
            }
            s_silent_modified = false;
            s_saved_dir = Vector3::Zero();
            s_last_aim_info = 0;
        }

        if (s_nobita_modified && engine::mem::addr_valid(s_last_local)) {
            if (s_saved_rot.w != 0.0f || s_saved_rot.x != 0.0f || s_saved_rot.y != 0.0f || s_saved_rot.z != 0.0f) {
                wpm<Quaternion>(s_last_local + offsets::Player_Rotation, s_saved_rot);
                wpm<Quaternion>(s_last_local + 0x614, s_saved_rot);
            }
            s_nobita_modified = false;
            s_saved_rot = Quaternion{};
            s_last_local = 0;
        }
        g_current_target = 0;
    }

    void init() {
        g_current_target = 0;
        s_silent_modified = false;
        s_nobita_modified = false;
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
     *        Safely saves original direction/rotation and restores on toggle off.
     */
    void update() {
        bool aim_active = cfg::aim::silent || cfg::aim::nobita;
        if (!aim_active || !data::g_in_match.load(std::memory_order_relaxed)) {
            restore_state();
            return;
        }

        uint64_t local = data::g_local_player.load(std::memory_order_relaxed);
        if (!engine::mem::addr_valid(local)) {
            restore_state();
            return;
        }

        // Trigger condition check (On Fire, On Scope, or Always)
        if (cfg::aim::trigger_type == 1) { // On Fire
            bool is_firing = rpm<bool>(local + offsets::Player_IsFiring) || rpm<bool>(local + 0x848);
            if (!is_firing) {
                restore_state();
                return;
            }
        } else if (cfg::aim::trigger_type == 2) { // On Scope
            bool is_sight = rpm<bool>(local + offsets::Player_IsSight);
            if (!is_sight) {
                restore_state();
                return;
            }
        }

        // Resolve AimInfo with multi-version fallback (0xE58, 0xE50, 0xD80)
        uint64_t aim_info = rpm<uint64_t>(local + offsets::Player_AimInfo);
        if (!engine::mem::addr_valid(aim_info)) aim_info = rpm<uint64_t>(local + 0xE50);
        if (!engine::mem::addr_valid(aim_info)) aim_info = rpm<uint64_t>(local + 0xD80);

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
                // 1. Silent Aim with clean safety save/restore
                if (cfg::aim::silent && engine::mem::addr_valid(aim_info)) {
                    if (!s_silent_modified || s_last_aim_info != aim_info) {
                        Vector3 current_dir = rpm<Vector3>(aim_info + offsets::AimInfo_Direction);
                        if (current_dir != Vector3::Zero()) {
                            s_saved_dir = current_dir;
                        }
                        s_last_aim_info = aim_info;
                        s_silent_modified = true;
                    }

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
                } else if (!cfg::aim::silent && s_silent_modified) {
                    // Revert silent aim if turned off
                    if (engine::mem::addr_valid(s_last_aim_info) && s_saved_dir != Vector3::Zero()) {
                        wpm<Vector3>(s_last_aim_info + offsets::AimInfo_Direction, s_saved_dir);
                    }
                    s_silent_modified = false;
                }

                // 2. Nobita Mode with clean safety save/restore
                if (cfg::aim::nobita) {
                    if (!s_nobita_modified || s_last_local != local) {
                        s_saved_rot = rpm<Quaternion>(local + offsets::Player_Rotation);
                        s_last_local = local;
                        s_nobita_modified = true;
                    }

                    uint64_t follow = rpm<uint64_t>(local + offsets::Player_Camera);
                    if (!follow) follow = rpm<uint64_t>(local + 0x690);
                    uint64_t cam = follow ? rpm<uint64_t>(follow + offsets::Camera_Follow) : 0;
                    if (!cam && follow) cam = rpm<uint64_t>(follow + 0x30);
                    uint64_t int_ptr = cam ? rpm<uint64_t>(cam + offsets::Camera_IntPtr) : 0;
                    if (!int_ptr && cam) int_ptr = rpm<uint64_t>(cam + 0x10);

                    Vector3 cam_pos = Vector3::Zero();
                    if (int_ptr) {
                        data::Matrix4x4 m = rpm<data::Matrix4x4>(int_ptr + offsets::Camera_Matrix);
                        float a11 = m.m11, a12 = m.m21, a13 = m.m31;
                        float a21 = m.m12, a22 = m.m22, a23 = m.m32;
                        float a31 = m.m14, a32 = m.m24, a33 = m.m34;
                        float b1 = -m.m41, b2 = -m.m42, b3 = -m.m44;

                        float det = a11 * (a22 * a33 - a23 * a32)
                                    - a12 * (a21 * a33 - a23 * a31)
                                    + a13 * (a21 * a32 - a22 * a31);

                        if (std::abs(det) > 0.0001f) {
                            float inv = 1.0f / det;
                            cam_pos = Vector3(
                                (b1 * (a22 * a33 - a23 * a32) - a12 * (b2 * a33 - a23 * b3) + a13 * (b2 * a32 - a22 * b3)) * inv,
                                (a11 * (b2 * a33 - a23 * b3) - b1 * (a21 * a33 - a23 * a31) + a13 * (a21 * b3 - b2 * a31)) * inv,
                                (a11 * (a22 * b3 - b2 * a32) - a12 * (a21 * b3 - b2 * a31) + b1 * (a21 * a32 - a22 * a31)) * inv
                            );
                        }
                    }
                    if (cam_pos == Vector3::Zero()) {
                        uint64_t my_head = rpm<uint64_t>(local + offsets::Bone_Head);
                        if (!my_head) my_head = rpm<uint64_t>(local + offsets::Player_HeadTF);
                        if (!my_head) my_head = rpm<uint64_t>(local + 0x6A0);
                        cam_pos = my_head ? data::get_node_position(my_head) : Vector3::Zero();
                    }

                    if (cam_pos != Vector3::Zero()) {
                        Vector3 dir = enemy_bone_world - cam_pos;
                        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                        if (len > 0.001f) {
                            dir.x /= len; dir.y /= len; dir.z /= len;
                            Quaternion target_rot = Quaternion::LookRotation(dir, Vector3(0.0f, 1.0f, 0.0f));
                            Quaternion final_rot = target_rot;
                            if (cfg::aim::smooth > 1.0f) {
                                Quaternion cur_rot = rpm<Quaternion>(local + offsets::Player_Rotation);
                                if (cur_rot.x != 0.0f || cur_rot.y != 0.0f || cur_rot.z != 0.0f || cur_rot.w != 0.0f) {
                                    float t = 1.0f / cfg::aim::smooth;
                                    if (t < 0.05f) t = 0.05f;
                                    if (t > 1.0f) t = 1.0f;
                                    final_rot = Quaternion::Slerp(cur_rot, target_rot, t);
                                }
                            }
                            wpm<Quaternion>(local + offsets::Player_Rotation, final_rot);
                            wpm<Quaternion>(local + 0x614, final_rot);
                        }
                    }
                } else if (!cfg::aim::nobita && s_nobita_modified) {
                    // Revert Nobita rotation if turned off
                    if (engine::mem::addr_valid(s_last_local) && (s_saved_rot.w != 0.0f || s_saved_rot.x != 0.0f)) {
                        wpm<Quaternion>(s_last_local + offsets::Player_Rotation, s_saved_rot);
                        wpm<Quaternion>(s_last_local + 0x614, s_saved_rot);
                    }
                    s_nobita_modified = false;
                }
            }
        } else {
            restore_state();
        }
    }

    uint64_t get_current_target() {
        return g_current_target;
    }

} // namespace engine::aim
