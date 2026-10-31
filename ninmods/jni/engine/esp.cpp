#define IMGUI_DEFINE_MATH_OPERATORS
#include "esp.hpp"
#include "game_data.hpp"
#include "menu/config.hpp"
#include "menu/style.hpp"
#include "display_metrics.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>

namespace engine::esp {

    static inline ImU32 to_u32(const ImVec4& c, float alpha_mul = 1.0f) {
        return IM_COL32(
            static_cast<int>(c.x * 255.0f),
            static_cast<int>(c.y * 255.0f),
            static_cast<int>(c.z * 255.0f),
            static_cast<int>(c.w * 255.0f * alpha_mul)
        );
    }

    static inline ImU32 with_alpha(ImU32 c, float mul) {
        int a = static_cast<int>(((c >> 24) & 0xFF) * mul);
        if (a < 0) a = 0; if (a > 255) a = 255;
        return (c & 0x00FFFFFF) | (static_cast<ImU32>(a) << 24);
    }

    static inline ImU32 fade_dist(ImU32 base, float dist, float max_range) {
        float t = ImClamp(1.0f - dist / (max_range > 0.0f ? max_range : 400.0f), 0.35f, 1.0f);
        return with_alpha(base, t);
    }

    static inline ImU32 hp_color(float t) {
        t = ImClamp(t, 0.0f, 1.0f);
        float r, g, b;
        if (t < 0.5f) {
            float k = t * 2.0f;
            r = 1.0f;
            g = 0.35f + 0.55f * k;
            b = 0.15f;
        } else {
            float k = (t - 0.5f) * 2.0f;
            r = 1.0f - 0.9f * k;
            g = 0.9f;
            b = 0.15f + 0.30f * k;
        }
        return IM_COL32(static_cast<int>(r * 255), static_cast<int>(g * 255), static_cast<int>(b * 255), 255);
    }

    static inline bool ok(const Vector3& v) {
        return v.x > 0.0f && v.y > 0.0f;
    }

    static constexpr ImU32 C_SHADOW = IM_COL32(0, 0, 0, 180);
    static constexpr ImU32 C_BORDER = IM_COL32(255, 255, 255, 40);

    struct PlayerBox {
        ImVec2 tl, br;
        bool   valid;
    };

    /**
     * @brief Computes 2D bounding box directly from Head and Toe screen positions.
     *        No skeleton bone traversal required.
     */
    static inline PlayerBox compute_player_box(const data::PlayerData& p) {
        PlayerBox box{ {0, 0}, {0, 0}, false };
        if (!ok(p.Head) || !ok(p.Toe)) return box;

        float top_y = ImMin(p.Head.y, p.Toe.y);
        float bot_y = ImMax(p.Head.y, p.Toe.y);
        float h = bot_y - top_y;
        if (h < 10.0f) h = 10.0f;

        float w = h * 0.48f;
        float cx = (p.Head.x + p.Toe.x) * 0.5f;

        box.tl = ImVec2(cx - w * 0.5f, top_y - h * 0.08f);
        box.br = ImVec2(cx + w * 0.5f, bot_y + h * 0.04f);
        box.valid = true;
        return box;
    }

    /**
     * @brief Draws bounding box (Corner, Full, or Glow).
     */
    static void draw_box(ImDrawList* dl, ImVec2 tl, ImVec2 br, ImU32 col, int type) {
        float thick = cfg::esp::box_thickness > 0.5f ? cfg::esp::box_thickness : 1.8f;
        float round = cfg::esp::box_rounding;

        if (type == 0) {
            float len = (br.x - tl.x) * (cfg::esp::bracket_frac > 0.05f ? cfg::esp::bracket_frac : 0.28f);
            ImVec2 pts[8][2] = {
                {tl,                         ImVec2(tl.x + len, tl.y)},
                {tl,                         ImVec2(tl.x, tl.y + len)},
                {ImVec2(br.x - len, tl.y),   ImVec2(br.x, tl.y)},
                {ImVec2(br.x, tl.y),         ImVec2(br.x, tl.y + len)},
                {ImVec2(tl.x, br.y - len),   ImVec2(tl.x, br.y)},
                {ImVec2(tl.x, br.y),         ImVec2(tl.x + len, br.y)},
                {ImVec2(br.x - len, br.y),   ImVec2(br.x, br.y)},
                {ImVec2(br.x, br.y - len),   ImVec2(br.x, br.y)}
            };
            for (auto& s : pts) {
                dl->AddLine(s[0], s[1], with_alpha(col, 0.25f), thick + 3.0f);
                dl->AddLine(ImVec2(s[0].x + 1, s[0].y + 1), ImVec2(s[1].x + 1, s[1].y + 1), C_SHADOW, thick + 1.0f);
                dl->AddLine(s[0], s[1], col, thick);
            }
        } else if (type == 1) {
            dl->AddRect(ImVec2(tl.x + 1, tl.y + 1), ImVec2(br.x + 1, br.y + 1),
                        IM_COL32(0, 0, 0, 160), round, 0, thick + 1.5f);
            dl->AddRect(tl, br, col, round, 0, thick);
        } else {
            dl->AddRect(tl, br, with_alpha(col, 0.18f), round, 0, thick + 4.0f);
            dl->AddRect(tl, br, with_alpha(col, 0.45f), round, 0, thick + 2.0f);
            dl->AddRect(tl, br, col, round, 0, thick);
            dl->AddRectFilled(tl, br, with_alpha(col, 0.08f), round);
        }
    }

    /**
     * @brief Draws clean horizontal health bar directly above bounding box.
     */
    static void draw_health_bar(ImDrawList* dl, ImVec2 tl, ImVec2 br, int cur_hp, int max_hp) {
        float hp01 = max_hp > 0 ? static_cast<float>(cur_hp) / static_cast<float>(max_hp) : 0.0f;
        hp01 = ImClamp(hp01, 0.0f, 1.0f);

        float bar_h = 3.0f;
        float bar_y = tl.y - bar_h - 3.0f;
        float w = br.x - tl.x;

        dl->AddRectFilled(ImVec2(tl.x, bar_y), ImVec2(br.x, bar_y + bar_h),
                          IM_COL32(0, 0, 0, 210), bar_h * 0.5f);
        dl->AddRectFilled(ImVec2(tl.x, bar_y), ImVec2(tl.x + w * hp01, bar_y + bar_h),
                          hp_color(hp01), bar_h * 0.5f);
        dl->AddRect(ImVec2(tl.x, bar_y), ImVec2(br.x, bar_y + bar_h),
                    C_BORDER, bar_h * 0.5f, 0, 1.0f);
    }

    /**
     * @brief Main ESP dispatch loop: Renders strictly ESP Line, ESP Box, and ESP Health.
     */
    void render_all() {
        bool any_esp = cfg::esp::enabled || cfg::esp::box || cfg::esp::tracer || cfg::esp::health;
        if (!any_esp || !data::g_in_match.load()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        ImGuiIO& io = ImGui::GetIO();
        float sw = (io.DisplaySize.x > 0.0f) ? io.DisplaySize.x : (display::width > 0.0f ? display::width : 2400.0f);
        ImVec2 anchor(sw * 0.5f, 0.0f);

        std::vector<data::PlayerData> snapshot;
        {
            std::lock_guard<std::mutex> lk(data::g_players_mutex);
            snapshot = data::g_players;
        }

        for (const auto& p : snapshot) {
            if (!ok(p.Head)) continue;
            if (p.distance > cfg::esp::max_range) continue;

            ImU32 base_col = p.isVisible ? to_u32(cfg::esp::box_visible) : to_u32(cfg::esp::box_col);
            ImU32 box_col  = fade_dist(base_col, p.distance, cfg::esp::max_range);

            // 1. ESP Line (Tracer from top-center screen to enemy head)
            if (cfg::esp::tracer) {
                ImVec2 head(p.Head.x, p.Head.y);
                float aa = ImClamp(1.0f - p.distance / cfg::esp::max_range, 0.25f, 1.0f);
                dl->AddLine(anchor, head, with_alpha(to_u32(cfg::esp::box_col), 0.35f * aa), 4.0f);
                dl->AddLine(anchor, head, with_alpha(to_u32(cfg::esp::box_col), 0.95f * aa), 1.8f);
            }

            // 2. Compute 2D Box from Head and Toe
            PlayerBox box = compute_player_box(p);
            if (!box.valid) continue;

            // 3. ESP Box
            if (cfg::esp::box) {
                draw_box(dl, box.tl, box.br, box_col, cfg::esp::box_type);
            }

            // 4. ESP Health Bar
            if (cfg::esp::health) {
                draw_health_bar(dl, box.tl, box.br, p.curHP, p.maxHP);
            }
        }
    }

} // namespace engine::esp
