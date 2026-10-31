#pragma once
#include <cstdint>

/**
 * @file engine.hpp
 * @brief Core Game Engine Orchestrator for ninmods.
 * Coordinates memory scanning, data updates, combat aimbot, and ESP overlay.
 */

namespace engine {

    inline const char* g_active_package = "com.dts.freefireth";
    constexpr const char* TARGET_PACKAGE = "com.dts.freefireth";
    constexpr const char* TARGET_PACKAGE_MAX = "com.dts.freefiremax";
    constexpr const char* TARGET_REGION  = "Global";

    const char* get_target_package();
    const char* get_target_region();
    int get_target_pid();

    bool init();
    void update();
    void render_esp();
    void shutdown();

} // namespace engine
