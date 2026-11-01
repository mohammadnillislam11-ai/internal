#include "bar.hpp"
#include "menu.hpp"
#include "Icon.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "styles/styles.h"
#include <cmath>
#include <cstdio>
#include "libraries/storage/logger.h"

namespace ui::bar {
using namespace style;

static float lrp(float a, float b, float t) { return a + (b - a) * t; }
static float ease(float t) { return -(cosf(3.14159265f * t) - 1.f) / 2.f; }

void set_game_alpha(float a) { g_game_alpha = a; }
float game_alpha() { return g_game_alpha; }

static float g_fps_x = -1.f;
static float g_fps_y = -1.f;
static bool g_fps_drag = false;
static bool g_fps_potential = false;
static ImVec2 g_fps_off(0, 0);
static ImVec2 g_fps_down(0, 0);
static constexpr float FPS_DRAG_THRESHOLD = 8.f;

static bool s_fps_pos_init = false;

void set_pos(float x, float y) {
    g_fps_x = x;
    g_fps_y = y;
    g_puck_x = x;
    g_puck_y = y;
    s_fps_pos_init = true;
    if (g_sw > 0.f)
        g_fps_x = ImClamp(g_fps_x, 0.f, ImMax(0.f, g_sw - g_puck_w));
    if (g_sh > 0.f)
        g_fps_y = ImClamp(g_fps_y, 0.f, ImMax(0.f, g_sh - g_puck_h));
    g_puck_x = g_fps_x;
    g_puck_y = g_fps_y;
}

static void get_fps_pos(float w, float h) {
    if (!s_fps_pos_init) {
        if (g_sw > 0.f && g_sh > 0.f) {
            g_fps_x = 45.f;
            g_fps_y = 160.f;
            s_fps_pos_init = true;
        }
    }
    if (g_sw > 0.f)
        g_fps_x = ImClamp(g_fps_x, 0.f, ImMax(0.f, g_sw - w));
    if (g_sh > 0.f)
        g_fps_y = ImClamp(g_fps_y, 0.f, ImMax(0.f, g_sh - h));

    g_puck_x = g_fps_x;
    g_puck_y = g_fps_y;
    g_puck_w = w;
    g_puck_h = h;
}

/**
 * @brief Renders the floating draggable logo puck when the main menu is closed.
 *        Matches menu border styling and tracks exact coordinates with the main menu.
 * @param alpha Global opacity multiplier.
 */
static void render_fps_icon(float alpha) {
  // Compact floating puck: Strictly clamped to puck window proportions (~135 x 46 dp)
  // Prevents the puck from ballooning to the full menu window dimensions (g_sw, g_sh)
  float scale = 1.0f;
  if (ImGui::GetCurrentContext()) {
    float fbScale = ImGui::GetIO().DisplayFramebufferScale.y;
    if (fbScale >= 1.0f && fbScale <= 4.0f) scale = fbScale;
  }
  float maxW = 160.0f * scale;
  float maxH = 56.0f * scale;

  const float w = (g_sw > 10.f && g_sw <= maxW) ? g_sw : (135.f * scale);
  const float h = (g_sh > 10.f && g_sh <= maxH) ? g_sh : (46.f * scale);

  g_fps_x = 0.f;
  g_fps_y = 0.f;
  g_puck_x = 0.f;
  g_puck_y = 0.f;
  g_puck_w = w;
  g_puck_h = h;

  ImDrawList *dl = ImGui::GetForegroundDrawList();
  float fps = ImGui::GetIO().Framerate;
  ImU32 bg_col = IM_COL32(22, 16, 36, static_cast<int>(245 * alpha));
  ImU32 border_col = col(UITheme::border, alpha);

  float pillR = h * 0.5f;
  dl->AddRectFilled(
      {g_fps_x - 3.f, g_fps_y - 3.f}, {g_fps_x + w + 3.f, g_fps_y + h + 3.f},
      col(UITheme::border_light, 0.16f * alpha), pillR + 3.f);
  dl->AddRectFilled({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h}, bg_col,
                    pillR);
  dl->AddRect({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h}, border_col, pillR,
              0, 2.0f);

  // Top & bottom accent notches styled with signature border color
  float notchW = w * 0.22f;
  float notchX = w * 0.44f;
  dl->AddRectFilled({g_fps_x + notchX, g_fps_y - 2.0f},
                    {g_fps_x + notchX + notchW, g_fps_y + 1.5f},
                    col(UITheme::border_light, alpha), 1.5f);
  dl->AddRectFilled({g_fps_x + notchX, g_fps_y + h - 1.5f},
                    {g_fps_x + notchX + notchW, g_fps_y + h + 2.0f},
                    col(UITheme::border_light, alpha), 1.5f);

  float cx = g_fps_x + h * 0.52f, cy = g_fps_y + h * 0.5f, radius = h * 0.36f;
  dl->AddCircleFilled({cx, cy}, radius,
                      IM_COL32(36, 26, 56, static_cast<int>(255 * alpha)));
  dl->AddCircle({cx, cy}, radius, col(UITheme::border_light, alpha), 32, 2.2f);

  ImFont *font = ImGui::GetFont();
  if (font) {
    float nSize = h * 0.38f;
    float nw = font->CalcTextSizeA(nSize, FLT_MAX, 0.f, "N").x;
    dl->AddText(font, nSize, {cx - nw * 0.5f, cy - nSize * 0.5f},
                IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)), "N");

    int fpsInt = static_cast<int>(fps + 0.5f);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d FPS", fpsInt);
    ImU32 text_col = fps >= 45.f   ? IM_COL32(52, 211, 110, 255)
                     : fps >= 28.f ? IM_COL32(255, 180, 40, 255)
                                   : IM_COL32(255, 69, 58, 255);
    float fpsSize = h * 0.34f;
    float textX = cx + radius + h * 0.20f;
    dl->AddText(font, fpsSize, {textX, g_fps_y + (h - fpsSize) * 0.5f},
                text_col, buf);
  }
}

/**
 * @brief Dispatches the floating puck renderer with ease-in-out transition when
 * menu is closed.
 */
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
