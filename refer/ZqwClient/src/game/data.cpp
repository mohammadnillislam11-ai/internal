#include "data.hpp"
#include "Offsets.h"
#include "../other/utils.hpp"
#include "../other/memory.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_map>

namespace data {

    std::vector<PlayerData>  players;
    std::mutex               players_mutex;
    std::atomic<uint64_t>    localPlayerAddr{0};
    std::atomic<bool>        inMatch{false};
    matrix                   viewMatrix{};

    static std::atomic<bool> g_run{false};
    static std::thread       g_thread;

    static std::unordered_map<uint64_t, int>  g_visCounter;
    static std::unordered_map<uint64_t, bool> g_visStable;

    /**
     * @brief Reads the current health points of a player entity.
     * @param p Memory address of the player entity.
     * @return Current HP value, or 0 if unresolvable.
     */
    static int readHP(uint64_t p) {
        auto pool  = rpm<uint64_t>(p + Offsets::Player_HP);
        if (!pool) return 0;
        auto datas = rpm<uint64_t>(pool + Offsets::HP_List);
        if (!datas) return 0;
        auto elem  = rpm<uint64_t>(datas + Offsets::HP_Entry);
        if (!elem) return 0;
        return rpm<int>(elem + Offsets::HP_Value);
    }

    /**
     * @brief Reads the UTF-16 nickname string of a player entity.
     * @param p Memory address of the player entity.
     * @param buf Output buffer for ASCII/UTF-8 string.
     * @param buflen Buffer length in bytes.
     * @return Number of characters parsed into buf.
     */
    static int readName(uint64_t p, char* buf, size_t buflen) {
        buf[0] = 0;
        auto sp = rpm<uint64_t>(p + Offsets::Player_Name);
        if (!sp || sp < 0x10000) return 0;
        int len = rpm<int>(sp + 0x10);
        if (len <= 0 || len > 64) return 0;
        uint16_t utf16[65];
        if (len > 64) len = 64;
        if (!mem_read(sp + 0x14, utf16, len * 2)) return 0;
        int n = 0;
        for (int i = 0; i < len && n < (int)buflen - 1; i++) {
            uint16_t c = utf16[i];
            if (c < 0x20 || c > 0x7E) continue;
            buf[n++] = (char)c;
        }
        buf[n] = 0;
        return n;
    }

    /**
     * @brief Reads the weapon identifier currently equipped by a player.
     * @param p Memory address of the player entity.
     * @return Weapon type integer ID, or -1 on failure.
     */
    static int readWeapon(uint64_t p) {
        auto w = rpm<uint64_t>(p + Offsets::Player_Weapon);
        if (!w) return -1;
        auto wd = rpm<uint64_t>(w + Offsets::Weapon_Data);
        if (!wd) return -1;
        return rpm<int>(wd + 0x64);
    }

    /**
     * @brief Transforms an entity bone node coordinate to 2D screen space.
     * @param player Address of the player entity.
     * @param offset Pointer offset to the bone transform node.
     * @return Screen-space Vector3 coordinate.
     */
    static Vector3 boneScreen(uint64_t player, uintptr_t offset) {
        uint64_t node = rpm<uint64_t>(player + offset);
        if (!node) return Vector3::Zero();
        Vector3 w = GetNodePosition(node);
        if (w == Vector3::Zero()) return Vector3::Zero();
        return WorldToScreenPoint(viewMatrix, w);
    }

    /**
     * @brief Core entity memory reader loop. Reads game matrices and active players.
     * Gated strictly by cfg::esp::enabled to ensure 100% manual control from menu.
     */
    void update() {
        if (!cfg::esp::enabled) {
            if (!players.empty()) {
                std::lock_guard<std::mutex> lk(players_mutex);
                players.clear();
            }
            return;
        }

        if (proc::pid <= 0 || proc::lib == 0) return;

        std::vector<PlayerData> tmp;
        tmp.reserve(64);

        // Base chain
        auto GfCell = rpm<uint64_t>(proc::lib + oxorany(Offsets::GameFacade));
        if (!GfCell) return;
        auto GfClass = rpm<uint64_t>(GfCell);
        if (!GfClass) return;

        auto StaticFields = rpm<uint64_t>(GfClass + Offsets::GameFacade_P2);
        if (!StaticFields) return;

        auto BaseGame = rpm<uint64_t>(StaticFields + 0x8);
        if (!BaseGame) return;

        bool matchActive = rpm<bool>(BaseGame + Offsets::MatchIsRunning);
        inMatch.store(matchActive);

        if (!matchActive) {
            localPlayerAddr.store(0);
            {
                std::lock_guard<std::mutex> lk(players_mutex);
                players.clear();
            }
            return;
        }

        auto m_Match = rpm<uint64_t>(BaseGame + Offsets::BaseGame_Match);
        if (!m_Match) return;

        auto localPlayer = rpm<uint64_t>(m_Match + Offsets::Match_LocalPlayer);

        // Simpan local player
        localPlayerAddr.store(localPlayer ? localPlayer : 0);

        if (!localPlayer) return;

        // ── Camera chain ──
        uint64_t Follow = 0, Cam = 0, IntPtr = 0;

        Follow = rpm<uint64_t>(localPlayer + Offsets::Player_Camera);
        if (Follow) Cam = rpm<uint64_t>(Follow + Offsets::Camera_Follow);
        if (Cam)    IntPtr = rpm<uint64_t>(Cam + Offsets::Camera_IntPtr);

        if (!IntPtr) {
            // Fallback offset alternatif
            Follow = rpm<uint64_t>(localPlayer + 0x690);
            if (Follow) Cam = rpm<uint64_t>(Follow + 0x30);
            if (Cam)    IntPtr = rpm<uint64_t>(Cam + 0x10);
        }

        if (!IntPtr) return;

        viewMatrix = rpm<matrix>(IntPtr + Offsets::Camera_Matrix);

        // ── Local position ──
        Vector3 localPos = Vector3::Zero();
        uint64_t headNode = rpm<uint64_t>(localPlayer + Offsets::Bone_Head);
        if (headNode) localPos = GetNodePosition(headNode);

        // ── PLAYERS ──
        uint64_t dictAddr = rpm<uint64_t>(BaseGame + Offsets::Match_PlayerDict);
        if (!dictAddr) return;

        MonoDictionary dict(dictAddr);
        if (!dict.valid) return;

        int nv = dict.getNumValues();
        for (int i = 0; i < nv; i++) {
            uint64_t enemy = dict.entry(i);
            if (!enemy || enemy < 0x100000000 || enemy == localPlayer) continue;

            if (rpm<bool>(enemy + Offsets::Player_IsDead)) continue;
            bool isBot = rpm<bool>(enemy + Offsets::Player_IsBot);

            int hp = readHP(enemy);
            if (hp <= 0) continue;

            auto avatarMgr = rpm<uint64_t>(enemy + Offsets::Player_Avatar);
            if (!avatarMgr) continue;
            auto uma = rpm<uint64_t>(avatarMgr + Offsets::Avatar_Uma);
            if (!uma) continue;

            bool rawVisible = rpm<bool>(uma + Offsets::Uma_Visible);

            // Debounce visible
            bool visible;
            {
                auto it = g_visStable.find(enemy);
                if (it == g_visStable.end()) {
                    g_visStable[enemy] = rawVisible;
                    g_visCounter[enemy] = 0;
                    visible = rawVisible;
                } else if (it->second == rawVisible) {
                    g_visCounter[enemy] = 0;
                    visible = rawVisible;
                } else {
                    g_visCounter[enemy]++;
                    if (g_visCounter[enemy] >= 2) {
                        g_visStable[enemy] = rawVisible;
                        g_visCounter[enemy] = 0;
                        visible = rawVisible;
                    } else {
                        visible = it->second;
                    }
                }
            }

            // Team
            auto umaData = rpm<uint64_t>(uma + Offsets::Uma_Data);
            if (umaData && rpm<bool>(umaData + Offsets::UmaData_Skip)) continue;

            // Head world
            uint64_t hNode = rpm<uint64_t>(enemy + Offsets::Bone_Head);
            if (!hNode) continue;
            Vector3 headWorld = GetNodePosition(hNode);
            if (headWorld == Vector3::Zero()) continue;

            float dist = calculate_distance(localPos, headWorld);
            if (dist > cfg::esp::max_range) continue;
            if (!visible && dist < 200.f) continue;

            Vector3 headScreen = WorldToScreenPoint(viewMatrix, headWorld);
            if (headScreen.x < 0) continue;

            Vector3 toeScreen = boneScreen(enemy, Offsets::Bone_LeftToe);
            if (toeScreen.x < 0 || toeScreen.y < 0) continue;

            // Build
            PlayerData pd;
            readName(enemy, pd.name, sizeof(pd.name));
            pd.weaponType = readWeapon(enemy);
            if (!pd.name[0] && isBot) snprintf(pd.name, sizeof(pd.name), "BOT");

            pd.addr       = enemy;
            pd.Head       = headScreen;
            pd.Toe        = toeScreen;
            pd.isVisible  = visible;
            pd.isBot      = isBot;
            pd.distance   = dist;
            pd.curHP      = hp;
            pd.maxHP      = 200;
            pd.curArmor   = rpm<int>(enemy + Offsets::Player_Armor);
            pd.isFiring   = rpm<bool>(enemy + Offsets::Player_IsFiring);
            pd.isGrounded = rpm<bool>(enemy + Offsets::Player_IsGrounded);

            Vector3 vel = rpm<Vector3>(enemy + Offsets::Player_Velocity);
            pd.speed = sqrtf(vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);

            // Bones
            pd.Hip       = boneScreen(enemy, Offsets::Bone_Hip);
            pd.Breast    = boneScreen(enemy, Offsets::Bone_Breast);
            pd.Root      = boneScreen(enemy, Offsets::Bone_Root);
            pd.LShoulder = boneScreen(enemy, Offsets::Bone_LeftShoulder);
            pd.RShoulder = boneScreen(enemy, Offsets::Bone_RightShoulder);
            pd.LAnkle    = boneScreen(enemy, Offsets::Bone_LeftAnkle);
            pd.RAnkle    = boneScreen(enemy, Offsets::Bone_RightAnkle);
            pd.LToe      = toeScreen;
            pd.RToe      = boneScreen(enemy, Offsets::Bone_RightToe);
            pd.LForeArm  = boneScreen(enemy, Offsets::Bone_LeftForeArm);
            pd.RForeArm  = boneScreen(enemy, Offsets::Bone_RightForeArm);
            pd.LHand     = boneScreen(enemy, Offsets::Bone_LeftHand);
            pd.RHand     = boneScreen(enemy, Offsets::Bone_RightHand);

            tmp.push_back(pd);
        }

        if (g_visStable.size() > 200) {
            g_visStable.clear();
            g_visCounter.clear();
        }

        {
            std::lock_guard<std::mutex> lk(players_mutex);
            players.swap(tmp);
        }
    }

    /**
     * @brief Background thread worker routine with randomized sleep jitter.
     */
    static void loop() {
        uint32_t seed = 0x9E3779B1u;
        while (g_run.load(std::memory_order_relaxed)) {
            update();
            seed = seed * 1664525u + 1013904223u;
            int jitter = (seed >> 24) & 7;
            std::this_thread::sleep_for(std::chrono::milliseconds(5 + jitter));
        }
    }

    void thread_start() {
        if (g_run.exchange(true)) return;
        g_thread = std::thread(loop);
    }

    void thread_stop() {
        if (!g_run.exchange(false)) return;
        if (g_thread.joinable()) g_thread.join();
        std::lock_guard<std::mutex> lk(players_mutex);
        players.clear();
    }
}