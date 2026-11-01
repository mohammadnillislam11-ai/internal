#pragma once
#include "imgui.h"

/**
 * @file config.hpp
 * @brief Central feature flags for ninmods.
 * Strict Security Policy: All features default to FALSE (OFF).
 * No background scanning or hooking occurs until explicitly toggled ON from the menu.
 */

namespace cfg {


    namespace esp {
        // Master Toggles - Active by default for immediate visual feedback
        inline bool  enabled        = true;
        inline bool  box            = true;  // ESP Box
        inline int   box_type       = 0;     // 0 = Corner, 1 = Full, 2 = Glow
        inline bool  tracer         = true;  // ESP Line
        inline bool  health         = true;  // ESP Health

        // Sliders & Adjustments
        inline float max_range      = 250.0f;
        inline float box_rounding   = 3.0f;
        inline float box_thickness  = 1.8f;
        inline float bracket_frac   = 0.28f;

        // Color Themes (3 selectable colors: 0=Blue, 1=Red, 2=Pink)
        inline int   box_col_idx    = 0; // 0 = Blue
        inline int   vis_col_idx    = 1; // 1 = Red
        inline ImVec4 box_col       = ImVec4(0.10f, 0.65f, 1.00f, 1.0f); // Blue
        inline ImVec4 box_visible   = ImVec4(1.00f, 0.22f, 0.25f, 1.0f); // Red
    }

    namespace aim {
        // Master Toggles
        inline bool  enabled        = false;
        inline bool  silent         = false; // Aim Silent
        inline bool  nobita         = false; // Nobita Mode
        inline int   target_bone    = 0;     // 0 = Head, 1 = Neck, 2 = Chest, 3 = Pelvis
        inline int   trigger_type   = 0;     // 0 = On Fire, 1 = On Scope, 2 = Always
        inline bool  draw_fov       = true;  // Circle Aim (FOV) Active by default

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
