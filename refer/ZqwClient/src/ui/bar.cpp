#include "bar.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "styles/styles.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <cstdio>

namespace ui::bar {
using namespace style;

static float lrp(float a, float b, float t) { return a + (b - a) * t; }
static float ease(float t) { return -(cosf(3.14159265f * t) - 1.f) / 2.f; }

void set_game_alpha(float a) { g_game_alpha = a; }
float game_alpha() { return g_game_alpha; }

/** Floating FPS indicator position and drag status */
static float g_fps_x = -1.f;
static float g_fps_y = -1.f;
static bool g_fps_drag = false;
static bool g_fps_potential = false;
static ImVec2 g_fps_off(0, 0);
static ImVec2 g_fps_down(0, 0);
static constexpr float FPS_DRAG_THRESHOLD = 10.f;

static void get_fps_pos(float w, float h) {
  if (g_fps_x < 0.f || g_fps_y < 0.f) {
    // Default position: Top-Left (always visible on all phone screen
    // resolutions)
    g_fps_x = 50.f;
    g_fps_y = 120.f;
  }
  if (g_sw > 0)
    g_fps_x = ImMax(10.f, ImMin(g_fps_x, g_sw - w - 10.f));
  if (g_sh > 0)
    g_fps_y = ImMax(10.f, ImMin(g_fps_y, g_sh - h - 10.f));
}

static void render_fps_icon(float alpha) {
  const float w = 118.f, h = 40.f;
  get_fps_pos(w, h);

  ImDrawList *dl = ImGui::GetForegroundDrawList();
  ImVec2 mp = ImGui::GetMousePos();

  // Generous touch hit-testing for mobile finger taps
  const float hitPad = 14.f;
  bool hov = (mp.x >= g_fps_x - hitPad && mp.x <= g_fps_x + w + hitPad &&
              mp.y >= g_fps_y - hitPad && mp.y <= g_fps_y + h + hitPad);

  // Detect drag start or tap
  if (hov && ImGui::IsMouseClicked(0)) {
    g_fps_potential = true;
    g_fps_down = mp;
    g_fps_off = ImVec2(mp.x - g_fps_x, mp.y - g_fps_y);
  }

  // Drag
  if (g_fps_potential && ImGui::IsMouseDown(0)) {
    float dx = mp.x - g_fps_down.x;
    float dy = mp.y - g_fps_down.y;
    if (g_fps_drag ||
        (dx * dx + dy * dy) > (FPS_DRAG_THRESHOLD * FPS_DRAG_THRESHOLD)) {
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

  ImU32 bg_col = IM_COL32(16, 18, 24, (int)(240 * alpha));
  ImU32 border_col = g_fps_drag ? IM_COL32(120, 185, 255, (int)(255 * alpha))
                                : IM_COL32(90, 160, 255, (int)(200 * alpha));

  // Outer glow
  float pillR = h * 0.5f;
  dl->AddRectFilled({g_fps_x - 3, g_fps_y - 3},
                    {g_fps_x + w + 3, g_fps_y + h + 3},
                    IM_COL32(90, 160, 255, (int)(35 * alpha)), pillR + 3.f);
  // Shadow
  dl->AddRectFilled({g_fps_x + 2, g_fps_y + 4},
                    {g_fps_x + w + 2, g_fps_y + h + 4},
                    IM_COL32(0, 0, 0, (int)(120 * alpha)), pillR);
  // Background pill
  dl->AddRectFilled({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h}, bg_col,
                    pillR);
  // Border
  dl->AddRect({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h}, border_col, pillR,
              0, g_fps_drag ? 2.f : 1.5f);

  // Accent strip left (electric blue)
  dl->AddRectFilled({g_fps_x + 6.f, g_fps_y + 6.f},
                    {g_fps_x + 9.f, g_fps_y + h - 6.f},
                    IM_COL32(90, 160, 255, (int)(255 * alpha)), 2.f);

  /** Brand label "nin" and live FPS reading */
  ImFont *font = ImGui::GetFont();
  if (font) {
    // Text "nin"
    dl->AddText(font, 16.f, {g_fps_x + 16.f, g_fps_y + (h - 16.f) * 0.5f},
                IM_COL32(240, 245, 255, (int)(255 * alpha)), "nin");

    // Separator
    dl->AddText(font, 12.f, {g_fps_x + 46.f, g_fps_y + (h - 12.f) * 0.5f},
                IM_COL32(90, 160, 255, (int)(180 * alpha)), "|");

    // FPS Text
    int fpsInt = (int)(fps + 0.5f);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d FPS", fpsInt);
    ImU32 text_col = fps >= 50.f ? IM_COL32(52, 211, 153, (int)(255 * alpha))
                     : fps >= 30.f
                         ? IM_COL32(251, 191, 36, (int)(255 * alpha))
                         : IM_COL32(248, 113, 113, (int)(255 * alpha));
    dl->AddText(font, 14.f, {g_fps_x + 56.f, g_fps_y + (h - 14.f) * 0.5f},
                text_col, buf);
  }
}

/** Floating indicator renderer (visible when menu is minimized) */
void render() {
  static float prog = 0.f;
  float dt = ImGui::GetIO().DeltaTime;

  float tgt = g_open ? 1.f : 0.f;
  float spd = 12.f * dt;
  if (prog < tgt)
    prog = ImMin(prog + spd, tgt);
  else if (prog > tgt)
    prog = ImMax(prog - spd, tgt);
  prog = ImClamp(prog, 0.f, 1.f);

  g_alpha = ease(prog);

  if (g_game_alpha > 0.001f && !g_open) {
    render_fps_icon(1.f);
  }
}
} // namespace ui::bar
