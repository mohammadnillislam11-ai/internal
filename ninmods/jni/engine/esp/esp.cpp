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

        float headToFeetHeight = std::abs(p.Toe.y - p.Head.y);
        if (headToFeetHeight < 6.0f) return box;

        // NEXOMODS standard formula:
        float boxHeight = headToFeetHeight * 1.18f;
        float boxWidth  = boxHeight * 0.46f;

        // Horizontally centered strictly on Head.x (zero animation jitter)
        float boxX = p.Head.x - (boxWidth * 0.5f);
        float boxY = p.Head.y - (boxHeight * 0.10f);

        box.tl = ImVec2(boxX, boxY);
        box.br = ImVec2(boxX + boxWidth, boxY + boxHeight);
        box.valid = true;
        return box;
    }

    /**
     * @brief Draws bounding box (0 = Full Rect / চতুর্ভুজ বক্স, 1 = Corner / চার কোনা, 2 = Glow).
     */
    static void draw_box(ImDrawList* dl, ImVec2 tl, ImVec2 br, ImU32 col, int type) {
        float thick = cfg::esp::box_thickness > 0.5f ? cfg::esp::box_thickness : 1.8f;
        float round = cfg::esp::box_rounding;

        if (type == 0) {
            // চতুর্ভুজ বক্স (Full 2D Rectangular Bounding Box with contrast drop shadows)
            dl->AddRect(ImVec2(tl.x - 1.0f, tl.y - 1.0f), ImVec2(br.x + 1.0f, br.y + 1.0f),
                        IM_COL32(0, 0, 0, 210), round, 0, thick + 1.6f);
            dl->AddRect(ImVec2(tl.x + 1.0f, tl.y + 1.0f), ImVec2(br.x - 1.0f, br.y - 1.0f),
                        IM_COL32(0, 0, 0, 160), round, 0, thick);
            dl->AddRect(tl, br, col, round, 0, thick);
        } else if (type == 1) {
            // Corner Box (চার কোনা ব্র্যাকেট)
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
        } else {
            // Glow Box
            dl->AddRect(tl, br, with_alpha(col, 0.18f), round, 0, thick + 4.0f);
            dl->AddRect(tl, br, with_alpha(col, 0.45f), round, 0, thick + 2.0f);
            dl->AddRect(tl, br, col, round, 0, thick);
            dl->AddRectFilled(tl, br, with_alpha(col, 0.08f), round);
        }
    }

    /**
     * @brief Draws clean vertical health bar directly on the right side of the bounding box,
     *        matching the exact height of the box.
     */
    static void draw_health_bar(ImDrawList* dl, ImVec2 tl, ImVec2 br, int cur_hp, int max_hp) {
        float box_h = br.y - tl.y;
        if (box_h <= 2.0f) return;

        float hp01 = max_hp > 0 ? static_cast<float>(cur_hp) / static_cast<float>(max_hp) : 0.0f;
        hp01 = ImClamp(hp01, 0.0f, 1.0f);

        // Position directly on the right side of the bounding box
        float gap = 4.0f;
        float bar_w = 4.0f;
        float bar_x = br.x + gap;
        float top_y = tl.y;
        float bot_y = br.y;

        // Black outer contrast shadow
        dl->AddRectFilled(ImVec2(bar_x - 1.0f, top_y - 1.0f),
                          ImVec2(bar_x + bar_w + 1.0f, bot_y + 1.0f),
                          IM_COL32(0, 0, 0, 220), 1.5f);

        // Dark track background (depleted health)
        dl->AddRectFilled(ImVec2(bar_x, top_y),
                          ImVec2(bar_x + bar_w, bot_y),
                          IM_COL32(25, 20, 30, 220), 1.0f);

        // Active health fill from bottom to top according to remaining HP
        float filled_h = box_h * hp01;
        float fill_top_y = bot_y - filled_h;
        if (filled_h > 0.5f) {
            dl->AddRectFilled(ImVec2(bar_x, fill_top_y),
                              ImVec2(bar_x + bar_w, bot_y),
                              hp_color(hp01), 1.0f);
        }

        // Clean border outline
        dl->AddRect(ImVec2(bar_x, top_y),
                    ImVec2(bar_x + bar_w, bot_y),
                    C_BORDER, 1.0f, 0, 1.0f);
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
        float sh = (io.DisplaySize.y > 0.0f) ? io.DisplaySize.y : (display::height > 0.0f ? display::height : 1080.0f);
        ImVec2 anchor(sw * 0.5f, 0.0f);

        std::vector<data::PlayerData> snapshot;
        {
            std::lock_guard<std::mutex> lk(data::g_players_mutex);
            snapshot = data::g_players;
        }

        for (const auto& p : snapshot) {
            if (!ok(p.Head) || !ok(p.Toe)) continue;
            if (p.distance > cfg::esp::max_range) continue;

            ImU32 base_col = p.isVisible ? to_u32(cfg::esp::box_visible) : to_u32(cfg::esp::box_col);
            ImU32 box_col  = fade_dist(base_col, p.distance, cfg::esp::max_range);
            float aa = ImClamp(1.0f - p.distance / cfg::esp::max_range, 0.25f, 1.0f);

            if (p.isOnScreen) {
                PlayerBox box = compute_player_box(p);
                if (box.valid) {
                    // 1. ESP Line (Tracer line: NEXOMODS architecture attached directly to top of box)
                    if (cfg::esp::tracer) {
                        float centerX = (box.tl.x + box.br.x) * 0.5f;
                        float centerY = box.tl.y; // Attached to top edge of box
                        ImVec2 target_pt(centerX, centerY);

                        // Top anchor dot
                        dl->AddCircleFilled(anchor, 4.0f, box_col);
                        dl->AddCircle(anchor, 5.5f, IM_COL32(0, 0, 0, 180), 12, 1.2f);

                        // Distance adaptive thickness
                        float line_thick = (p.distance > 70.0f) ? 1.2f : 1.8f;
                        dl->AddLine(anchor, target_pt, with_alpha(box_col, 0.30f * aa), line_thick + 2.0f);
                        dl->AddLine(anchor, target_pt, with_alpha(box_col, 0.95f * aa), line_thick);
                    }

                    // 2. ESP Box (NEXOMODS style)
                    if (cfg::esp::box) {
                        draw_box(dl, box.tl, box.br, box_col, cfg::esp::box_type);
                    }

                    // 3. ESP Health Bar
                    if (cfg::esp::health) {
                        draw_health_bar(dl, box.tl, box.br, p.curHP, p.maxHP);
                    }
                }
            }
        }
    }

} // namespace engine::esp
