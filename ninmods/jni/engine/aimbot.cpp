#define IMGUI_DEFINE_MATH_OPERATORS
#include "aimbot.hpp"
#include "game_data.hpp"
#include "offsets.hpp"
#include "memory.hpp"
#include "menu/config.hpp"
#include "display_metrics.h"
#include "imgui.h"
#include "glm.hpp"
#include <cmath>
#include <android/log.h>

#define LOG_TAG "NINMODS_AIM"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace engine::aim {

    static uint64_t g_current_target = 0;
    static bool g_hook_installed = false;

    /**
     * @brief 9-parameter damage intercept hook for silent aim headshot redirection.
     *        Rewrites hit group, hit location, hit normal, and ray direction to the
     *        nearest locked enemy head world position.
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

                    *(short*)((uintptr_t)damageInfo + offsets::DmgInfo_HitGroup) = 1;
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

    /**
     * @brief 4-parameter damage intercept hook — secondary headshot redirect path.
     *        Used when the game calls a simplified damage variant.
     */
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
                    *(short*)((uintptr_t)damageInfo + offsets::DmgInfo_HitGroup) = 1;
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
            g_hook_installed = true;
            LOGI("[+] Aimbot hooks disabled (cross-process, using memory read/write approach)");
        }
    }

    /**
     * @brief Renders the 2D FOV circle overlay centered on the screen.
     *        Color shifts to green when a target is locked; otherwise neon purple.
     *        Only active when cfg::aim::draw_fov is true.
     */
    void render_fov() {
        if (!cfg::aim::draw_fov) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
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
     * @brief Finds the closest enemy whose head screen position falls within the FOV circle radius.
     * @param screen_cx Screen horizontal center.
     * @param screen_cy Screen vertical center.
     * @return Pointer to the nearest qualifying PlayerData, or nullptr if none found.
     */
    static const data::PlayerData* find_best_target(float screen_cx, float screen_cy) {
        std::lock_guard<std::mutex> lk(data::g_players_mutex);

        const data::PlayerData* best = nullptr;
        float min_dist_crosshair = cfg::aim::fov;

        for (const auto& p : data::g_players) {
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
                best = &p;
            }
        }
        return best;
    }

    /**
     * @brief Per-frame aim assist update. Selects the best target within the FOV circle and
     *        writes the normalized 3D direction vector to the AimInfo memory offset via
     *        process_vm_writev. No-ops when cfg::aim::enabled is false or not in match.
     */
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

            uint64_t local = data::g_local_player.load(std::memory_order_relaxed);
            if (mem::is_valid_addr(local) && (cfg::aim::silent || cfg::aim::nobita || cfg::aim::draw_fov)) {
                uint64_t aim_info = mem::read<uint64_t>(local + offsets::Player_AimInfo);
                if (mem::is_valid_addr(aim_info)) {
                    uint64_t my_head_node = mem::read<uint64_t>(local + offsets::Bone_Head);
                    Vector3 my_pos = (my_head_node) ? data::get_node_position(my_head_node) : Vector3::Zero();
                    Vector3 enemy_head = target->HeadWorld;
                    if (my_pos != Vector3::Zero() && enemy_head != Vector3::Zero()) {
                        glm::vec3 p1(my_pos.x, my_pos.y, my_pos.z);
                        glm::vec3 p2(enemy_head.x, enemy_head.y, enemy_head.z);
                        glm::vec3 dir = p2 - p1;
                        float len = glm::length(dir);
                        if (std::isfinite(len) && len > 0.001f) {
                            dir = glm::normalize(dir);
                            Vector3 dir_out(dir.x, dir.y, dir.z);
                            if (std::isfinite(dir_out.x) && std::isfinite(dir_out.y) && std::isfinite(dir_out.z)) {
                                mem::write<Vector3>(aim_info + offsets::AimInfo_Direction, dir_out);
                            }
                        }
                    }
                }
            }
        } else {
            g_current_target = 0;
        }
    }

    /** @return Memory address of the currently tracked target entity, or 0 if none. */
    uint64_t get_current_target() {
        return g_current_target;
    }

} // namespace engine::aim
