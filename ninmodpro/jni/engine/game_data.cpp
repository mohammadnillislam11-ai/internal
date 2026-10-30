#include "game_data.hpp"
#include "offsets.hpp"
#include "memory.hpp"
#include "menu/config.hpp"
#include "display_metrics.h"
#include "oxorany.h"
#include "glm.hpp"
#include "MonoString.h"
#include <thread>
#include <chrono>
#include <unordered_map>

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

    // ========================================================================
    // MATH HELPERS (Connected to GLM Mathematics Suite)
    // ========================================================================
    float get_distance_meters(const Vector3& a, const Vector3& b) {
        glm::vec3 v1(a.x, a.y, a.z);
        glm::vec3 v2(b.x, b.y, b.z);
        return glm::distance(v1, v2);
    }

    Vector3 world_to_screen(const Matrix4x4& vm, const Vector3& pos, float screen_w, float screen_h) {
        float w = (pos.x * vm.m14) + (pos.y * vm.m24) + (pos.z * vm.m34) + vm.m44;
        if (w >= 0.001f) {
            float inv_w = 1.0f / w;
            float cx = screen_w * 0.5f;
            float cy = screen_h * 0.5f;
            float x = (pos.x * vm.m11) + (pos.y * vm.m21) + (pos.z * vm.m31) + vm.m41;
            float y = (pos.x * vm.m12) + (pos.y * vm.m22) + (pos.z * vm.m32) + vm.m42;
            return Vector3(cx + cx * x * inv_w, cy - cy * y * inv_w, 0.0f);
        }
        return Vector3(-1.0f, -1.0f, -1.0f);
    }

    // ========================================================================
    // UNITY TRANSFORM POSITION HIERARCHY
    // ========================================================================
    static Vector3 get_transform_position(uint64_t transform) {
        auto transform_obj = mem::read<uint64_t>(transform + 0x10);
        if (transform_obj == 0) return Vector3::Zero();

        auto matrix_ptr = mem::read<uint64_t>(transform_obj + 0x38);
        if (!matrix_ptr) return Vector3::Zero();

        uint64_t matrices = mem::read<uint64_t>(matrix_ptr + 0x18);
        uint64_t indices  = mem::read<uint64_t>(matrix_ptr + 0x20);
        if (!matrices || !indices) return Vector3::Zero();

        auto index_val = mem::read<uint64_t>(transform_obj + 0x40);

        auto result = mem::read<Vector3>(matrices + (index_val * 0x30));
        auto transform_index = mem::read<int>(indices + (index_val * 4));

        for (int tries = 0; transform_index >= 0 && tries < 50; tries++) {
            uint64_t mat_addr = matrices + (transform_index * 0x30);
            auto t = mem::read<TransformMatrix>(mat_addr);

            float rx = t.rotation.x, ry = t.rotation.y, rz = t.rotation.z, rw = t.rotation.w;
            float sx = result.x * t.scale.x;
            float sy = result.y * t.scale.y;
            float sz = result.z * t.scale.z;

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

            transform_index = mem::read<int>(indices + (transform_index * 4));
        }

        return result;
    }

    Vector3 get_node_position(uint64_t node) {
        auto trans = mem::read<uint64_t>(node + 0x10);
        if (!trans) return Vector3::Zero();
        return get_transform_position(trans);
    }

    static Vector3 bone_screen(uint64_t player, uintptr_t offset, float sw, float sh) {
        uint64_t node = mem::read<uint64_t>(player + offset);
        if (!node) return Vector3::Zero();
        Vector3 world = get_node_position(node);
        if (world == Vector3::Zero()) return Vector3::Zero();
        return world_to_screen(g_view_matrix, world, sw, sh);
    }

    // ========================================================================
    // READ ENTITY PROPERTIES
    // ========================================================================
    static int read_hp(uint64_t player) {
        auto pool = mem::read<uint64_t>(player + offsets::Player_HP);
        if (!pool) return 0;
        auto list = mem::read<uint64_t>(pool + offsets::HP_List);
        if (!list) return 0;
        auto elem = mem::read<uint64_t>(list + offsets::HP_Entry);
        if (!elem) return 0;
        return mem::read<int>(elem + offsets::HP_Value);
    }

    static void read_name(uint64_t player, char* buf, size_t buflen) {
        buf[0] = '\0';
        auto str_ptr = mem::read<uint64_t>(player + offsets::Player_Name);
        if (!str_ptr || str_ptr < 0x10000) return;

        int len = mem::read<int>(str_ptr + 0x10);
        if (len <= 0 || len > 64) return;

        uint16_t utf16[65];
        if (len > 64) len = 64;
        if (!mem::read_raw(str_ptr + 0x14, utf16, len * sizeof(uint16_t))) return;

        int n = 0;
        for (int i = 0; i < len && n < static_cast<int>(buflen) - 1; i++) {
            uint16_t c = utf16[i];
            if (c >= 0x20 && c <= 0x7E) {
                buf[n++] = static_cast<char>(c);
            }
        }
        buf[n] = '\0';
    }

    static int read_weapon(uint64_t player) {
        auto w = mem::read<uint64_t>(player + offsets::Player_Weapon);
        if (!w) return -1;
        auto wd = mem::read<uint64_t>(w + offsets::Weapon_Data);
        if (!wd) return -1;
        return mem::read<int>(wd + 0x64);
    }

    // ========================================================================
    // SCAN DROPPED LOOT
    // ========================================================================
    static void scan_loot(uint64_t base_game, const Vector3& local_pos, float sw, float sh) {
        std::vector<LootItem> list;

        uint64_t match = mem::read<uint64_t>(base_game + offsets::BaseGame_Match);
        if (!match) return;

        uint64_t meta_mgr = mem::read<uint64_t>(match + offsets::Match_MetaManager);
        if (!meta_mgr) return;

        uint64_t dict_ptr = mem::read<uint64_t>(meta_mgr + offsets::MetaManager_DropObjects);
        if (!dict_ptr) return;

        uint64_t entries = mem::read<uint64_t>(dict_ptr + offsets::Dict_Entries);
        int count = mem::read<int>(dict_ptr + offsets::Dict_Count);
        if (!entries || count <= 0 || count > 500) return;

        uint64_t base = entries + offsets::Dict_ArrayHeader;
        for (int i = 0; i < count; i++) {
            uint64_t e = base + (i * offsets::Dict_EntryStride);
            uint64_t drop_obj = mem::read<uint64_t>(e + offsets::Dict_EntryValue);
            if (!drop_obj || drop_obj < 0x100000) continue;

            bool landed = mem::read<bool>(drop_obj + offsets::DropObject_Landed);
            if (!landed) continue;

            Vector3 world = mem::read<Vector3>(drop_obj + offsets::DropObject_Position);
            if (world == Vector3::Zero()) continue;

            float dist = get_distance_meters(local_pos, world);
            if (dist > cfg::esp::loot_range) continue;

            Vector3 screen = world_to_screen(g_view_matrix, world, sw, sh);
            if (screen.x < 0) continue;

            LootItem item;
            item.addr      = drop_obj;
            item.guid      = mem::read<uint32_t>(drop_obj + offsets::DropObject_Guid);
            item.resID     = mem::read<uint32_t>(drop_obj + offsets::DropObject_ResID);
            item.worldPos  = world;
            item.screenPos = screen;
            item.distance  = dist;
            list.push_back(item);
        }

        std::lock_guard<std::mutex> lk(g_loots_mutex);
        g_loots = std::move(list);
    }

    // ========================================================================
    // UPDATE CYCLE
    // ========================================================================
    void update_cycle() {
        // Zero-Automation & Strict Security:
        // Do not touch process memory if neither ESP nor Aimbot is enabled.
        if (!cfg::esp::enabled && !cfg::aim::enabled) {
            {
                std::lock_guard<std::mutex> lk(g_players_mutex);
                if (!g_players.empty()) g_players.clear();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return;
        }

        if (mem::g_pid <= 0 || mem::g_lib_base == 0) return;

        float screen_w = static_cast<float>(display::width > 0 ? display::width : 2400);
        float screen_h = static_cast<float>(display::height > 0 ? display::height : 1080);

        std::vector<PlayerData> tmp_players;
        tmp_players.reserve(64);

        // GameFacade -> BaseGame
        auto gf_cell = mem::read<uint64_t>(mem::g_lib_base + oxorany(offsets::GameFacade));
        if (!gf_cell) return;

        auto gf_class = mem::read<uint64_t>(gf_cell);
        if (!gf_class) return;

        auto static_fields = mem::read<uint64_t>(gf_class + offsets::GameFacade_P2);
        if (!static_fields) return;

        auto base_game = mem::read<uint64_t>(static_fields + 0x8);
        if (!base_game) return;

        bool match_active = mem::read<bool>(base_game + offsets::MatchIsRunning);
        g_in_match.store(match_active);

        if (!match_active) {
            g_local_player.store(0);
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

        auto match = mem::read<uint64_t>(base_game + offsets::BaseGame_Match);
        if (!match) return;

        auto local_player = mem::read<uint64_t>(match + offsets::Match_LocalPlayer);
        g_local_player.store(local_player ? local_player : 0);
        if (!local_player) return;

        // Camera Matrix
        uint64_t follow = mem::read<uint64_t>(local_player + offsets::Player_Camera);
        uint64_t cam = follow ? mem::read<uint64_t>(follow + offsets::Camera_Follow) : 0;
        uint64_t int_ptr = cam ? mem::read<uint64_t>(cam + offsets::Camera_IntPtr) : 0;

        if (!int_ptr) {
            follow = mem::read<uint64_t>(local_player + 0x690);
            cam = follow ? mem::read<uint64_t>(follow + 0x30) : 0;
            int_ptr = cam ? mem::read<uint64_t>(cam + 0x10) : 0;
        }

        if (!int_ptr) return;

        g_view_matrix = mem::read<Matrix4x4>(int_ptr + offsets::Camera_Matrix);

        Vector3 local_pos = Vector3::Zero();
        uint64_t local_head_node = mem::read<uint64_t>(local_player + offsets::Bone_Head);
        if (local_head_node) local_pos = get_node_position(local_head_node);

        // Loot scan
        if (cfg::esp::loot) {
            scan_loot(base_game, local_pos, screen_w, screen_h);
        } else {
            std::lock_guard<std::mutex> lk(g_loots_mutex);
            g_loots.clear();
        }

        // Player dictionary scan
        uint64_t dict_addr = mem::read<uint64_t>(base_game + offsets::Match_PlayerDict);
        if (!dict_addr) return;

        uint64_t entries_ptr = mem::read<uint64_t>(dict_addr + offsets::Dict_Entries);
        int dict_count = mem::read<int>(dict_addr + offsets::Dict_Count);
        if (!entries_ptr || dict_count <= 0 || dict_count > 120) return;

        uint64_t entries_head = entries_ptr + offsets::Dict_ArrayHeader;

        for (int i = 0; i < dict_count; i++) {
            uint64_t entry_addr = entries_head + (i * offsets::Dict_EntryStride);
            uint64_t enemy = mem::read<uint64_t>(entry_addr + offsets::Dict_EntryValue);

            if (!enemy || enemy < 0x100000 || enemy == local_player) continue;
            if (mem::read<bool>(enemy + offsets::Player_IsDead)) continue;

            int hp = read_hp(enemy);
            if (hp <= 0) continue;

            bool is_bot = mem::read<bool>(enemy + offsets::Player_IsBot);

            auto avatar = mem::read<uint64_t>(enemy + offsets::Player_Avatar);
            auto uma = avatar ? mem::read<uint64_t>(avatar + offsets::Avatar_Uma) : 0;
            if (!uma) continue;

            // Teammate Filter (UmaData_Skip from reference engine)
            auto uma_data = mem::read<uint64_t>(uma + offsets::Uma_Data);
            if (uma_data && mem::read<bool>(uma_data + offsets::UmaData_Skip)) continue;

            bool raw_visible = mem::read<bool>(uma + offsets::Uma_Visible);

            // Debounce visibility
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
            uint64_t head_node = mem::read<uint64_t>(enemy + offsets::Bone_Head);
            if (!head_node) continue;
            Vector3 head_world = get_node_position(head_node);
            if (head_world == Vector3::Zero()) continue;

            float dist = get_distance_meters(local_pos, head_world);
            if (dist > cfg::esp::max_range) continue;

            Vector3 head_screen = world_to_screen(g_view_matrix, head_world, screen_w, screen_h);
            if (head_screen.x < 0) continue;

            Vector3 toe_screen = bone_screen(enemy, offsets::Bone_LeftToe, screen_w, screen_h);
            if (toe_screen.x < 0 || toe_screen.y < 0) continue;

            PlayerData pd;
            read_name(enemy, pd.name, sizeof(pd.name));
            pd.weaponType = read_weapon(enemy);
            if (!pd.name[0] && is_bot) snprintf(pd.name, sizeof(pd.name), "BOT");

            pd.addr       = enemy;
            pd.Head       = head_screen;
            pd.Toe        = toe_screen;
            pd.isVisible  = visible;
            pd.isBot      = is_bot;
            pd.distance   = dist;
            pd.curHP      = hp;
            pd.maxHP      = 200;
            pd.curArmor   = mem::read<int>(enemy + offsets::Player_Armor);
            pd.isFiring   = mem::read<bool>(enemy + offsets::Player_IsFiring);
            pd.isGrounded = mem::read<bool>(enemy + offsets::Player_IsGrounded);

            Vector3 vel = mem::read<Vector3>(enemy + offsets::Player_Velocity);
            pd.speed = sqrtf(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z);

            // Skeleton bones
            pd.Hip       = bone_screen(enemy, offsets::Bone_Hip, screen_w, screen_h);
            pd.Breast    = bone_screen(enemy, offsets::Bone_Breast, screen_w, screen_h);
            pd.Root      = bone_screen(enemy, offsets::Bone_Root, screen_w, screen_h);
            pd.LShoulder = bone_screen(enemy, offsets::Bone_LeftShoulder, screen_w, screen_h);
            pd.RShoulder = bone_screen(enemy, offsets::Bone_RightShoulder, screen_w, screen_h);
            pd.LAnkle    = bone_screen(enemy, offsets::Bone_LeftAnkle, screen_w, screen_h);
            pd.RAnkle    = bone_screen(enemy, offsets::Bone_RightAnkle, screen_w, screen_h);
            pd.LToe      = toe_screen;
            pd.RToe      = bone_screen(enemy, offsets::Bone_RightToe, screen_w, screen_h);
            pd.LForeArm  = bone_screen(enemy, offsets::Bone_LeftForeArm, screen_w, screen_h);
            pd.RForeArm  = bone_screen(enemy, offsets::Bone_RightForeArm, screen_w, screen_h);
            pd.LHand     = bone_screen(enemy, offsets::Bone_LeftHand, screen_w, screen_h);
            pd.RHand     = bone_screen(enemy, offsets::Bone_RightHand, screen_w, screen_h);

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

    // ========================================================================
    // WORKER THREAD LIFECYCLE
    // ========================================================================
    static void worker_loop() {
        while (g_worker_running.load(std::memory_order_relaxed)) {
            update_cycle();
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }

    void start_worker() {
        if (g_worker_running.exchange(true)) return;
        g_worker_thread = std::thread(worker_loop);
    }

    void stop_worker() {
        if (!g_worker_running.exchange(false)) return;
        if (g_worker_thread.joinable()) g_worker_thread.join();

        std::lock_guard<std::mutex> lk(g_players_mutex);
        g_players.clear();
        g_in_match.store(false);
        g_local_player.store(0);
    }

} // namespace engine::data
