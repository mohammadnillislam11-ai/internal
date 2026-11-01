#pragma once
#include <cstdint>
#include "imgui.h"
#include "engine/memory.hpp"

namespace ui::menu {
    inline float g_menu_x = -1.f;
    inline float g_menu_y = -1.f;
    inline float g_menu_w = 760.f;
    inline float g_menu_h = 460.f;
    inline ImVec2 g_menu_pos(-1.f, -1.f);

    inline void set_pos(float x, float y) {
        g_menu_pos.x = x;
        g_menu_pos.y = y;
        g_menu_x = x;
        g_menu_y = y;
    }

    inline bool is_inside(float x, float y) {
        if (g_menu_x < 0.f || g_menu_y < 0.f) return false;
        return (x >= g_menu_x && x <= (g_menu_x + g_menu_w) &&
                y >= g_menu_y && y <= (g_menu_y + g_menu_h));
    }

    void render();
}
