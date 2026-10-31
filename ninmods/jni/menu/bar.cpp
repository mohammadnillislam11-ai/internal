#include "bar.hpp"
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
static void get_fps_pos(float w, float h) {
    if (!s_fps_pos_init) {
        if (g_sw > 0.f && g_sh > 0.f) {
            g_fps_x = g_sw - w - 25.f;
            g_fps_y = g_sh - h - 35.f;
            s_fps_pos_init = true;
        }
    }
    if (g_sw > 0.f)
        g_fps_x = ImClamp(g_fps_x, 0.f, ImMax(0.f, g_sw - w));
    if (g_sh > 0.f)
        g_fps_y = ImClamp(g_fps_y, 0.f, ImMax(0.f, g_sh - h));
}

/**
 * @brief Renders the floating draggable logo puck when the main menu is closed.
 *        Features neon light-purple glowing circular N logo and live FPS counter.
 * @param alpha Global opacity multiplier.
 */
static void render_fps_icon(float alpha) {
  const float w = 146.f, h = 54.f;
  get_fps_pos(w, h);

  ImDrawList *dl = ImGui::GetForegroundDrawList();
  ImVec2 mp = ImGui::GetMousePos();
  bool hov = (mp.x >= g_fps_x && mp.x <= g_fps_x + w && mp.y >= g_fps_y &&
              mp.y <= g_fps_y + h);

  if (hov && (ImGui::IsMouseClicked(0) || (!g_fps_potential && ImGui::IsMouseDown(0)))) {
    g_fps_potential = true;
    g_fps_down = mp;
    g_fps_off = ImVec2(mp.x - g_fps_x, mp.y - g_fps_y);
  }

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

  if (ImGui::IsMouseReleased(0)) {
    if (g_fps_drag) {
      logger::log_info("[PUCK] Floating logo moved to: (X: %.1f, Y: %.1f)", g_fps_x, g_fps_y);
    } else if (g_fps_potential) {
      g_open = true;
      logger::log_info("[MENU] Menu opened via floating logo tap.");
    }
    g_fps_potential = false;
    g_fps_drag = false;
  }

  float fps = ImGui::GetIO().Framerate;
  ImU32 bg_col = IM_COL32(24, 18, 40, static_cast<int>(245 * alpha));
  ImU32 border_col = g_fps_drag ? col(UITheme::accent_light, alpha)
                                : col(UITheme::border_light, alpha);

  float pillR = h * 0.5f;
  dl->AddRectFilled(
      {g_fps_x - 3.f, g_fps_y - 3.f}, {g_fps_x + w + 3.f, g_fps_y + h + 3.f},
      IM_COL32(180, 105, 255, static_cast<int>(40 * alpha)), pillR + 3.f);
  dl->AddRectFilled({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h}, bg_col,
                    pillR);
  dl->AddRect({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h}, border_col, pillR,
              0, g_fps_drag ? 2.4f : 1.6f);

  // Top & bottom neon purple accent notches centered exactly like Image 2
  dl->AddRectFilled({g_fps_x + 60.f, g_fps_y - 2.0f},
                    {g_fps_x + 86.f, g_fps_y + 1.5f},
                    col(UITheme::accent, alpha), 1.5f);
  dl->AddRectFilled({g_fps_x + 60.f, g_fps_y + h - 1.5f},
                    {g_fps_x + 86.f, g_fps_y + h + 2.0f},
                    col(UITheme::accent, alpha), 1.5f);

  float cx = g_fps_x + 28.f, cy = g_fps_y + h * 0.5f, radius = 19.f;
  dl->AddCircleFilled({cx, cy}, radius,
                      IM_COL32(45, 30, 75, static_cast<int>(255 * alpha)));
  dl->AddCircle({cx, cy}, radius, col(UITheme::accent, alpha), 32, 2.2f);
  dl->AddCircle({cx, cy}, radius + 2.f, col(UITheme::accent_dark, alpha * 0.5f),
                32, 1.2f);

  ImFont *font = ImGui::GetFont();
  if (font) {
    float nw = font->CalcTextSizeA(19.f, FLT_MAX, 0.f, "N").x;
    dl->AddText(font, 19.f, {cx - nw * 0.5f, cy - 9.5f},
                IM_COL32(255, 255, 255, static_cast<int>(255 * alpha)), "N");

    int fpsInt = static_cast<int>(fps + 0.5f);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d FPS", fpsInt);
    ImU32 text_col = fps >= 45.f   ? IM_COL32(52, 211, 110, 255)
                     : fps >= 28.f ? IM_COL32(255, 180, 40, 255)
                                   : IM_COL32(255, 69, 58, 255);
    dl->AddText(font, 18.5f, {g_fps_x + 58.f, g_fps_y + (h - 18.5f) * 0.5f},
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
