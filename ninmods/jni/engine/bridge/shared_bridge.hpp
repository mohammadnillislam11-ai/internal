#pragma once
#include <cstdint>
#include <cstring>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <algorithm>
#include "engine/data/game_data.hpp"

namespace engine::bridge {

    constexpr const char* BRIDGE_PATH = "/data/local/tmp/ninmods_bridge.dat";
    constexpr uint32_t BRIDGE_MAGIC = 0x4E494E31; // "NIN1"
    constexpr size_t MAX_BRIDGE_PLAYERS = 64;

    struct BridgeControl {
        bool  aim_enabled;
        int   target_bone;
        float aim_fov;
        float aim_dist;
        float aim_smooth;
        bool  aim_silent;
        bool  aim_nobita;
        bool  speed_hack;
        bool  fast_switch;
        float max_range;
    };

    struct BridgeData {
        uint32_t magic;
        uint64_t heartbeat_ms;
        int32_t  pid;
        uint64_t lib_base;
        bool     in_match;
        uint64_t local_player;
        data::Matrix4x4 view_matrix;
        uint32_t player_count;
        data::PlayerData players[MAX_BRIDGE_PLAYERS];

        // Controls synchronized from UI to Daemon
        BridgeControl ui_cfg;
    };

    inline uint64_t now_ms() {
        struct timeval tv{};
        gettimeofday(&tv, nullptr);
        return static_cast<uint64_t>(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
    }

} // namespace engine::bridge
