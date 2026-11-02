#include <sys/prctl.h>
#include "game_data.hpp"
#include "offsets.hpp"
#include "memory.hpp"
#include "menu/config.hpp"
#include "display_metrics.h"
#include "libraries/storage/logger.h"
#include "oxorany.h"
#include <thread>
#include <chrono>
#include <unordered_map>
#include <algorithm>

namespace engine::data {

    std::vector<PlayerData> g_players;
    std::mutex              g_players_mutex;

    std::vector<LootItem>   g_loots;
    std::mutex              g_loots_mutex;

    std::atomic<uint64_t>   g_local_player{0};
    std::atomic<bool>       g_in_match{false};
    Matrix4x4               g_view_matrix{};

    static std::atomic<bool> g_worker_running{false};
    static std::thread       g_worker_thread;

    static std::unordered_map<uint64_t, int>  g_vis_counter;
    static std::unordered_map<uint64_t, bool> g_vis_stable;

    /**
     * @brief Computes Euclidean distance between two Vector3 points.
     */
    float get_distance_meters(const Vector3& a, const Vector3& b) {
        float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    /**
     * @brief Projects 3D world coordinates to 2D viewport coordinates using camera projection matrix.
     */
    Vector3 world_to_screen(const Matrix4x4& vm, const Vector3& pos, float screen_w, float screen_h) {
        float v12 = (pos.x * vm.m14) + (pos.y * vm.m24) + (pos.z * vm.m34) + vm.m44;
        if (v12 >= 0.001f) {
            float inv = 1.0f / v12;
            float cx = screen_w * 0.5f;
            float cy = screen_h * 0.5f;
            float v9  = (pos.x * vm.m11) + (pos.y * vm.m21) + (pos.z * vm.m31) + vm.m41;
            float v10 = (pos.x * vm.m12) + (pos.y * vm.m22) + (pos.z * vm.m32) + vm.m42;
            return Vector3(cx + cx * v9 * inv, cy - cy * v10 * inv, 0.0f);
        }
        return Vector3(-1.0f, -1.0f, -1.0f);
    }

    struct ScreenProjection {
        Vector3 screen_pos;
        bool    on_screen{false};
        bool    behind{false};
    };

    /**
     * @brief Projects 3D world coordinates to screen with full 360-degree border clamping.
     *        Enables real-time directional tracer lines pointing towards enemy regardless
     *        of whether they are in front, behind, or around the player.
     */
    static ScreenProjection project_world_to_screen_360(const Matrix4x4& vm, const Vector3& pos, float screen_w, float screen_h) {
        ScreenProjection res;
        float cx = screen_w * 0.5f;
        float cy = screen_h * 0.5f;

        float x = (pos.x * vm.m11) + (pos.y * vm.m21) + (pos.z * vm.m31) + vm.m41;
        float y = (pos.x * vm.m12) + (pos.y * vm.m22) + (pos.z * vm.m32) + vm.m42;
        float w = (pos.x * vm.m14) + (pos.y * vm.m24) + (pos.z * vm.m34) + vm.m44;

        if (w > 0.001f) {
            float inv = 1.0f / w;
            float sx = cx + cx * x * inv;
            float sy = cy - cy * y * inv;

            if (sx >= 0.0f && sx <= screen_w && sy >= 0.0f && sy <= screen_h) {
                res.screen_pos = Vector3(sx, sy, 0.0f);
                res.on_screen = true;
                res.behind = false;
                return res;
            }
        }

        res.behind = (w <= 0.001f);
        res.on_screen = false;

        float dir_x = 0.0f;
        float dir_y = 0.0f;

        if (w > 0.001f) {
            float inv = 1.0f / w;
            dir_x = cx * x * inv;
            dir_y = -cy * y * inv;
        } else {
            float angle = atan2f(x, w);
            dir_x = sinf(angle);
            dir_y = -cosf(angle);
        }

        float len = sqrtf(dir_x * dir_x + dir_y * dir_y);
        if (len > 0.0001f) {
            dir_x /= len;
            dir_y /= len;
        } else {
            dir_x = 0.0f;
            dir_y = 1.0f;
        }

        float margin = 28.0f;
        float half_w = cx - margin;
        float half_h = cy - margin;

        float scale_x = (fabsf(dir_x) > 1e-5f) ? (half_w / fabsf(dir_x)) : 1e9f;
        float scale_y = (fabsf(dir_y) > 1e-5f) ? (half_h / fabsf(dir_y)) : 1e9f;
        float t = (scale_x < scale_y) ? scale_x : scale_y;

        res.screen_pos = Vector3(cx + dir_x * t, cy + dir_y * t, 0.0f);
        return res;
    }

    /**
     * @brief Reconstructs a Unity Transform's world position by walking the parent hierarchy.
     *        Standardized from ZqwClient TRS matrix chain engine.
     */
    static Vector3 get_transform_position(uint64_t transform) {
        auto transform_obj = rpm<uint64_t>(transform + 0x10);
        if (transform_obj == 0) return Vector3::Zero();

        struct { uint64_t matrices; uint64_t indices; } ptrs{};
        auto matrix_ptr = rpm<uint64_t>(transform_obj + 0x38);
        if (!matrix_ptr) return Vector3::Zero();

        ptrs.matrices = rpm<uint64_t>(matrix_ptr + 0x18);
        ptrs.indices  = rpm<uint64_t>(matrix_ptr + 0x20);
        if (!ptrs.matrices || !ptrs.indices) return Vector3::Zero();

        auto index_val = rpm<uint64_t>(transform_obj + 0x40);

        auto result = rpm<Vector3>(ptrs.matrices + (index_val * 0x30));
        auto transform_index = rpm<int>(ptrs.indices + (index_val * 4));

        for (int tries = 0; transform_index >= 0 && tries < 50; tries++) {
            uint64_t mat_addr = ptrs.matrices + (transform_index * 0x30);
            auto t = rpm<TransformMatrix>(mat_addr);

            auto rx = t.rotation.x, ry = t.rotation.y, rz = t.rotation.z, rw = t.rotation.w;
            auto sx = result.x * t.scale.x;
            auto sy = result.y * t.scale.y;
            auto sz = result.z * t.scale.z;

            result.x = t.position.x + sx +
                       (sx * ((ry * ry * -2.0f) - (rz * rz * 2.0f))) +
                       (sy * ((rw * rz * -2.0f) - (ry * rx * -2.0f))) +
                       (sz * ((rz * rx *  2.0f) - (rw * ry * -2.0f)));
            result.y = t.position.y + sy +
                       (sx * ((rx * ry *  2.0f) - (rw * rz * -2.0f))) +
                       (sy * ((rz * rz * -2.0f) - (rx * rx *  2.0f))) +
                       (sz * ((rw * rx * -2.0f) - (rz * ry * -2.0f)));
            result.z = t.position.z + sz +
                       (sx * ((rw * ry * -2.0f) - (rx * rz * -2.0f))) +
                       (sy * ((ry * rz *  2.0f) - (rw * rx * -2.0f))) +
                       (sz * ((rx * rx * -2.0f) - (ry * ry *  2.0f)));

            transform_index = rpm<int>(ptrs.indices + (transform_index * 4));
        }

        return result;
    }

    Vector3 get_node_position(uint64_t node) {
        auto transform_val = rpm<uint64_t>(node + 0x10);
        if (transform_val == 0) return Vector3::Zero();
        return get_transform_position(transform_val);
    }

    /**
     * @brief Helper that extracts node world position and translates it directly to screen coordinates.
     */
    static Vector3 bone_screen(uint64_t player, uintptr_t offset, float sw, float sh) {
        uint64_t node = rpm<uint64_t>(player + offset);
        if (!node) return Vector3::Zero();
        Vector3 w = get_node_position(node);
        if (w == Vector3::Zero()) return Vector3::Zero();
        return world_to_screen(g_view_matrix, w, sw, sh);
    }

    /**
     * @brief Reads player health from the managed health pool chain.
     */
    static int read_hp(uint64_t player) {
        auto pool = rpm<uint64_t>(player + offsets::Player_HP);
        if (!pool) return 0;
        auto datas = rpm<uint64_t>(pool + offsets::HP_List);
        if (!datas) return 0;
        auto elem = rpm<uint64_t>(datas + offsets::HP_Entry);
        if (!elem) return 0;
        return rpm<int>(elem + offsets::HP_Value);
    }



    /**
     * @brief Managed IL2CPP Dictionary wrapper supporting both standard monoDictionary (entries at +0x28)
     *        and packed array layouts.
     */
    class ManagedDictionary {
    public:
        uint64_t address{0};
        uint64_t entries_head{0};
        int      count{0};
        bool     valid{false};

        explicit ManagedDictionary(uint64_t addr) : address(addr) {
            if (!engine::mem::addr_valid(addr)) return;

            // Strategy 1: Standard Unity IL2CPP monoDictionary - monoArray<V>* values at +0x28
            uint64_t values_arr = rpm<uint64_t>(addr + 0x28);
            if (engine::mem::addr_valid(values_arr)) {
                int len = rpm<int>(values_arr + 0x18); // monoArray max_length
                if (len <= 0 || len > 2000) {
                    len = rpm<int>(addr + 0x38); // fallback to dict->size
                }
                if (len > 0 && len <= 2000) {
                    entries_head = values_arr + 0x20; // monoArray vector offset
                    count = len;
                    valid = true;
                    return;
                }
            }

            // Strategy 2: monoArray at +0x18
            uint64_t arr18 = rpm<uint64_t>(addr + 0x18);
            if (engine::mem::addr_valid(arr18)) {
                int len = rpm<int>(arr18 + 0x18);
                if (len <= 0 || len > 2000) len = rpm<int>(addr + 0x20);
                if (len > 0 && len <= 2000) {
                    entries_head = arr18 + 0x20;
                    count = len;
                    valid = true;
                    return;
                }
            }

            // Strategy 3: monoArray at +0x20
            uint64_t arr20 = rpm<uint64_t>(addr + 0x20);
            if (engine::mem::addr_valid(arr20)) {
                int len = rpm<int>(arr20 + 0x18);
                if (len <= 0 || len > 2000) len = rpm<int>(addr + 0x38);
                if (len > 0 && len <= 2000) {
                    entries_head = arr20 + 0x20;
                    count = len;
                    valid = true;
                    return;
                }
            }
        }

        inline int get_count() const { return count; }

        inline uint64_t entry(int i) const {
            if (!valid || i < 0 || i >= count) return 0;
            // Each IL2CPP Entry<K, V> is 0x18 bytes, Value pointer is at offset +0x10
            uint64_t entry_addr = entries_head + (i * 0x18);
            uint64_t val = rpm<uint64_t>(entry_addr + 0x10);
            if (!engine::mem::addr_valid(val)) {
                // Fallback: direct pointer array (each entry 8 bytes)
                val = rpm<uint64_t>(entries_head + (i * 8));
            }
            return val;
        }
    };



    static inline uint64_t resolve_candidate_basegame(uint64_t cell_addr, uint64_t& out_ptr1, uint64_t& out_ptr2, uint64_t& out_sf) {
        out_ptr1 = 0; out_ptr2 = 0; out_sf = 0;
        if (!engine::mem::addr_valid(cell_addr)) return 0;

        uint64_t ptr1 = rpm<uint64_t>(cell_addr);
        if (!engine::mem::addr_valid(ptr1)) return 0;
        out_ptr1 = ptr1;

        // Path A: ptr1 is gf_cell (Double indirection: cell -> class -> static_fields -> base_game)
        uint64_t ptr2 = rpm<uint64_t>(ptr1);
        if (engine::mem::addr_valid(ptr2)) {
            out_ptr2 = ptr2;
            for (uintptr_t p2_off : {0xB8, 0xB0, 0xC0}) {
                uint64_t sf = rpm<uint64_t>(ptr2 + p2_off);
                if (engine::mem::addr_valid(sf)) {
                    out_sf = sf;
                    for (uintptr_t bg_off : {0x8, 0x0, 0x10}) {
                        uint64_t bg = rpm<uint64_t>(sf + bg_off);
                        if (engine::mem::addr_valid(bg)) return bg;
                    }
                }
            }
        }

        // Path B: ptr1 is gf_class directly (Single indirection: cell -> static_fields -> base_game)
        for (uintptr_t p2_off : {0xB8, 0xB0, 0xC0}) {
            uint64_t sf = rpm<uint64_t>(ptr1 + p2_off);
            if (engine::mem::addr_valid(sf)) {
                out_sf = sf;
                for (uintptr_t bg_off : {0x8, 0x0, 0x10}) {
                    uint64_t bg = rpm<uint64_t>(sf + bg_off);
                    if (engine::mem::addr_valid(bg)) return bg;
                }
            }
        }

        return 0;
    }

    /**
     * @brief One full entity scan cycle with comprehensive sector diagnostics.
     */
    void update_cycle() {
        int pid = (engine::mem::g_pid > 0) ? engine::mem::g_pid : proc::pid;
        uintptr_t lib = (engine::mem::g_lib_base > 0) ? engine::mem::g_lib_base : proc::lib;
        if (pid <= 0 || lib == 0) return;

        static auto s_epoch = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s_epoch).count();
        double now_sec = static_cast<double>(elapsed) / 1000.0;

        static double s_last_diag_tick = 0.0;
        bool print_diag = (now_sec - s_last_diag_tick >= 2.5);
        if (print_diag) s_last_diag_tick = now_sec;

        // SECTOR 1: Verify ELF Header Readability
        static bool s_elf_verified = false;
        if (!s_elf_verified) {
            uint32_t elf_magic = rpm<uint32_t>(lib);
            if (elf_magic == 0x464C457F) {
                logger::log_success("[SECTOR-1] Memory Read Verified: libil2cpp ELF=0x%08X (engine=%d)", elf_magic, engine::mem::g_last_engine_mode);
                s_elf_verified = true;
            } else {
                int err = engine::mem::g_last_read_errno;
                logger::log_warn("[SECTOR-1] Memory Read Alert: libil2cpp at 0x%lx returned 0x%08X (errno=%d: %s, engine=%d)",
                    (unsigned long)lib, elf_magic, err, (err != 0 ? strerror(err) : "None"), engine::mem::g_last_engine_mode);
            }
        }

        float screen_w = static_cast<float>(display::width > 0 ? display::width : 2400);
        float screen_h = static_cast<float>(display::height > 0 ? display::height : 1080);

        std::vector<PlayerData> tmp_players;
        tmp_players.reserve(64);

        // SECTOR 1: BaseGame Candidate Evaluation
        static uintptr_t s_working_facade_offset = 0;
        uint64_t base_game = 0;
        uint64_t diag_p1 = 0, diag_p2 = 0, diag_sf = 0;

        if (s_working_facade_offset != 0) {
            base_game = resolve_candidate_basegame(lib + s_working_facade_offset, diag_p1, diag_p2, diag_sf);
            if (!base_game) s_working_facade_offset = 0;
        }

        static const uintptr_t candidates[] = {
            0xA72E4E0, // ZqwClient / FF Normal OB55 / 1.132.x
            0xB4C41A0, // NEXOMODS FF
            0xA986B7C, // NEXOMODS FF alt
            0xAD6DC18, // STORM_MODS FF MAX
            0xAD68E18, // STORM_MODS FF MAX Ind
            0xA7384E0,
            0xA8E44E0,
            0xAD74BB0,
            0xAD6FDA8,
            0xAB74BB0,
            0xA62E4E0,
            0xA82E4E0,
            0xA92E4E0,
            0xA72E500,
            0xA72E4C0
        };

        if (!base_game) {
            for (uintptr_t cand : candidates) {
                uint64_t bg = resolve_candidate_basegame(lib + cand, diag_p1, diag_p2, diag_sf);
                if (bg) {
                    s_working_facade_offset = cand;
                    base_game = bg;
                    logger::log_success("[SECTOR-1] LINKED BaseGame: 0x%lx (Offset: 0x%lx, cell=0x%lx, class=0x%lx, sf=0x%lx)",
                        (unsigned long)bg, (unsigned long)cand, (unsigned long)diag_p1, (unsigned long)diag_p2, (unsigned long)diag_sf);
                    break;
                }
            }

            if (!base_game && print_diag) {
                uint64_t p1 = 0, p2 = 0, sf = 0;
                resolve_candidate_basegame(lib + candidates[0], p1, p2, sf);
                logger::log_info("[SECTOR-1] Searching BaseGame... Cand 0x%lx: cell=0x%lx, class=0x%lx, sf=0x%lx",
                    (unsigned long)candidates[0], (unsigned long)p1, (unsigned long)p2, (unsigned long)sf);
            }
        }

        if (!base_game) return;

        // SECTOR 2: Match & Local Player Resolution
        uint64_t match = rpm<uint64_t>(base_game + offsets::BaseGame_Match);
        uint64_t local_player = 0;
        if (match && engine::mem::addr_valid(match)) {
            local_player = rpm<uint64_t>(match + offsets::Match_LocalPlayer);
            if (!local_player) {
                for (uintptr_t lpoff : {0xD8, 0xD0, 0xE0, 0xC8}) {
                    uint64_t lp = rpm<uint64_t>(match + lpoff);
                    if (engine::mem::addr_valid(lp)) { local_player = lp; break; }
                }
            }
        }
        if (!local_player) {
            local_player = rpm<uint64_t>(base_game + offsets::Match_LocalPlayer);
        }

        // SECTOR 3: Player Dictionary Resolution (probes multiple offsets)
        uint64_t dict_addr = 0;
        for (uintptr_t doff : {0xC0, 0xC8, 0xB8, 0xB0, 0xD0}) {
            uint64_t d = rpm<uint64_t>(base_game + doff);
            if (engine::mem::addr_valid(d)) {
                ManagedDictionary test_dict(d);
                if (test_dict.valid && test_dict.count > 0) {
                    dict_addr = d;
                    break;
                }
            }
            if (match && engine::mem::addr_valid(match)) {
                uint64_t dm = rpm<uint64_t>(match + doff);
                if (engine::mem::addr_valid(dm)) {
                    ManagedDictionary test_dict(dm);
                    if (test_dict.valid && test_dict.count > 0) {
                        dict_addr = dm;
                        break;
                    }
                }
            }
        }
        if (!dict_addr) {
            dict_addr = rpm<uint64_t>(base_game + offsets::Match_PlayerDict);
            if (!dict_addr && match) dict_addr = rpm<uint64_t>(match + offsets::Match_PlayerDict);
        }

        ManagedDictionary dict(dict_addr);

        bool match_active = false;
        if (dict.valid && dict.count > 0) {
            match_active = true; // Training targets or combat enemies exist!
        } else if (match && engine::mem::addr_valid(match)) {
            match_active = true;
        } else if (local_player && engine::mem::addr_valid(local_player)) {
            match_active = true;
        }
        g_in_match.store(match_active);

        if (print_diag) {
            logger::log_info("[SECTOR-2] MatchState -> BaseGame=0x%lx | Match=0x%lx | Local=0x%lx | Active=%s | Dict=0x%lx (Count: %d)",
                (unsigned long)base_game, (unsigned long)match, (unsigned long)local_player,
                match_active ? "ACTIVE" : "STANDBY", (unsigned long)dict_addr, dict.valid ? dict.count : 0);
        }

        if (!dict.valid || dict.count <= 0) {
            std::lock_guard<std::mutex> lk(g_players_mutex);
            g_players.clear();
            return;
        }

        // SECTOR 4: Camera Matrix Resolution (NEXOMODS 0x30 chain + fallbacks)
        uint64_t int_ptr = 0;
        uint64_t follow = 0;
        uint64_t cam = 0;
        if (local_player && engine::mem::addr_valid(local_player)) {
            g_local_player.store(local_player);

            // NEXOMODS verified chain: local_player + 0x30 -> FollowCamera + 0x30 -> Camera + 0x10 -> IntPtrCam
            follow = rpm<uint64_t>(local_player + offsets::Player_Camera); // 0x30
            if (follow && engine::mem::addr_valid(follow)) {
                cam = rpm<uint64_t>(follow + 0x30);
                if (cam && engine::mem::addr_valid(cam)) int_ptr = rpm<uint64_t>(cam + 0x10);
            }

            // Fallback chain: local_player + 0x690 -> Camera_Follow (0x28) -> Camera_IntPtr (0x10)
            if (!int_ptr) {
                follow = rpm<uint64_t>(local_player + offsets::Player_Camera_Alt); // 0x690
                if (follow && engine::mem::addr_valid(follow)) {
                    cam = rpm<uint64_t>(follow + offsets::Camera_Follow);
                    if (cam && engine::mem::addr_valid(cam)) int_ptr = rpm<uint64_t>(cam + offsets::Camera_IntPtr);
                }
            }

            // Fallback: local_player + 0x398 (GetPlayerMainCamera)
            if (!int_ptr) {
                cam = rpm<uint64_t>(local_player + 0x398);
                if (cam && engine::mem::addr_valid(cam)) int_ptr = rpm<uint64_t>(cam + 0x10);
            }
        }

        if (int_ptr && engine::mem::addr_valid(int_ptr)) {
            g_view_matrix = rpm<Matrix4x4>(int_ptr + offsets::Camera_Matrix); // 0x100
            if (std::abs(g_view_matrix.m11) < 0.0001f && std::abs(g_view_matrix.m22) < 0.0001f) {
                g_view_matrix = rpm<Matrix4x4>(int_ptr + 0xDC); // alt matrix offset
            }
        }

        if (print_diag && local_player) {
            logger::log_info("[SECTOR-3] Camera -> follow=0x%lx, cam=0x%lx, int_ptr=0x%lx, vm.m11=%.2f",
                (unsigned long)follow, (unsigned long)cam, (unsigned long)int_ptr, g_view_matrix.m11);
        }

        Vector3 local_pos = Vector3::Zero();
        if (local_player) {
            for (uintptr_t off : {0x648, 0x6A0, 0x6A8, 0x668}) {
                uint64_t ln = rpm<uint64_t>(local_player + off);
                if (engine::mem::addr_valid(ln)) {
                    Vector3 lp = get_node_position(ln);
                    if (lp != Vector3::Zero()) { local_pos = lp; break; }
                }
            }
        }

        // SECTOR 5: Entity Scanning & Screen Projection
        int num_entries = dict.get_count();
        int valid_entities_count = 0;

        for (int i = 0; i < num_entries; i++) {
            uint64_t enemy = dict.entry(i);
            if (!enemy || enemy < 0x100000 || (local_player && enemy == local_player)) continue;

            if (rpm<bool>(enemy + offsets::Player_IsDead)) continue;

            int hp = read_hp(enemy);
            if (hp <= 0) hp = 100; // Training dummy & bot fallback

            bool is_bot = rpm<bool>(enemy + offsets::Player_IsBot);

            auto avatar = rpm<uint64_t>(enemy + offsets::Player_Avatar);
            if (!avatar) avatar = rpm<uint64_t>(enemy + offsets::Player_Avatar_Alt);
            auto uma = avatar ? rpm<uint64_t>(avatar + offsets::Avatar_Uma) : 0;
            if (uma) {
                auto uma_data = rpm<uint64_t>(uma + offsets::Uma_Data);
                if (uma_data && rpm<bool>(uma_data + offsets::UmaData_Skip)) {
                    continue; // Skip confirmed teammate
                }
            }

            bool raw_visible = uma ? rpm<bool>(uma + offsets::Uma_Visible) : true;
            bool visible = raw_visible;
            {
                auto it = g_vis_stable.find(enemy);
                if (it == g_vis_stable.end()) {
                    g_vis_stable[enemy] = raw_visible;
                    g_vis_counter[enemy] = 0;
                } else if (it->second == raw_visible) {
                    g_vis_counter[enemy] = 0;
                } else {
                    g_vis_counter[enemy]++;
                    if (g_vis_counter[enemy] >= 2) {
                        g_vis_stable[enemy] = raw_visible;
                        g_vis_counter[enemy] = 0;
                        visible = raw_visible;
                    } else {
                        visible = it->second;
                    }
                }
            }

            // Head world & distance: probe NEXOMODS 0x648, standard 0x6A0, 0x6A8, 0x668
            uint64_t head_node = 0;
            Vector3 head_world = Vector3::Zero();
            for (uintptr_t off : {0x648, 0x6A0, 0x6A8, 0x6B0, 0x668}) {
                uint64_t hn = rpm<uint64_t>(enemy + off);
                if (engine::mem::addr_valid(hn)) {
                    Vector3 pos = get_node_position(hn);
                    if (pos != Vector3::Zero()) {
                        head_node = hn;
                        head_world = pos;
                        break;
                    }
                }
            }
            if (!head_node || head_world == Vector3::Zero()) continue;

            float dist = (local_pos != Vector3::Zero()) ? get_distance_meters(local_pos, head_world) : 25.0f;
            if (dist > cfg::esp::max_range) continue;

            Vector3 head_screen = world_to_screen(g_view_matrix, head_world, screen_w, screen_h);
            if (head_screen.x < 0.0f || head_screen.y < 0.0f ||
                head_screen.x > screen_w || head_screen.y > screen_h) {
                continue; // Behind camera or offscreen
            }

            // Foot world: probe NEXOMODS 0x670, standard 0x6E8, 0x6B8
            Vector3 foot_world = Vector3::Zero();
            for (uintptr_t off : {0x670, 0x6E8, 0x6B8, 0x6F0}) {
                uint64_t fn = rpm<uint64_t>(enemy + off);
                if (engine::mem::addr_valid(fn)) {
                    Vector3 fpos = get_node_position(fn);
                    if (fpos != Vector3::Zero()) {
                        foot_world = fpos;
                        break;
                    }
                }
            }
            if (foot_world == Vector3::Zero()) {
                bool is_knocked = rpm<bool>(enemy + offsets::Player_DeathInfo) || (hp <= 0);
                float model_h = is_knocked ? 0.90f : 1.72f;
                foot_world = head_world;
                foot_world.y -= model_h;
            }

            Vector3 toe_screen = world_to_screen(g_view_matrix, foot_world, screen_w, screen_h);
            if (toe_screen.x < 0.0f || toe_screen.y < 0.0f) {
                toe_screen = head_screen;
                toe_screen.y += (screen_h * 0.15f);
            }

            PlayerData pd;
            pd.addr       = enemy;
            pd.Head       = head_screen;
            pd.Toe        = toe_screen;
            pd.HeadWorld  = head_world;
            pd.isOnScreen = true;
            pd.isBehind   = false;
            pd.isVisible  = visible;
            pd.isBot      = is_bot;
            pd.distance   = dist;
            pd.curHP      = hp;
            pd.maxHP      = 200;

            if (cfg::aim::enabled && cfg::aim::target_bone == 1) {
                pd.Breast = bone_screen(enemy, offsets::Bone_Breast, screen_w, screen_h);
            }
            if (cfg::aim::enabled && cfg::aim::target_bone == 2) {
                pd.Hip = bone_screen(enemy, offsets::Bone_Hip, screen_w, screen_h);
            }

            tmp_players.push_back(pd);
            valid_entities_count++;

            if (print_diag && valid_entities_count <= 2) {
                logger::log_info("[SECTOR-5] Target[%d]: addr=0x%lx, hp=%d, dist=%.0fm, screen=(%.0f, %.0f)",
                    i, (unsigned long)enemy, hp, dist, head_screen.x, head_screen.y);
            }
        }

        if (print_diag && !tmp_players.empty()) {
            logger::log_success("[RADAR] Tracking %zu target(s) on-screen! P0 at (%.0f, %.0f) dist=%.0fm",
                tmp_players.size(), tmp_players[0].Head.x, tmp_players[0].Head.y, tmp_players[0].distance);
        }

        {
            std::lock_guard<std::mutex> lk(g_players_mutex);
            g_players = std::move(tmp_players);
            if (!g_players.empty()) {
                g_in_match.store(true);
            }
        }

        if (g_vis_stable.size() > 200) {
            g_vis_stable.clear();
            g_vis_counter.clear();
        }
    }

    /**
     * @brief High-frequency worker thread with randomized jitter to match ZqwClient timing model.
     */
    static void worker_loop() {
        prctl(PR_SET_NAME, "kworker/u16:1", 0, 0, 0);
        uint32_t seed = 0x9E3779B1u;
        while (g_worker_running.load(std::memory_order_relaxed)) {
            if (engine::mem::g_pid <= 0 || engine::mem::g_lib_base == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            }
            if (!g_in_match.load(std::memory_order_relaxed)) {
                update_cycle();
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
                continue;
            }
            update_cycle();
            seed = seed * 1664525u + 1013904223u;
            int jitter = (seed >> 24) & 7;
            std::this_thread::sleep_for(std::chrono::milliseconds(5 + jitter));
        }
    }

    void start_worker() {
        if (g_worker_running.exchange(true)) return;
        g_worker_thread = std::thread(worker_loop);
    }

    void stop_worker() {
        if (!g_worker_running.exchange(false)) return;
        try {
            if (g_worker_thread.joinable()) {
                g_worker_thread.join();
            }
        } catch (...) {
        }
        std::lock_guard<std::mutex> lk(g_players_mutex);
        g_players.clear();
    }

} // namespace engine::data

/**
 * @brief Global data namespace aliases aligned with ZqwClient usage.
 */
namespace data {
    std::vector<engine::data::PlayerData>& players = engine::data::g_players;
    std::mutex& players_mutex = engine::data::g_players_mutex;
    std::vector<engine::data::LootItem>& loots = engine::data::g_loots;
    std::mutex& loots_mutex = engine::data::g_loots_mutex;
    std::atomic<uint64_t>& localPlayerAddr = engine::data::g_local_player;
    std::atomic<bool>& inMatch = engine::data::g_in_match;
    engine::data::Matrix4x4& viewMatrix = engine::data::g_view_matrix;

    void update() {
        engine::data::update_cycle();
    }
    void thread_start() {
        engine::data::start_worker();
    }
    void thread_stop() {
        engine::data::stop_worker();
    }
    size_t player_count() {
        std::lock_guard<std::mutex> lk(engine::data::g_players_mutex);
        return engine::data::g_players.size();
    }
}
