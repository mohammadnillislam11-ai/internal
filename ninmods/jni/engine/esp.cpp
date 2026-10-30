#define IMGUI_DEFINE_MATH_OPERATORS
#include "esp.hpp"
#include "game_data.hpp"
#include "menu/config.hpp"
#include "menu/style.hpp"
#include "display_metrics.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdio>
#include <algorithm>

namespace engine::esp {

    /** @brief Converts an ImVec4 RGBA float color to ImU32 with optional alpha multiplier. */
    static inline ImU32 to_u32(const ImVec4& c, float alpha_mul = 1.0f) {
        return IM_COL32(
            static_cast<int>(c.x * 255.0f),
            static_cast<int>(c.y * 255.0f),
            static_cast<int>(c.z * 255.0f),
            static_cast<int>(c.w * 255.0f * alpha_mul)
        );
    }

    /** @brief Returns a copy of an ImU32 color with its alpha channel scaled by mul. */
    static inline ImU32 with_alpha(ImU32 c, float mul) {
        int a = static_cast<int>(((c >> 24) & 0xFF) * mul);
        if (a < 0) a = 0; if (a > 255) a = 255;
        return (c & 0x00FFFFFF) | (static_cast<ImU32>(a) << 24);
    }

    struct BoundingBox {
        ImVec2 min_pos;
        ImVec2 max_pos;
        bool   valid;
    };

    /**
     * @brief Calculates a 2D screen-space bounding box from a player's bone screen positions.
     * @param p PlayerData containing projected bone positions.
     * @return BoundingBox with valid=true when sufficient bone data is available.
     */
    static BoundingBox calculate_bounds(const data::PlayerData& p) {
        BoundingBox bb{ {0,0}, {0,0}, false };

        const Vector3* points[] = {
            &p.Head, &p.Breast, &p.Hip,
            &p.LShoulder, &p.RShoulder,
            &p.LForeArm,  &p.RForeArm,
            &p.LHand,     &p.RHand,
            &p.LAnkle,    &p.RAnkle,
            &p.LToe,      &p.RToe
        };

        float min_x = 9999.0f, min_y = 9999.0f;
        float max_x = -9999.0f, max_y = -9999.0f;
        int count = 0;

        for (const auto* pt : points) {
            if (pt->x > 0.0f && pt->y > 0.0f) {
                min_x = ImMin(min_x, pt->x);
                max_x = ImMax(max_x, pt->x);
                min_y = ImMin(min_y, pt->y);
                max_y = ImMax(max_y, pt->y);
                count++;
            }
        }

        if (count < 2) {
            if (p.Head.x > 0.0f && p.Toe.x > 0.0f) {
                float h = fabsf(p.Toe.y - p.Head.y);
                if (h > 5.0f) {
                    float w = h * 0.45f;
                    float mid_x = (p.Head.x + p.Toe.x) * 0.5f;
                    bb.min_pos = ImVec2(mid_x - w * 0.5f, ImMin(p.Head.y, p.Toe.y) - 10.0f);
                    bb.max_pos = ImVec2(mid_x + w * 0.5f, ImMax(p.Head.y, p.Toe.y) + 5.0f);
                    bb.valid = true;
                    return bb;
                }
            }
            return bb;
        }

        float pad_x = ImClamp((max_x - min_x) * 0.15f, 6.0f, 25.0f);
        float pad_y = 12.0f;

        bb.min_pos = ImVec2(min_x - pad_x, min_y - pad_y);
        bb.max_pos = ImVec2(max_x + pad_x, max_y + 6.0f);
        bb.valid = true;
        return bb;
    }

    /**
     * @brief Draws a corner or full 2D bounding box around a player.
     * @param dl        ImGui draw list for the current foreground layer.
     * @param tl        Top-left screen coordinate of the box.
     * @param br        Bottom-right screen coordinate of the box.
     * @param col       Box outline color (ImU32).
     * @param style     0 = corner brackets, 1 = full rectangle.
     * @param thickness Line thickness in pixels.
     * @param rounding  Corner rounding radius (full box only).
     */
    static void draw_box(ImDrawList* dl, ImVec2 tl, ImVec2 br, ImU32 col, int style, float thickness, float rounding) {
        if (style == 0) {
            float w = br.x - tl.x;
            float h = br.y - tl.y;
            float line_w = ImMin(w * 0.25f, 20.0f);
            float line_h = ImMin(h * 0.20f, 20.0f);

            ImU32 shadow = IM_COL32(0, 0, 0, 160);
            dl->AddLine(ImVec2(tl.x - 1, tl.y), ImVec2(tl.x + line_w + 1, tl.y), shadow, thickness + 1.5f);
            dl->AddLine(ImVec2(tl.x, tl.y - 1), ImVec2(tl.x, tl.y + line_h + 1), shadow, thickness + 1.5f);

            dl->AddLine(tl, ImVec2(tl.x + line_w, tl.y), col, thickness);
            dl->AddLine(tl, ImVec2(tl.x, tl.y + line_h), col, thickness);

            dl->AddLine(ImVec2(br.x, tl.y), ImVec2(br.x - line_w, tl.y), col, thickness);
            dl->AddLine(ImVec2(br.x, tl.y), ImVec2(br.x, tl.y + line_h), col, thickness);

            dl->AddLine(ImVec2(tl.x, br.y), ImVec2(tl.x + line_w, br.y), col, thickness);
            dl->AddLine(ImVec2(tl.x, br.y), ImVec2(tl.x, br.y - line_h), col, thickness);

            dl->AddLine(br, ImVec2(br.x - line_w, br.y), col, thickness);
            dl->AddLine(br, ImVec2(br.x, br.y - line_h), col, thickness);
        } else {
            dl->AddRect(ImVec2(tl.x - 1, tl.y - 1), ImVec2(br.x + 1, br.y + 1), IM_COL32(0, 0, 0, 180), rounding, 0, thickness + 1.5f);
            dl->AddRect(tl, br, col, rounding, 0, thickness);
        }
    }

    /**
     * @brief Renders a skeleton overlay by connecting bone screen positions with hairline segments.
     * @param dl  ImGui draw list.
     * @param p   PlayerData containing projected bone coordinates.
     * @param col Skeleton line color (ImU32).
     */
    static void draw_skeleton(ImDrawList* dl, const data::PlayerData& p, ImU32 col) {
        auto valid = [](const Vector3& v) { return v.x > 0.0f && v.y > 0.0f; };
        auto line = [&](const Vector3& a, const Vector3& b) {
            if (valid(a) && valid(b)) {
                dl->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), col, 1.0f);
            }
        };

        line(p.Head, p.Breast);
        line(p.Breast, p.Hip);

        line(p.Breast, p.LShoulder);
        line(p.LShoulder, p.LForeArm);
        line(p.LForeArm, p.LHand);

        line(p.Breast, p.RShoulder);
        line(p.RShoulder, p.RForeArm);
        line(p.RForeArm, p.RHand);

        line(p.Hip, p.LAnkle);
        line(p.LAnkle, p.LToe);

        line(p.Hip, p.RAnkle);
        line(p.RAnkle, p.RToe);

        if (valid(p.Head)) {
            dl->AddCircleFilled(ImVec2(p.Head.x, p.Head.y), 3.0f, col, 12);
        }
    }

    /**
     * @brief Draws vertical health and optional armor bars to the left of the bounding box.
     * @param dl  ImGui draw list.
     * @param p   PlayerData with curHP, maxHP, curArmor.
     * @param tl  Top-left corner of the bounding box.
     * @param br  Bottom-right corner of the bounding box.
     */
    static void draw_health_and_armor(ImDrawList* dl, const data::PlayerData& p, ImVec2 tl, ImVec2 br) {
        float box_h = br.y - tl.y;
        float bar_w = 4.0f;
        float bar_x = tl.x - bar_w - 5.0f;

        float hp_pct = ImClamp(static_cast<float>(p.curHP) / static_cast<float>(p.maxHP > 0 ? p.maxHP : 200), 0.0f, 1.0f);
        float hp_fill_h = box_h * hp_pct;

        dl->AddRectFilled(ImVec2(bar_x, tl.y), ImVec2(bar_x + bar_w, br.y), IM_COL32(20, 20, 20, 180), 2.0f);
        ImU32 hp_col = (hp_pct > 0.5f) ? IM_COL32(40, 220, 80, 255) : ((hp_pct > 0.25f) ? IM_COL32(240, 180, 30, 255) : IM_COL32(230, 40, 40, 255));
        dl->AddRectFilled(ImVec2(bar_x, br.y - hp_fill_h), ImVec2(bar_x + bar_w, br.y), hp_col, 2.0f);

        if (cfg::esp::armor && p.curArmor > 0) {
            float arm_x = bar_x - bar_w - 3.0f;
            float arm_pct = ImClamp(static_cast<float>(p.curArmor) / 100.0f, 0.0f, 1.0f);
            float arm_fill_h = box_h * arm_pct;

            dl->AddRectFilled(ImVec2(arm_x, tl.y), ImVec2(arm_x + bar_w, br.y), IM_COL32(20, 20, 20, 180), 2.0f);
            dl->AddRectFilled(ImVec2(arm_x, br.y - arm_fill_h), ImVec2(arm_x + bar_w, br.y), IM_COL32(60, 160, 255, 255), 2.0f);
        }
    }

    /**
     * @brief Renders name and/or distance badge above the bounding box.
     * @param dl       ImGui draw list.
     * @param p        PlayerData with name and distance fields.
     * @param center_x Horizontal center of the bounding box.
     * @param top_y    Top Y coordinate of the bounding box.
     */
    static void draw_info_panel(ImDrawList* dl, const data::PlayerData& p, float center_x, float top_y) {
        char buf[64];
        if (cfg::esp::name && cfg::esp::distance) {
            snprintf(buf, sizeof(buf), "%s [%.0fm]", p.name, p.distance);
        } else if (cfg::esp::name) {
            snprintf(buf, sizeof(buf), "%s", p.name);
        } else if (cfg::esp::distance) {
            snprintf(buf, sizeof(buf), "[%.0fm]", p.distance);
        } else {
            return;
        }

        ImVec2 text_sz = ImGui::CalcTextSize(buf);
        float badge_x = center_x - (text_sz.x * 0.5f);
        float badge_y = top_y - text_sz.y - 6.0f;

        dl->AddRectFilled(ImVec2(badge_x - 6.0f, badge_y - 2.0f),
                          ImVec2(badge_x + text_sz.x + 6.0f, badge_y + text_sz.y + 2.0f),
                          IM_COL32(15, 10, 25, 200), 4.0f);

        ImU32 text_col = p.isBot ? IM_COL32(180, 180, 180, 255) : to_u32(cfg::esp::name_col);
        dl->AddText(ImVec2(badge_x, badge_y), text_col, buf);
    }

    /**
     * @brief Main ESP render dispatcher. Called once per ImGui frame.
     *        All rendering is gated behind cfg::esp::enabled and g_in_match state.
     *        Tracer lines originate from screen center-bottom (player feet anchor) giving
     *        360-degree directional coverage regardless of where the enemy appears on screen.
     */
    void render_all() {
        if (!cfg::esp::enabled || !data::g_in_match.load()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        float sw = static_cast<float>(display::width > 0 ? display::width : 2400);
        float sh = static_cast<float>(display::height > 0 ? display::height : 1080);
        ImVec2 feet_anchor(sw * 0.5f, sh);

        std::vector<data::PlayerData> snapshot;
        {
            std::lock_guard<std::mutex> lk(data::g_players_mutex);
            snapshot = data::g_players;
        }

        for (const auto& p : snapshot) {
            if (p.Head.x < 0.0f || p.Head.y < 0.0f) continue;
            if (p.distance > cfg::esp::max_range) continue;

            BoundingBox bb = calculate_bounds(p);
            if (!bb.valid) continue;

            float cx = (bb.min_pos.x + bb.max_pos.x) * 0.5f;
            ImU32 box_col = p.isVisible ? to_u32(cfg::esp::box_visible) : to_u32(cfg::esp::box_col);

            if (cfg::esp::tracer) {
                float dist_alpha = ImClamp(1.0f - (p.distance / cfg::esp::max_range), 0.25f, 0.9f);
                dl->AddLine(feet_anchor, ImVec2(p.Head.x, p.Head.y), with_alpha(box_col, dist_alpha), 0.8f);
            }

            if (cfg::esp::box) {
                draw_box(dl, bb.min_pos, bb.max_pos, box_col, cfg::esp::box_type, cfg::esp::box_thickness, cfg::esp::box_rounding);
            }

            if (cfg::esp::skeleton) {
                draw_skeleton(dl, p, box_col);
            }

            if (cfg::esp::health) {
                draw_health_and_armor(dl, p, bb.min_pos, bb.max_pos);
            }

            if (cfg::esp::name || cfg::esp::distance) {
                draw_info_panel(dl, p, cx, bb.min_pos.y);
            }

            if (cfg::esp::firing_ind && p.isFiring) {
                dl->AddCircleFilled(ImVec2(cx, bb.min_pos.y - 18.0f), 6.0f, IM_COL32(255, 60, 20, 255));
                dl->AddCircle(ImVec2(cx, bb.min_pos.y - 18.0f), 9.0f, IM_COL32(255, 140, 20, 200), 16, 2.0f);
            }
        }

        if (cfg::esp::loot) {
            std::vector<data::LootItem> loot_snap;
            {
                std::lock_guard<std::mutex> lk(data::g_loots_mutex);
                loot_snap = data::g_loots;
            }
            for (const auto& item : loot_snap) {
                char loot_buf[32];
                snprintf(loot_buf, sizeof(loot_buf), "Item [%.0fm]", item.distance);
                dl->AddText(ImVec2(item.screenPos.x, item.screenPos.y), IM_COL32(255, 215, 0, 220), loot_buf);
            }
        }
    }

} // namespace engine::esp
