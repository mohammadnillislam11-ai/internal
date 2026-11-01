#pragma once
#include <cstdint>
#include "engine/memory.hpp"

namespace ui::menu {
    inline float g_menu_x = -1.f;
    inline float g_menu_y = -1.f;
    inline float g_menu_w = 860.f;
    inline float g_menu_h = 530.f;

    inline bool is_inside(float x, float y) {
        if (g_menu_x < 0.f || g_menu_y < 0.f) return false;
        return (x >= g_menu_x && x <= (g_menu_x + g_menu_w) &&
                y >= g_menu_y && y <= (g_menu_y + g_menu_h));
    }

    void render();
}
