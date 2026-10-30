#pragma once
#include "imgui.h"

namespace cfg {
namespace esp {
    /** Master Switch (Default: OFF, 100% manual menu gating) */
    inline bool  enabled      = false;

    /** Essential ESP features (All default to OFF) */
    inline bool  tracer       = false;   // ESP Line
    inline bool  box          = false;   // ESP Box
    inline int   box_type     = 0;       // Box Style (0 = Corner brackets, 1 = Full outline)
    inline bool  name         = false;   // ESP Name
    inline bool  health       = false;   // ESP Health bar
    inline bool  distance     = false;   // ESP Distance
    inline bool  skeleton     = false;   // ESP Skeleton bones
    inline bool  crosshair    = false;   // ESP Crosshair ring
    inline bool  firing_ind   = false;   // Target ESP / Firing indicator
    inline bool  bot_tag      = false;   // Bot tag

    /** Geometry and range parameters */
    inline float max_range     = 250.f;
    inline float box_rounding  = 2.f;
    inline float box_thickness = 1.2f;
    inline float bracket_frac  = 0.25f;

    /** Theme and visibility color palettes */
    inline ImVec4 box_col      = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 box_visible  = ImVec4(0.30f, 1.0f, 0.55f, 1.0f);
    inline ImVec4 name_col     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col = ImVec4(0.95f, 0.95f, 0.30f, 1.0f);
}
}
