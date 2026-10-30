#pragma once

#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "../other/utils.hpp"
#include "math.hpp"
#include "data.hpp"
#include "../protect/oxorany.hpp"
#include <cstdint>
#include <mutex>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <android/log.h>

/** Logging macro for AimSilent */
#define AS_LOG(...) __android_log_print(ANDROID_LOG_INFO, "AimSilent", __VA_ARGS__)

/** Feature state toggles */
inline bool AimSilentEnabled = false;
inline bool AimSilentNobita  = false;
static bool g_AimSilentHookInstalled = false;

/** Entity transform and distance helper routines */

static void *GetHeadColliderSilent(void *player) {
    if (!player) return nullptr;
    void *(*_GetHead)(void *) = (void *(*)(void *))((uintptr_t)proc::lib + Offsets::Player_get_HeadCollider);
    return _GetHead(player);
}

static void *GetHeadTFSilent(void *player) {
    if (!player) return nullptr;
    void *(*_GetHead)(void *) = (void *(*)(void *))((uintptr_t)proc::lib + Offsets::Player_GetHeadTF);
    return _GetHead(player);
}

static Vector3 GetHeadPositionSilent(void *player) {
    if (!player) return Vector3::Zero();
    void *headTF = GetHeadTFSilent(player);
    if (!headTF) return Vector3::Zero();
    return GetNodePosition((uint64_t)headTF);
}

static void *GetClosestEnemySilent() {
    std::lock_guard<std::mutex> lk(data::players_mutex);

    void *bestTarget = nullptr;
    float bestDist = 99999.0f;

    uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
    if (!localAddr) return nullptr;

    for (auto& p : data::players) {
        if (!p.addr) continue;
        if (p.addr == localAddr) continue;
        if (p.curHP <= 0) continue;
        if (p.distance > 1000.0f) continue;

        if (p.distance < bestDist) {
            bestDist = p.distance;
            bestTarget = (void*)p.addr;
        }
    }

    return bestTarget;
}

/**
 * @brief Tests memory write capability at specified virtual address.
 * @param addr Virtual memory address to probe.
 * @return True if write operation succeeded.
 */
static inline bool TestWrite(uintptr_t addr) {
    uint32_t orig = rpm<uint32_t>(addr);
    bool wr = wpm<uint32_t>(addr, orig);
    return wr;
}

/** Function pointer for original 9-param OnCalcDamage */
static int (*orig_OnCalcDamage_9)(void *player, int damage, uint damagerID,
                         void *damageInfo, int hitGroup,
                         Vector3 hitPos, Vector3 hitNormal,
                         void *checkParams, void *weaponInfo, uint vehicleID) = nullptr;

static int hook_OnCalcDamage_9(void *player, int damage, uint damagerID,
                      void *damageInfo, int hitGroup,
                      Vector3 hitPos, Vector3 hitNormal,
                      void *checkParams, void *weaponInfo, uint vehicleID) {

    static int callCount = 0;
    if (callCount++ < 30) {
        AS_LOG("HOOK9 #%d | player=%p dmgInfo=%p hitGroup=%d dmg=%d",
               callCount, player, damageInfo, hitGroup, damage);
    }

    if (!AimSilentEnabled && !AimSilentNobita) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    if (!player || !damageInfo) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    AS_LOG("AIM ACTIVE (9)! player=%p dmgInfo=%p", player, damageInfo);

    void *target = GetClosestEnemySilent();
    if (!target) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    Vector3 enemyHeadPos = GetHeadPositionSilent(target);
    if (enemyHeadPos == Vector3::Zero()) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    Vector3 myPos = Vector3::Zero();
    {
        uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (localAddr) {
            uint64_t headNode = rpm<uint64_t>(localAddr + Offsets::Bone_Head);
            if (headNode) myPos = GetNodePosition(headNode);
        }
    }

    Vector3 direction = (myPos == Vector3::Zero())
                        ? hitPos
                        : Vector3::Normalize(enemyHeadPos - myPos);

    AS_LOG("MODIFY: headPos=(%.1f,%.1f,%.1f)",
           enemyHeadPos.x, enemyHeadPos.y, enemyHeadPos.z);

    *(short *)((uintptr_t)damageInfo + Offsets::DmgInfo_HitGroup) = 1;
    *(Vector3 *)((uintptr_t)damageInfo + Offsets::DmgInfo_HitLocation) = enemyHeadPos;
    *(Vector3 *)((uintptr_t)damageInfo + Offsets::DmgInfo_HitNormal) = direction;
    *(Vector3 *)((uintptr_t)damageInfo + Offsets::DmgInfo_RayDir) = direction;

    hitGroup = 1;

    return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                              hitGroup, enemyHeadPos, direction,
                              checkParams, weaponInfo, vehicleID);
}

/**
 * @brief Initializes manual Aim Silent memory hook.
 */
static inline void InitAimSilentHook() {
    if (g_AimSilentHookInstalled) return;

    uintptr_t base = (uintptr_t)proc::lib;
    if (!base) return;

    uintptr_t addr9 = base + Offsets::Player_TakeDamage_9;
    if (!TestWrite(addr9)) return;

    g_AimSilentHookInstalled = true;
}