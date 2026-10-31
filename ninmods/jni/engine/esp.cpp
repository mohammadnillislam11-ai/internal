#define IMGUI_DEFINE_MATH_OPERATORS
#include "esp.hpp"
#include "game_data.hpp"
#include "menu/config.hpp"
#include "menu/style.hpp"
#include "display_metrics.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace engine::esp {

    /** @brief Converts ImVec4 color to 32-bit packed RGBA with alpha multiplier. */
    static inline ImU32 to_u32(const ImVec4& c, float alpha_mul = 1.0f) {
        return IM_COL32(
            static_cast<int>(c.x * 255.0f),
            static_cast<int>(c.y * 255.0f),
            static_cast<int>(c.z * 255.0f),
            static_cast<int>(c.w * 255.0f * alpha_mul)
        );
    }

    /** @brief Adjusts opacity of an ImU32 color. */
    static inline ImU32 with_alpha(ImU32 c, float mul) {
        int a = static_cast<int>(((c >> 24) & 0xFF) * mul);
        if (a < 0) a = 0; if (a > 255) a = 255;
        return (c & 0x00FFFFFF) | (static_cast<ImU32>(a) << 24);
    }

    /** @brief Fades color based on distance ratio. */
    static inline ImU32 fade_dist(ImU32 base, float dist, float max_range) {
        float t = ImClamp(1.0f - dist / (max_range > 0.0f ? max_range : 400.0f), 0.35f, 1.0f);
        return with_alpha(base, t);
    }

    /** @brief Dynamic gradient color mapping for health percentage (green -> yellow -> red). */
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

    /** @brief Gradient color mapping for body armor. */
    static inline ImU32 armor_color(float t) {
        t = ImClamp(t, 0.0f, 1.0f);
        return IM_COL32(static_cast<int>(80 + 100 * t), static_cast<int>(180 + 60 * t), 255, 255);
    }

    static inline bool ok(const Vector3& v) {
        return v.x > 0.0f && v.y > 0.0f;
    }

    static inline void fmt_dist(float d, char* out, size_t n) {
        if (d >= 1000.0f) snprintf(out, n, "%.1fkm", d / 1000.0f);
        else             snprintf(out, n, "%.0fm", d);
    }

    static inline void trunc_name(const char* src, char* out, size_t n) {
        if (!src || !src[0]) { out[0] = '\0'; return; }
        size_t len = strlen(src);
        if (len <= 12) { snprintf(out, n, "%s", src); return; }
        snprintf(out, n, "%.11s.", src);
    }

    static constexpr ImU32 C_SHADOW = IM_COL32(0, 0, 0, 180);
    static constexpr ImU32 C_BORDER = IM_COL32(255, 255, 255, 40);
    static constexpr float ESP_FONT_SIZE = 12.0f;
    static constexpr float PANEL_ROW_H   = ESP_FONT_SIZE + 5.0f;
    static constexpr float PANEL_TOP_OFFSET = 30.0f;

    struct SkeletonBox {
        ImVec2 tl, br;
        bool   valid;
    };

    /**
     * @brief Computes 2D screen bounding box dynamically from all player bone positions.
     */
    static inline SkeletonBox compute_skeleton_box(const data::PlayerData& p) {
        SkeletonBox sb{ {0,0}, {0,0}, false };

        const Vector3* pts[] = {
            &p.Head, &p.Breast, &p.Hip,
            &p.LShoulder, &p.RShoulder,
            &p.LForeArm,  &p.RForeArm,
            &p.LHand,     &p.RHand,
            &p.LAnkle,    &p.RAnkle,
            &p.LToe,      &p.RToe
        };

        float min_x =  1e9f, min_y =  1e9f;
        float max_x = -1e9f, max_y = -1e9f;
        int   cnt   = 0;

        for (auto* v : pts) {
            if (!ok(*v)) continue;
            if (v->x < min_x) min_x = v->x;
            if (v->x > max_x) max_x = v->x;
            if (v->y < min_y) min_y = v->y;
            if (v->y > max_y) max_y = v->y;
            cnt++;
        }

        if (cnt < 4) {
            if (ok(p.Head) && ok(p.Toe)) {
                min_y = ImMin(p.Head.y, p.Toe.y);
                max_y = ImMax(p.Head.y, p.Toe.y);
                float box_h = max_y - min_y;
                float half_w = (box_h > 10.0f) ? (box_h * 0.28f) : 15.0f;
                float center_x = (p.Head.x + p.Toe.x) * 0.5f;
                min_x = center_x - half_w;
                max_x = center_x + half_w;
                sb.valid = true;
            }
            if (!sb.valid) return sb;
        } else {
            sb.valid = true;
        }

        const float pad_x = 4.0f;
        const float pad_y = 3.0f;

        sb.tl = ImVec2(min_x - pad_x, min_y - pad_y);
        sb.br = ImVec2(max_x + pad_x, max_y + pad_y);
        return sb;
    }

    /**
     * @brief Renders the floating name, distance, AI/PL badge, and mini health/armor bars.
     */
    static void draw_name_panel(ImDrawList* dl, const data::PlayerData& p, float cx, float top_y) {
        if (!cfg::esp::name && !cfg::esp::distance && !cfg::esp::health && !cfg::esp::armor) return;

        char name_buf[24];
        trunc_name(p.name, name_buf, sizeof(name_buf));
        if (name_buf[0] == '\0') {
            snprintf(name_buf, sizeof(name_buf), "%s", p.isBot ? "BOT" : "Player");
        }

        char dist_buf[16];
        fmt_dist(p.distance, dist_buf, sizeof(dist_buf));

        ImU32 name_col = to_u32(cfg::esp::name_col);
        ImU32 dist_col = to_u32(cfg::esp::distance_col);
        ImU32 vis_tag  = IM_COL32(30, 100, 220, 240);
        ImU32 bot_tag  = IM_COL32(200, 40, 40, 240);

        const char* tag = cfg::esp::bot_tag ? (p.isBot ? "AI" : "PL") : nullptr;
        ImU32 tag_bg = p.isBot ? bot_tag : vis_tag;

        ImU32 bg_main    = p.isVisible ? IM_COL32(12, 12, 16, 235) : IM_COL32(35, 35, 42, 215);
        ImU32 text_color = p.isVisible ? name_col : with_alpha(name_col, 0.65f);

        const float fs    = ESP_FONT_SIZE;
        const float pad_x = 5.0f;
        const float pad_y = 2.0f;
        const float gap   = 3.0f;

        float tag_w  = tag ? ImGui::CalcTextSize(tag).x + pad_x * 2.0f : 0.0f;
        float name_w = cfg::esp::name ? ImGui::CalcTextSize(name_buf).x + pad_x * 2.0f : 0.0f;
        float dist_w = cfg::esp::distance ? ImGui::CalcTextSize(dist_buf).x + pad_x * 2.0f : 0.0f;

        float row_h = PANEL_ROW_H;
        float total_w = tag_w;
        if (name_w > 0.0f) total_w += (total_w > 0.0f ? gap : 0.0f) + name_w;
        if (dist_w > 0.0f) total_w += (total_w > 0.0f ? gap : 0.0f) + dist_w;
        if (total_w <= 0.0f) return;

        float bx = cx - total_w * 0.5f;
        float by = top_y - row_h - PANEL_TOP_OFFSET;

        dl->AddRectFilled(ImVec2(bx + 1, by + 1), ImVec2(bx + total_w + 1, by + row_h + 1), C_SHADOW, 4.0f);
        dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + total_w, by + row_h), bg_main, 4.0f);
        dl->AddRect(ImVec2(bx, by), ImVec2(bx + total_w, by + row_h), C_BORDER, 4.0f, 0, 1.0f);

        float cursor_x = bx;

        if (tag) {
            dl->AddRectFilled(ImVec2(cursor_x, by), ImVec2(cursor_x + tag_w, by + row_h),
                              with_alpha(tag_bg, p.isVisible ? 1.0f : 0.65f), 4.0f, ImDrawFlags_RoundCornersLeft);
            ImVec2 ts = ImGui::CalcTextSize(tag);
            dl->AddText(ImGui::GetFont(), fs, ImVec2(cursor_x + (tag_w - ts.x) * 0.5f, by + pad_y),
                        IM_COL32(255, 255, 255, 255), tag);
            cursor_x += tag_w;
        }

        if (cfg::esp::name) {
            if (tag) cursor_x += gap;
            ImVec2 ts = ImGui::CalcTextSize(name_buf);
            dl->AddText(ImGui::GetFont(), fs, ImVec2(cursor_x + (name_w - ts.x) * 0.5f, by + pad_y), text_color, name_buf);
            cursor_x += name_w;
        }

        if (cfg::esp::distance) {
            if (cursor_x > bx) cursor_x += gap;
            ImVec2 ts = ImGui::CalcTextSize(dist_buf);
            ImU32 d_col = p.isVisible ? dist_col : with_alpha(dist_col, 0.65f);
            dl->AddText(ImGui::GetFont(), fs, ImVec2(cursor_x + (dist_w - ts.x) * 0.5f, by + pad_y), d_col, dist_buf);
        }

        float bar_y = by + row_h + 2.0f;
        float bar_h = 3.0f;
        float bar_offset = 0.0f;

        if (cfg::esp::health) {
            float hp01 = p.maxHP > 0 ? static_cast<float>(p.curHP) / static_cast<float>(p.maxHP) : 0.0f;
            hp01 = ImClamp(hp01, 0.0f, 1.0f);

            dl->AddRectFilled(ImVec2(bx, bar_y), ImVec2(bx + total_w, bar_y + bar_h),
                              IM_COL32(0, 0, 0, 210), bar_h * 0.5f);
            dl->AddRectFilled(ImVec2(bx, bar_y), ImVec2(bx + total_w * hp01, bar_y + bar_h),
                              hp_color(hp01), bar_h * 0.5f);
            dl->AddRect(ImVec2(bx, bar_y), ImVec2(bx + total_w, bar_y + bar_h),
                        C_BORDER, bar_h * 0.5f, 0, 1.0f);
            bar_offset = bar_h + 1.0f;
        }

        if (cfg::esp::armor && p.curArmor > 0) {
            float arm01 = ImClamp(static_cast<float>(p.curArmor) / 100.0f, 0.0f, 1.0f);
            float arm_y  = bar_y + bar_offset;
            float arm_h  = 2.5f;

            dl->AddRectFilled(ImVec2(bx, arm_y), ImVec2(bx + total_w, arm_y + arm_h),
                              IM_COL32(0, 0, 0, 210), arm_h * 0.5f);
            dl->AddRectFilled(ImVec2(bx, arm_y), ImVec2(bx + total_w * arm01, arm_y + arm_h),
                              armor_color(arm01), arm_h * 0.5f);
        }
    }

    /**
     * @brief Draws bounding box with corner bracket, rounded rectangle, or translucent filled styling.
     */
    static void draw_box(ImDrawList* dl, ImVec2 tl, ImVec2 br, ImU32 col, int type) {
        float thick = cfg::esp::box_thickness > 0.5f ? cfg::esp::box_thickness : 1.5f;
        float round = cfg::esp::box_rounding;

        if (type == 0) {
            float len = (br.x - tl.x) * (cfg::esp::bracket_frac > 0.05f ? cfg::esp::bracket_frac : 0.25f);
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
            dl->AddRect(ImVec2(tl.x + 2, tl.y + 2), ImVec2(br.x + 2, br.y + 2),
                        IM_COL32(0, 0, 0, 120), round, 0, thick + 3.0f);
            dl->AddRect(tl, br, with_alpha(col, 0.35f), round, 0, thick + 2.0f);
            dl->AddRect(tl, br, col, round, 0, thick);
        } else {
            dl->AddRect(tl, br, with_alpha(col, 0.18f), round, 0, thick + 5.0f);
            dl->AddRect(tl, br, with_alpha(col, 0.45f), round, 0, thick + 2.0f);
            dl->AddRect(tl, br, col, round, 0, thick);
            dl->AddRectFilled(tl, br, with_alpha(col, 0.06f), round);
        }
    }

    static inline void s_line(ImDrawList* dl, const Vector3& a, const Vector3& b, ImU32 col, float t = 1.6f) {
        if (!ok(a) || !ok(b)) return;
        dl->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), with_alpha(col, 0.25f), t + 2.0f);
        dl->AddLine(ImVec2(a.x, a.y), ImVec2(b.x, b.y), col, t);
    }

    static inline void s_dot(ImDrawList* dl, const Vector3& v, ImU32 col, float r = 2.4f) {
        if (!ok(v)) return;
        dl->AddCircleFilled(ImVec2(v.x, v.y), r + 2.0f, with_alpha(col, 0.25f));
        dl->AddCircleFilled(ImVec2(v.x, v.y), r, col);
    }

    /**
     * @brief Connects bone hierarchy with dual-pass antialiased lines and glowing joints.
     */
    static void draw_skeleton(ImDrawList* dl, const data::PlayerData& p, ImU32 col) {
        s_line(dl, p.Head, p.Breast, col, 1.8f);
        s_line(dl, p.Breast, p.Hip, col, 1.8f);

        s_line(dl, p.Breast, p.LShoulder, col);
        s_line(dl, p.Breast, p.RShoulder, col);
        s_line(dl, p.LShoulder, p.LForeArm, col);
        s_line(dl, p.RShoulder, p.RForeArm, col);
        s_line(dl, p.LForeArm, p.LHand, col);
        s_line(dl, p.RForeArm, p.RHand, col);

        s_line(dl, p.Hip, p.LAnkle, col, 1.8f);
        s_line(dl, p.Hip, p.RAnkle, col, 1.8f);
        s_line(dl, p.LAnkle, p.LToe, col);
        s_line(dl, p.RAnkle, p.RToe, col);

        s_dot(dl, p.Head, col, 2.8f);
        s_dot(dl, p.Breast, col, 2.4f);
        s_dot(dl, p.Hip, col, 2.4f);
        s_dot(dl, p.LShoulder, col, 2.0f);
        s_dot(dl, p.RShoulder, col, 2.0f);
        s_dot(dl, p.LHand, col, 2.0f);
        s_dot(dl, p.RHand, col, 2.0f);
        s_dot(dl, p.LAnkle, col, 2.0f);
        s_dot(dl, p.RAnkle, col, 2.0f);
    }

    /**
     * @brief Renders pulsating alert ring when enemy is firing.
     */
    static void draw_firing(ImDrawList* dl, const data::PlayerData& p, float cx, float ty) {
        if (!p.isFiring) return;
        float t = static_cast<float>(ImGui::GetTime());
        float pulse = 0.5f + 0.5f * sinf(t * 12.0f);
        float r = 3.0f + pulse * 1.2f;
        ImVec2 pos(cx, ty - 20.0f);

        dl->AddCircleFilled(pos, r + 4.0f, IM_COL32(255, 200, 40, static_cast<int>(70 * pulse)));
        dl->AddCircleFilled(pos, r, IM_COL32(255, 200, 40, 255));
        dl->AddCircle(pos, r + 1.0f, IM_COL32(255, 255, 255, 210), 16, 1.2f);
    }

    /**
     * @brief Draws clean weapon category pill badge under feet.
     */
    static void draw_weapon(ImDrawList* dl, const data::PlayerData& p, ImVec2 foot) {
        if (p.weaponType < 0) return;
        static const char* k_names[] = {
            "AR", "SNIPER", "SMG", "SHOTGUN", "PISTOL", "MELEE",
            "GRENADE", "LMG", "RIFLE", "MARKSMAN", "SPECIAL"
        };
        const char* wn = (p.weaponType < static_cast<int>(sizeof(k_names)/sizeof(*k_names)))
                         ? k_names[p.weaponType] : "WPN";

        const float fs    = ESP_FONT_SIZE - 2.0f;
        const float pad_x = 6.0f;
        const float pad_y = 3.0f;

        ImVec2 ts = ImGui::CalcTextSize(wn);
        float pw = ts.x + pad_x * 2.0f;
        float ph = fs + pad_y * 2.0f;
        float px = foot.x - pw * 0.5f;
        float py = foot.y + 4.0f;

        ImU32 w_col = to_u32(cfg::esp::weapon_col);
        ImU32 bg    = p.isVisible ? IM_COL32(0, 0, 0, 210) : IM_COL32(0, 0, 0, 150);

        dl->AddRectFilled(ImVec2(px + 1, py + 1), ImVec2(px + pw + 1, py + ph + 1), C_SHADOW, 5.0f);
        dl->AddRectFilled(ImVec2(px, py), ImVec2(px + pw, py + ph), bg, 5.0f);
        dl->AddRect(ImVec2(px, py), ImVec2(px + pw, py + ph), C_BORDER, 5.0f, 0, 1.0f);
        dl->AddText(ImGui::GetFont(), fs, ImVec2(px + pad_x, py + pad_y), w_col, wn);
    }

    static inline const char* loot_category(uint32_t resID) {
        uint32_t c = (resID >> 24) & 0xFF;
        switch (c) {
            case 0x01: return "GUN";
            case 0x02: return "AMMO";
            case 0x03: return "ARMOR";
            case 0x04: return "HEAL";
            case 0x05: return "NADE";
            case 0x06: return "BAG";
            default:   return "ITEM";
        }
    }

    static inline ImU32 loot_color(const char* cat) {
        if (strcmp(cat, "GUN")   == 0) return IM_COL32(255, 100, 100, 255);
        if (strcmp(cat, "AMMO")  == 0) return IM_COL32(255, 200, 80, 255);
        if (strcmp(cat, "ARMOR") == 0) return IM_COL32(80, 180, 255, 255);
        if (strcmp(cat, "HEAL")  == 0) return IM_COL32(80, 255, 120, 255);
        if (strcmp(cat, "NADE")  == 0) return IM_COL32(200, 80, 255, 255);
        if (strcmp(cat, "BAG")   == 0) return IM_COL32(160, 120, 60, 255);
        return IM_COL32(200, 200, 200, 255);
    }

    static inline void draw_crosshair(ImDrawList* dl, float cx, float cy) {
        float gap = 6.0f, len = 10.0f;
        ImU32 c = IM_COL32(220, 40, 40, 240);

        dl->AddLine(ImVec2(cx - gap - len, cy), ImVec2(cx - gap, cy), c, 1.8f);
        dl->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + gap + len, cy), c, 1.8f);
        dl->AddLine(ImVec2(cx, cy - gap - len), ImVec2(cx, cy - gap), c, 1.8f);
        dl->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + gap + len), c, 1.8f);
        dl->AddCircleFilled(ImVec2(cx, cy), 1.5f, c);
    }

    /**
     * @brief Main ESP dispatch loop.
     */
    void render_all() {
        bool any_esp = cfg::esp::enabled || cfg::esp::box || cfg::esp::skeleton || cfg::esp::tracer ||
                       cfg::esp::name || cfg::esp::distance || cfg::esp::health || cfg::esp::armor ||
                       cfg::esp::firing_ind || cfg::esp::weapon || cfg::esp::loot || cfg::esp::crosshair;
        if (!any_esp) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        ImGuiIO& io = ImGui::GetIO();
        float sw = (io.DisplaySize.x > 0.0f) ? io.DisplaySize.x : (display::width > 0.0f ? display::width : 2400.0f);
        float sh = (io.DisplaySize.y > 0.0f) ? io.DisplaySize.y : (display::height > 0.0f ? display::height : 1080.0f);

        if (cfg::esp::crosshair) {
            draw_crosshair(dl, sw * 0.5f, sh * 0.5f);
        }

        if (!data::g_in_match.load()) return;

        ImVec2 anchor(sw * 0.5f, 0.0f);

        std::vector<data::PlayerData> snapshot;
        {
            std::lock_guard<std::mutex> lk(data::g_players_mutex);
            snapshot = data::g_players;
        }

        for (const auto& p : snapshot) {
            if (p.Head.x < 0.0f || p.Head.y < 0.0f) continue;
            if (p.distance > cfg::esp::max_range) continue;

            // Draw tracer immediately to head - does not require full skeleton box to be computed
            if (cfg::esp::tracer && ok(p.Head)) {
                ImVec2 head(p.Head.x, p.Head.y);
                float aa = ImClamp(1.0f - p.distance / cfg::esp::max_range, 0.25f, 1.0f);
                dl->AddLine(anchor, head, with_alpha(to_u32(cfg::esp::box_col), 0.35f * aa), 4.0f);
                dl->AddLine(anchor, head, with_alpha(to_u32(cfg::esp::box_col), 0.95f * aa), 1.8f);
            }

            SkeletonBox sb = compute_skeleton_box(p);
            if (!sb.valid) continue;

            float cx = (sb.tl.x + sb.br.x) * 0.5f;
            ImU32 base_col = p.isVisible ? to_u32(cfg::esp::box_visible) : to_u32(cfg::esp::box_col);
            ImU32 box_col  = fade_dist(base_col, p.distance, cfg::esp::max_range);

            if (cfg::esp::box) {
                draw_box(dl, sb.tl, sb.br, box_col, cfg::esp::box_type);
            }

            if (cfg::esp::skeleton) {
                draw_skeleton(dl, p, box_col);
            }

            draw_name_panel(dl, p, cx, sb.tl.y);

            if (cfg::esp::firing_ind) {
                draw_firing(dl, p, cx, sb.tl.y);
            }

            if (cfg::esp::weapon) {
                draw_weapon(dl, p, ImVec2(cx, sb.br.y));
            }
        }

        if (cfg::esp::loot) {
            std::vector<data::LootItem> loot_snap;
            {
                std::lock_guard<std::mutex> lk(data::g_loots_mutex);
                loot_snap = data::g_loots;
            }
            for (const auto& item : loot_snap) {
                const char* cat = loot_category(item.resID);
                ImU32 cat_col = loot_color(cat);

                char loot_buf[32];
                snprintf(loot_buf, sizeof(loot_buf), "[%s] %.0fm", cat, item.distance);

                ImVec2 ts = ImGui::CalcTextSize(loot_buf);
                float px = item.screenPos.x - ts.x * 0.5f;
                float py = item.screenPos.y;

                dl->AddRectFilled(ImVec2(px - 3, py - 1), ImVec2(px + ts.x + 3, py + ts.y + 1), IM_COL32(10, 10, 15, 200), 3.0f);
                dl->AddText(ImVec2(px, py), cat_col, loot_buf);
            }
        }
    }

} // namespace engine::esp
