#pragma once

namespace ui::bar {
    inline bool  g_open  = false;
    inline float g_alpha = 1.f;
    inline float g_game_alpha = 1.f;

    inline float g_puck_x = 45.f;
    inline float g_puck_y = 160.f;
    inline float g_puck_w = 146.f;
    inline float g_puck_h = 54.f;

    void set_pos(float x, float y);
    void render();
    void set_game_alpha(float a);
    float game_alpha();
}
