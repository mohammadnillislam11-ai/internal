#pragma once
#include <cstdint>

/**
 * @file aimbot.hpp
 * @brief Combat engine module for Aimbot, Silent Aim, and FOV calculations.
 * Features safe memory state tracking and automatic restoration when toggled off.
 */

namespace engine::aim {

    void init();
    void update();
    void render_fov();
    void restore_state();
    uint64_t get_current_target();

} // namespace engine::aim
