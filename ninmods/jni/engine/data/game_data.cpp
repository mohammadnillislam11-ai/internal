#include <sys/prctl.h>
#include "game_data.hpp"
#include "offsets.hpp"
#include "memory.hpp"
#include "menu/config.hpp"
#include "display_metrics.h"
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
     * @brief Managed IL2CPP Dictionary wrapper matching ZqwClient stealth 1-syscall architecture.
     */
    class ManagedDictionary {
    public:
        uint64_t address{0};
        uint64_t entries_head{0};
        int      count{0};
        bool     valid{false};

        explicit ManagedDictionary(uint64_t addr) : address(addr) {
            if (!engine::mem::addr_valid(addr)) return;

            struct { uint64_t entries; uint64_t count_and_pad; } header{};
            if (!mem_read(addr + offsets::Dict_Entries, &header, sizeof(header))) return;

            if (!engine::mem::addr_valid(header.entries)) return;

            entries_head = header.entries + offsets::Dict_ArrayHeader;
            count        = static_cast<int>(header.count_and_pad & 0xFFFFFFFFu);
            valid        = (count > 0 && count < 0x1000);
        }

        inline int get_count() const { return count; }

        inline uint64_t entry(int i) const {
            if (!valid || i < 0 || i >= count) return 0;
            return rpm<uint64_t>(entries_head + (i * offsets::Dict_EntryStride) + offsets::Dict_EntryValue);
        }
    };



    /**
     * @brief One full entity scan cycle.
     */
    void update_cycle() {
        int pid = (engine::mem::g_pid > 0) ? engine::mem::g_pid : proc::pid;
        uintptr_t lib = (engine::mem::g_lib_base > 0) ? engine::mem::g_lib_base : proc::lib;
        if (pid <= 0 || lib == 0) return;

        float screen_w = static_cast<float>(display::width > 0 ? display::width : 2400);
        float screen_h = static_cast<float>(display::height > 0 ? display::height : 1080);

        std::vector<PlayerData> tmp_players;
        tmp_players.reserve(64);

        // Base Chain with dynamic multi-version fallback (Supports Standard & MAX)
        static uintptr_t s_working_facade_offset = 0;
        uint64_t base_game = 0;

        if (s_working_facade_offset != 0) {
            auto gf_cell = rpm<uint64_t>(lib + s_working_facade_offset);
            if (gf_cell) {
                auto gf_class = rpm<uint64_t>(gf_cell);
                if (gf_class) {
                    auto static_fields = rpm<uint64_t>(gf_class + offsets::GameFacade_P2);
                    if (static_fields) {
                        base_game = rpm<uint64_t>(static_fields + 0x8);
                    }
                }
            }
            if (!base_game) s_working_facade_offset = 0;
        }

        if (!base_game) {
            static const uintptr_t candidates[] = {
                offsets::GameFacade, // 0xA72E4E0 (Free Fire Standard)
                0xB4C41A0,           // Free Fire MAX (NEXOMODS)
                0xA986B7C,           // Free Fire MAX alternative
                0xA7384E0,
                0xA8E44E0
            };
            for (uintptr_t cand : candidates) {
                auto gf_cell = rpm<uint64_t>(lib + cand);
                if (!gf_cell) continue;
                auto gf_class = rpm<uint64_t>(gf_cell);
                if (!gf_class) continue;
                auto static_fields = rpm<uint64_t>(gf_class + offsets::GameFacade_P2);
                if (!static_fields) continue;
                auto bg = rpm<uint64_t>(static_fields + 0x8);
                if (bg && engine::mem::addr_valid(bg)) {
                    s_working_facade_offset = cand;
                    base_game = bg;
                    break;
                }
            }
        }

        if (!base_game) return;

        auto match = rpm<uint64_t>(base_game + offsets::BaseGame_Match);
        auto local_player = match ? rpm<uint64_t>(match + offsets::Match_LocalPlayer) : 0;

        bool match_active = false;
        if (match && engine::mem::addr_valid(match)) {
            int mir = rpm<int>(match + offsets::MatchIsRunning);
            if (mir == 1 || mir == 2 || mir == 3 || rpm<bool>(base_game + offsets::MatchIsRunning)) {
                match_active = true;
            }
        }
        if (!match_active && (match && engine::mem::addr_valid(match))) {
            match_active = true;
        }
        if (!match_active && (local_player && engine::mem::addr_valid(local_player))) {
            match_active = true;
        }
        g_in_match.store(match_active);

        if (!match_active) {
            g_local_player.store(0);
            g_vis_stable.clear();
            g_vis_counter.clear();
            {
                std::lock_guard<std::mutex> lk(g_players_mutex);
                g_players.clear();
            }
            {
                std::lock_guard<std::mutex> lk(g_loots_mutex);
                g_loots.clear();
            }
            return;
        }

        if (!match || !local_player) {
            g_local_player.store(0);
            std::lock_guard<std::mutex> lk(g_players_mutex);
            g_players.clear();
            return;
        }
        g_local_player.store(local_player);

        // Camera Matrix
        uint64_t follow = rpm<uint64_t>(local_player + offsets::Player_Camera);
        uint64_t cam = follow ? rpm<uint64_t>(follow + offsets::Camera_Follow) : 0;
        uint64_t int_ptr = cam ? rpm<uint64_t>(cam + offsets::Camera_IntPtr) : 0;

        if (!int_ptr) {
            follow = rpm<uint64_t>(local_player + 0x690);
            cam = follow ? rpm<uint64_t>(follow + 0x30) : 0;
            int_ptr = cam ? rpm<uint64_t>(cam + 0x10) : 0;
        }

        if (!int_ptr) {
            std::lock_guard<std::mutex> lk(g_players_mutex);
            g_players.clear();
            return;
        }

        g_view_matrix = rpm<Matrix4x4>(int_ptr + offsets::Camera_Matrix);

        Vector3 local_pos = Vector3::Zero();
        uint64_t local_head_node = rpm<uint64_t>(local_player + offsets::Bone_Head);
        if (local_head_node) local_pos = get_node_position(local_head_node);

        // No loot scan needed (ESP Loot removed)

        // Player Dictionary Scan
        uint64_t dict_addr = rpm<uint64_t>(base_game + offsets::Match_PlayerDict);
        if (!dict_addr) {
            std::lock_guard<std::mutex> lk(g_players_mutex);
            g_players.clear();
            return;
        }

        ManagedDictionary dict(dict_addr);
        if (!dict.valid) {
            std::lock_guard<std::mutex> lk(g_players_mutex);
            g_players.clear();
            return;
        }

        int num_entries = dict.get_count();
        for (int i = 0; i < num_entries; i++) {
            uint64_t enemy = dict.entry(i);
            if (!enemy || enemy < 0x100000 || enemy == local_player) continue;

            if (rpm<bool>(enemy + offsets::Player_IsDead)) continue;

            int hp = read_hp(enemy);
            if (hp <= 0) continue;

            bool is_bot = rpm<bool>(enemy + offsets::Player_IsBot);

            auto avatar = rpm<uint64_t>(enemy + offsets::Player_Avatar);
            if (!avatar) avatar = rpm<uint64_t>(enemy + 0x770);
            auto uma = avatar ? rpm<uint64_t>(avatar + offsets::Avatar_Uma) : 0;
            if (!uma) continue;

            // Teammate Filter (Verified across STORM_MODS 0x81, NEXOMODSROOT 0x81, and ZqwClient 0x91)
            auto uma_data = rpm<uint64_t>(uma + offsets::Uma_Data);
            if (uma_data) {
                if (rpm<bool>(uma_data + offsets::UmaData_isTeam) ||
                    rpm<bool>(uma_data + offsets::UmaData_Skip)) {
                    continue; // Teammate: Skip so tracer lines appear ONLY for enemies!
                }
            }

            bool raw_visible = rpm<bool>(uma + offsets::Uma_Visible);

            // Visibility Debouncing
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

            // Head world & distance
            uint64_t head_node = rpm<uint64_t>(enemy + offsets::Bone_Head);
            if (!head_node) continue;
            Vector3 head_world = get_node_position(head_node);
            if (head_world == Vector3::Zero()) continue;

            float dist = get_distance_meters(local_pos, head_world);
            if (dist > cfg::esp::max_range) continue;

            Vector3 head_screen = world_to_screen(g_view_matrix, head_world, screen_w, screen_h);
            if (head_screen.x < 0.0f || head_screen.y < 0.0f ||
                head_screen.x > screen_w || head_screen.y > screen_h) {
                continue; // NEXOMODS: Skip off-screen / behind targets
            }

            bool is_knocked = rpm<bool>(enemy + offsets::Player_DeathInfo) || (hp <= 0);
            float model_h = is_knocked ? 0.90f : 1.72f;

            Vector3 foot_world = head_world;
            foot_world.y -= model_h;

            Vector3 toe_screen = world_to_screen(g_view_matrix, foot_world, screen_w, screen_h);
            if (toe_screen.x < 0.0f || toe_screen.y < 0.0f) continue;

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
        }

        if (g_vis_stable.size() > 200) {
            g_vis_stable.clear();
            g_vis_counter.clear();
        }

        {
            std::lock_guard<std::mutex> lk(g_players_mutex);
            g_players.swap(tmp_players);
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
