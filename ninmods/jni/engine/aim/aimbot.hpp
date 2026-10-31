#pragma once
#include <cstdint>

/**
 * @file aimbot.hpp
 * @brief Combat engine module for Aimbot, Silent Aim, and FOV calculations.
 */

namespace engine::aim {

    void init();
    void update();
    void render_fov();
    uint64_t get_current_target();

} // namespace engine::aim
