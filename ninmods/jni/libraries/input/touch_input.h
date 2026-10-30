#pragma once
#include <cstdint>

/**
 * @file touch_input.h
 * @brief Android multi-touch input event polling and gesture handling.
 */
namespace touch {
    bool init(int32_t screen_w, int32_t screen_h, uint8_t orientation);
    void update(int32_t screen_w, int32_t screen_h, uint8_t orientation);
    void update_orientation(uint8_t orientation);
    uint8_t get_orientation();
    void toggle_orientation();
    void poll();
    void shutdown();
}
