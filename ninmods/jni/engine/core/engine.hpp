#pragma once
#include <cstdint>

/**
 * @file engine.hpp
 * @brief Core Game Engine Orchestrator for ninmods.
 * Strictly dedicated to Normal Free Fire (com.dts.freefireth).
 * Operates 100% in-memory with direct kernel assembly syscalls.
 */

namespace engine {

    inline const char* g_active_package = "com.dts.freefireth";
    constexpr const char* TARGET_PACKAGE = "com.dts.freefireth";
    constexpr const char* TARGET_REGION  = "Global";

    const char* get_target_package();
    const char* get_target_region();
    int get_target_pid();
    bool is_attached();

    bool init();
    void update();
    void render_esp();
    void shutdown();

} // namespace engine
