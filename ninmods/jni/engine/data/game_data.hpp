#pragma once
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <cmath>
#include "Vector3.hpp"
#include "Quaternion.h"

namespace engine::data {

    struct Vector4 {
        float x{0.0f}, y{0.0f}, z{0.0f}, w{0.0f};
    };

    struct Matrix4x4 {
        float m11, m12, m13, m14;
        float m21, m22, m23, m24;
        float m31, m32, m33, m34;
        float m41, m42, m43, m44;
    };

    struct TransformMatrix {
        Vector4 position;
        Quaternion rotation;
        Vector4 scale;
    };

    struct PlayerData {
        uint64_t addr;

        // Screen Positions
        Vector3  Head;
        Vector3  Toe;
        Vector3  HeadWorld;

        // Bone Hierarchy
        Vector3  Hip;
        Vector3  Breast;
        Vector3  Root;
        Vector3  LShoulder;
        Vector3  RShoulder;
        Vector3  LAnkle;
        Vector3  RAnkle;
        Vector3  LToe;
        Vector3  RToe;
        Vector3  LForeArm;
        Vector3  RForeArm;
        Vector3  LHand;
        Vector3  RHand;

        // Meta & Status
        bool     isOnScreen;
        bool     isBehind;
        bool     isVisible;
        bool     isBot;
        float    distance;
        int      curHP;
        int      maxHP;
        int      curArmor;
        bool     isFiring;
        bool     isGrounded;
        float    speed;
        char     name[48];
        int      weaponType;

        PlayerData()
            : addr(0),
              Head(Vector3::Zero()), Toe(Vector3::Zero()), HeadWorld(Vector3::Zero()),
              Hip(Vector3::Zero()), Breast(Vector3::Zero()), Root(Vector3::Zero()),
              LShoulder(Vector3::Zero()), RShoulder(Vector3::Zero()),
              LAnkle(Vector3::Zero()), RAnkle(Vector3::Zero()),
              LToe(Vector3::Zero()), RToe(Vector3::Zero()),
              LForeArm(Vector3::Zero()), RForeArm(Vector3::Zero()),
              LHand(Vector3::Zero()), RHand(Vector3::Zero()),
              isOnScreen(true), isBehind(false),
              isVisible(false), isBot(false), distance(0.0f),
              curHP(0), maxHP(200), curArmor(0),
              isFiring(false), isGrounded(true), speed(0.0f),
              weaponType(-1) {
            name[0] = '\0';
        }
    };

    struct LootItem {
        uint64_t addr;
        uint32_t guid;
        uint32_t resID;
        Vector3  worldPos;
        Vector3  screenPos;
        float    distance;

        LootItem()
            : addr(0), guid(0), resID(0),
              worldPos(Vector3::Zero()), screenPos(Vector3::Zero()),
              distance(0.0f) {}
    };

    extern std::vector<PlayerData> g_players;
    extern std::mutex              g_players_mutex;

    extern std::vector<LootItem>   g_loots;
    extern std::mutex              g_loots_mutex;

    extern std::atomic<uint64_t>   g_local_player;
    extern std::atomic<bool>       g_in_match;
    extern Matrix4x4               g_view_matrix;

    void start_worker();
    void stop_worker();
    void update_cycle();

    Vector3 world_to_screen(const Matrix4x4& vm, const Vector3& pos, float screen_w, float screen_h);
    float get_distance_meters(const Vector3& a, const Vector3& b);
    Vector3 get_node_position(uint64_t node);

} // namespace engine::data
