#include "bar.hpp"
#include "styles/styles.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <cstdio>

namespace ui::bar {
    using namespace style;

    static float lrp(float a, float b, float t) { return a + (b - a) * t; }
    static float ease(float t) { return -(cosf(3.14159265f * t) - 1.f) / 2.f; }

    void set_game_alpha(float a) { g_game_alpha = a; }
    float game_alpha() { return g_game_alpha; }

    // ============================================
    // ICON FPS
    // ============================================
    static float  g_fps_x = -1.f;
    static float  g_fps_y = -1.f;
    static bool   g_fps_drag = false;
    static bool   g_fps_potential = false;
    static ImVec2 g_fps_off(0, 0);
    static ImVec2 g_fps_down(0, 0);
    static constexpr float FPS_DRAG_THRESHOLD = 10.f;

    static void get_fps_pos(float w, float h) {
        if (g_fps_x < 0.f || g_fps_y < 0.f) {
            g_fps_x = g_sw - w - 25.f;
            g_fps_y = g_sh - h - 25.f;
        }
        if (g_sw > 0) g_fps_x = ImMax(0.f, ImMin(g_fps_x, g_sw - w));
        if (g_sh > 0) g_fps_y = ImMax(0.f, ImMin(g_fps_y, g_sh - h));
    }

    static void render_fps_icon(float alpha) {
        const float w = 112.f, h = 38.f;
        get_fps_pos(w, h);

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 mp = ImGui::GetMousePos();
        bool hov = (mp.x >= g_fps_x && mp.x <= g_fps_x + w &&
                    mp.y >= g_fps_y && mp.y <= g_fps_y + h);

        // Detect drag start
        if (hov && ImGui::IsMouseClicked(0)) {
            g_fps_potential = true;
            g_fps_down = mp;
            g_fps_off = ImVec2(mp.x - g_fps_x, mp.y - g_fps_y);
        }

        // Drag
        if (g_fps_potential && ImGui::IsMouseDown(0)) {
            float dx = mp.x - g_fps_down.x;
            float dy = mp.y - g_fps_down.y;
            if (g_fps_drag || (dx*dx + dy*dy) > (FPS_DRAG_THRESHOLD * FPS_DRAG_THRESHOLD)) {
                g_fps_drag = true;
                g_fps_x = mp.x - g_fps_off.x;
                g_fps_y = mp.y - g_fps_off.y;
                get_fps_pos(w, h);
            }
        }

        // Release -> click = buka menu
        if (ImGui::IsMouseReleased(0)) {
            if (g_fps_potential && !g_fps_drag) {
                g_open = true;
            }
            g_fps_potential = false;
            g_fps_drag = false;
        }

        float fps = ImGui::GetIO().Framerate;

        ImU32 bg_col     = IM_COL32(18, 12, 34, (int)(210 * alpha));
        ImU32 border_col = g_fps_drag ? col(UITheme::accent_light, alpha)
                                      : col(UITheme::border, alpha);

        // Background pill
        float pillR = h * 0.5f;
        dl->AddRectFilled({g_fps_x + 2, g_fps_y + 3}, {g_fps_x + w + 2, g_fps_y + h + 3},
                          IM_COL32(0, 0, 0, (int)(60 * alpha)), pillR);
        dl->AddRectFilled({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h},
                          bg_col, pillR);
        dl->AddRect({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h},
                    border_col, pillR, 0, g_fps_drag ? 2.f : 1.2f);

        // Accent strip kiri (neon purple)
        dl->AddRectFilled({g_fps_x + 4.f, g_fps_y + 4.f},
                          {g_fps_x + 7.f, g_fps_y + h - 4.f},
                          col(UITheme::accent, alpha), 2.f);

        // ============================================
        // TEXT NAME "nin" & FPS (NO LOGO)
        // ============================================
        ImFont* font = ImGui::GetFont();
        if (font) {
            // Text "nin"
            dl->AddText(font, 16.f,
                        {g_fps_x + 14.f, g_fps_y + (h - 16.f) * 0.5f},
                        col(UITheme::accent_light, alpha), "nin");

            // Separator
            dl->AddText(font, 12.f,
                        {g_fps_x + 44.f, g_fps_y + (h - 12.f) * 0.5f},
                        col(UITheme::text_dim, alpha), "|");

            // FPS Text
            int fpsInt = (int)(fps + 0.5f);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d FPS", fpsInt);
            ImU32 text_col = fps >= 50.f ? IM_COL32(52, 199, 89, 255)
                            : fps >= 30.f ? IM_COL32(255, 180, 40, 255)
                                          : IM_COL32(255, 69, 58, 255);
            dl->AddText(font, 14.f,
                        {g_fps_x + 54.f, g_fps_y + (h - 14.f) * 0.5f},
                        text_col, buf);
        }
    }

    // ============================================
    // RENDER - icon FPS hanya saat menu tertutup
    // ============================================
    void render() {
        static float prog = 0.f;
        float dt = ImGui::GetIO().DeltaTime;

        float tgt = g_open ? 1.f : 0.f;
        float spd = 12.f * dt;
        if (prog < tgt) prog = ImMin(prog + spd, tgt);
        else if (prog > tgt) prog = ImMax(prog - spd, tgt);
        prog = ImClamp(prog, 0.f, 1.f);

        g_alpha = ease(prog);

        if (g_game_alpha > 0.001f && !g_open) {
            render_fps_icon(1.f);
        }
    }
}
