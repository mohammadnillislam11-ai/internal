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
        // Master Toggles - 100% OFF by default
        inline bool  enabled        = false;
        inline bool  box            = false;
        inline int   box_type       = 0; // 0 = Corner, 1 = Full
        inline bool  name           = false;
        inline bool  health         = false;
        inline bool  armor          = false;
        inline bool  distance       = false;
        inline bool  weapon         = false;
        inline bool  bot_tag        = false;
        inline bool  tracer         = false;
        inline bool  skeleton       = false;
        inline bool  crosshair      = false;
        inline bool  firing_ind     = false;
        inline bool  loot           = false;

        // Sliders & Adjustments
        inline float max_range      = 250.0f;
        inline float loot_range     = 150.0f;
        inline float box_rounding   = 2.0f;
        inline float box_thickness  = 1.0f; // Thread-thin line
        inline float fix_esp        = 0.0f;

        // Color Themes
        inline ImVec4 box_col       = ImVec4(0.68f, 0.35f, 0.98f, 1.0f); // Royal Neon Purple
        inline ImVec4 box_visible   = ImVec4(0.30f, 1.00f, 0.55f, 1.0f); // Neon Green (Visible)
        inline ImVec4 name_col      = ImVec4(1.00f, 1.00f, 1.00f, 1.0f); // White
        inline ImVec4 distance_col  = ImVec4(0.95f, 0.95f, 0.30f, 1.0f); // Yellow
        inline ImVec4 weapon_col    = ImVec4(1.00f, 0.85f, 0.30f, 1.0f); // Orange Gold
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
