#pragma once
#include "imgui.h"

namespace cfg {
namespace esp {
    // ── Master Switch (Default: OFF) ──
    inline bool  enabled      = false;   // 100% Manual control: nothing runs until turned ON from menu

    // ── Features (All default to OFF) ──
    inline bool  box          = false;
    inline int   box_type     = 0;
    inline bool  name         = false;
    inline bool  health       = false;
    inline bool  armor        = false;
    inline bool  distance     = false;
    inline bool  weapon       = false;
    inline bool  bot_tag      = false;
    inline bool  skeleton     = false;
    inline bool  tracer       = false;
    inline bool  crosshair    = false;
    inline bool  firing_ind   = false;
    inline bool  loot         = false;

    // ── Setting ──
    inline float max_range     = 250.f;
    inline float loot_range    = 150.f;
    inline float box_rounding  = 3.f;
    inline float box_thickness = 2.0f;
    inline float bracket_frac  = 0.28f;

    // ── Warna ──
    inline ImVec4 box_col      = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 box_visible  = ImVec4(0.30f, 1.0f, 0.55f, 1.0f);
    inline ImVec4 name_col     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col = ImVec4(0.95f, 0.95f, 0.30f, 1.0f);
    inline ImVec4 weapon_col   = ImVec4(1.0f, 0.85f, 0.30f, 1.0f);
}
}