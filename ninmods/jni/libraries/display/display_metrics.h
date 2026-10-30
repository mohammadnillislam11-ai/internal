#pragma once
#include <algorithm>
#include <cstdint>

/**
 * @file display_metrics.h
 * @brief Auto-scaling resolution and DPI metrics calculation for Android.
 */
namespace display {

    inline float g_screen_width   = 2400.0f;
    inline float g_screen_height  = 1080.0f;
    inline float &width           = g_screen_width;
    inline float &height          = g_screen_height;
    inline float g_scale_factor   = 1.0f;
    inline float g_font_scale     = 1.0f;

    inline void init(int32_t width, int32_t height) {
        g_screen_width  = static_cast<float>(width);
        g_screen_height = static_cast<float>(height);

        // Normalize scaling against 1080p standard height
        float base_h = (g_screen_height < g_screen_width) ? g_screen_height : g_screen_width;
        if (base_h <= 0.0f) base_h = 1080.0f;

        g_scale_factor = base_h / 1080.0f;
        // Clamp scale factor between 0.70x (720p) and 1.60x (1440p/4K)
        g_scale_factor = std::max(0.70f, std::min(g_scale_factor, 1.60f));
        g_font_scale = g_scale_factor;
    }

    inline float scale(float val) {
        return val * g_scale_factor;
    }

} // namespace display
