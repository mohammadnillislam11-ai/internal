#pragma once
#include "imgui.h"

/**
 * @file config.hpp
 * @brief Central feature flags for ninmods.
 * Strict Security Policy: All features default to FALSE (OFF).
 * No background scanning or hooking occurs until explicitly toggled ON from the menu.
 */

namespace cfg {

    // Streamer Mode (Anti-Screen-Record / Anti-Screenshot protection)
    inline bool streamer_mode = false;

    namespace esp {
        // Master Toggles - 100% OFF by default (ESP Line, ESP Box, ESP Health only)
        inline bool  enabled        = false;
        inline bool  box            = false; // ESP Box
        inline int   box_type       = 0;     // 0 = Corner, 1 = Full, 2 = Glow
        inline bool  tracer         = false; // ESP Line
        inline bool  health         = false; // ESP Health

        // Sliders & Adjustments
        inline float max_range      = 250.0f;
        inline float box_rounding   = 3.0f;
        inline float box_thickness  = 1.8f;
        inline float bracket_frac   = 0.28f;

        // Color Themes
        inline ImVec4 box_col       = ImVec4(0.0f, 0.90f, 1.0f, 1.0f); // Bright Cyan
        inline ImVec4 box_visible   = ImVec4(0.30f, 1.00f, 0.55f, 1.0f); // Neon Green (Visible)
    }

    namespace aim {
        // Master Toggles - 100% OFF by default
        inline bool  enabled        = false;
        inline bool  silent         = false; // Aim Silent
        inline bool  nobita         = false; // Nobita Mode
        inline int   target_bone    = 0;     // 0 = Head, 1 = Neck, 2 = Chest, 3 = Pelvis
        inline int   trigger_type   = 0;     // 0 = On Fire, 1 = On Scope, 2 = Always
        inline bool  draw_fov       = false; // Circle Aim (FOV)

        // Sliders
        inline float fov            = 120.0f; // Circle Aim Radius (px)
        inline float distance       = 250.0f;
        inline float smooth         = 8.0f;
    }

    namespace misc {
        inline bool  speed_hack     = false;
        inline bool  fast_switch    = false;
        inline bool  grenade_esp    = false;
        inline bool  alert_360      = false;
    }

} // namespace cfg
