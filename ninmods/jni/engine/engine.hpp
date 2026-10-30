#pragma once
#include <cstdint>

/**
 * @file engine.hpp
 * @brief Core Game Engine Orchestrator for ninmods.
 * Coordinates memory scanning, data updates, combat aimbot, and ESP overlay.
 */

namespace engine {

    int find_active_game_pid();
    const char* get_target_package();
    const char* get_target_region();
    int get_target_pid();

    bool init();
    void update();
    void render_esp();
    void shutdown();

} // namespace engine
