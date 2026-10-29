#pragma once

#include "imgui.h"

#include <cmath>

namespace colors
{
    inline ImVec4 accent       = { 90.f / 255.f, 160.f / 255.f, 255.f / 255.f, 1.f };
    inline ImVec4 accent_hover = { 120.f / 255.f, 185.f / 255.f, 255.f / 255.f, 1.f };
    inline ImVec4 accent_dark  = { 50.f / 255.f, 110.f / 255.f, 220.f / 255.f, 1.f };
    inline ImVec4 accent_dim   = { 90.f / 255.f, 160.f / 255.f, 255.f / 255.f, 0.18f };

    // DARKER theme
    inline constexpr ImVec4 bg              = { 10.f / 255.f, 11.f / 255.f, 15.f / 255.f, 0.95f };
    inline constexpr ImVec4 sidebar_btn_bg  = { 16.f / 255.f, 18.f / 255.f, 24.f / 255.f, 0.95f };
    inline constexpr ImVec4 content         = { 14.f / 255.f, 15.f / 255.f, 20.f / 255.f, 1.f };
    inline constexpr ImVec4 section_box     = { 16.f / 255.f, 18.f / 255.f, 24.f / 255.f, 0.95f };
    inline constexpr ImVec4 section_header  = { 22.f / 255.f, 24.f / 255.f, 32.f / 255.f, 0.98f };
    inline constexpr ImVec4 checkbox_off    = { 30.f / 255.f, 32.f / 255.f, 42.f / 255.f, 1.f };
    inline constexpr ImVec4 slider_track    = { 28.f / 255.f, 30.f / 255.f, 40.f / 255.f, 1.f };
    inline constexpr ImVec4 dropdown_bg     = { 22.f / 255.f, 24.f / 255.f, 32.f / 255.f, 1.f };
    inline constexpr ImVec4 dropdown_pop_bg = { 14.f / 255.f, 16.f / 255.f, 22.f / 255.f, 1.f };
    inline constexpr ImVec4 search_bg       = { 18.f / 255.f, 20.f / 255.f, 28.f / 255.f, 0.95f };
    inline constexpr ImVec4 scrollbar_bg    = { 22.f / 255.f, 24.f / 255.f, 32.f / 255.f, 0.8f };

    inline constexpr ImVec4 text            = { 0.94f, 0.95f, 0.97f, 1.f };
    inline constexpr ImVec4 text_muted      = { 0.34f, 0.36f, 0.44f, 1.f };
    inline constexpr ImVec4 text_secondary  = { 0.52f, 0.55f, 0.63f, 1.f };
    inline constexpr ImVec4 text_header     = { 0.80f, 0.83f, 0.89f, 1.f };
    inline constexpr ImVec4 text_label      = { 0.46f, 0.49f, 0.56f, 1.f };

    inline constexpr ImVec4 icon_section    = { 0.62f, 0.66f, 0.74f, 1.f };
    inline constexpr ImVec4 icon_tab_off    = { 0.36f, 0.38f, 0.48f, 1.f };
    inline constexpr ImVec4 icon_tab_on     = { 0.94f, 0.95f, 0.97f, 1.f };

    inline constexpr ImVec4 warning         = { 0.95f, 0.30f, 0.30f, 1.f };
    inline constexpr ImVec4 warning_dim     = { 0.95f, 0.30f, 0.30f, 0.85f };
    inline constexpr ImVec4 exit_bg         = { 0.95f, 0.30f, 0.30f, 0.16f };
    inline constexpr ImVec4 exit_bg_hover   = { 0.95f, 0.30f, 0.30f, 0.28f };
    inline constexpr ImVec4 exit_icon       = { 0.95f, 0.45f, 0.45f, 1.f };

    inline constexpr ImVec4 social          = { 0.36f, 0.39f, 0.48f, 1.f };
    inline constexpr ImVec4 social_hover    = { 0.85f, 0.87f, 0.92f, 1.f };

    inline constexpr ImVec4 row_sep         = { 1.f, 1.f, 1.f, 0.04f };
    inline constexpr ImVec4 pattern_circle  = { 0.55f, 0.70f, 1.f, 0.05f };

    inline constexpr ImVec4 scrollbar_grab  = { 0.30f, 0.32f, 0.42f, 1.f };

    inline void sync_accent_hover()
    {
        accent_hover = {
            fminf(accent.x * 1.10f + 0.06f, 1.f),
            fminf(accent.y * 1.10f + 0.06f, 1.f),
            fminf(accent.z * 1.10f + 0.06f, 1.f),
            accent.w };
        accent_dark = {
            accent.x * 0.65f,
            accent.y * 0.65f,
            accent.z * 0.65f,
            accent.w };
        accent_dim = { accent.x, accent.y, accent.z, 0.18f };
    }
}
