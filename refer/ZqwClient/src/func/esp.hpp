#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "../game/data.hpp"
#include "../ui/cfg.hpp"
#include "../ui/styles/styles.h"
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cmath>

namespace func::esp {

    /** Color and alpha helper utilities */
    inline ImU32 toU32(const ImVec4& c, float alphaMul = 1.f) {
        return IM_COL32(
            (int)(c.x * 255.f),
            (int)(c.y * 255.f),
            (int)(c.z * 255.f),
            (int)(c.w * 255.f * alphaMul));
    }

    inline ImU32 withAlpha(ImU32 c, float mul) {
        int a = (int)(((c >> 24) & 0xFF) * mul);
        if (a < 0) a = 0; if (a > 255) a = 255;
        return (c & 0x00FFFFFF) | ((ImU32)a << 24);
    }

    inline ImU32 fadeDist(ImU32 base, float dist, float maxRange) {
        float t = ImClamp(1.f - dist / maxRange, 0.40f, 1.f);
        return withAlpha(base, t);
    }

    inline ImU32 hpColor(float t) {
        t = ImClamp(t, 0.f, 1.f);
        float r, g, b;
        if (t < 0.5f) { float k = t * 2.f; r = 1.f; g = 0.35f + 0.55f * k; b = 0.15f; }
        else          { float k = (t - 0.5f) * 2.f; r = 1.f - 0.9f * k; g = 0.9f; b = 0.15f + 0.30f * k; }
        return IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
    }

    /** Vector validation and string formatting routines */
    inline bool ok(const Vector3& v) { return v.x > 0.f && v.y > 0.f; }

    inline void fmtDist(float d, char* out, size_t n) {
        if (d >= 1000.f) snprintf(out, n, "%.1fkm", d / 1000.f);
        else             snprintf(out, n, "%.0fm", d);
    }

    inline void truncName(const char* src, char* out, size_t n) {
        if (!src || !src[0]) { out[0] = 0; return; }
        size_t len = strlen(src);
        if (len <= 12) { snprintf(out, n, "%s", src); return; }
        snprintf(out, n, "%.11s.", src);
    }

    /** Visual constants */
    static constexpr ImU32 C_SHADOW = IM_COL32(0, 0, 0, 180);
    static constexpr ImU32 C_BORDER = IM_COL32(255, 255, 255, 40);
    static constexpr float ESP_FONT_SIZE = 12.f;
    static constexpr float PANEL_ROW_H   = ESP_FONT_SIZE + 4.f;
    static constexpr float PANEL_TOP_OFFSET = 18.f;

    /** Bounding box computed from bone joints */
    struct SkeletonBox {
        ImVec2 tl, br;
        bool   valid;
    };

    inline SkeletonBox computeSkeletonBox(const PlayerData& p) {
        SkeletonBox sb{ {0,0}, {0,0}, false };

        const Vector3* pts[] = {
            &p.Head,      &p.Breast,    &p.Hip,
            &p.LShoulder, &p.RShoulder,
            &p.LForeArm,  &p.RForeArm,
            &p.LHand,     &p.RHand,
            &p.LAnkle,    &p.RAnkle,
            &p.LToe,      &p.RToe
        };

        float minX =  1e9f, minY =  1e9f;
        float maxX = -1e9f, maxY = -1e9f;
        int   cnt  = 0;

        for (auto* v : pts) {
            if (!ok(*v)) continue;
            if (v->x < minX) minX = v->x;
            if (v->x > maxX) maxX = v->x;
            if (v->y < minY) minY = v->y;
            if (v->y > maxY) maxY = v->y;
            cnt++;
        }

        if (cnt < 4) {
            if (ok(p.Head) && ok(p.Toe)) {
                minX = ImMin(p.Head.x, p.Toe.x);
                maxX = ImMax(p.Head.x, p.Toe.x);
                minY = ImMin(p.Head.y, p.Toe.y);
                maxY = ImMax(p.Head.y, p.Toe.y);
                sb.valid = true;
            }
            if (!sb.valid) return sb;
        } else {
            sb.valid = true;
        }

        const float padX = 3.f;
        const float padY = 2.f;

        sb.tl = ImVec2(minX - padX, minY - padY);
        sb.br = ImVec2(maxX + padX, maxY + padY);
        return sb;
    }

    /**
     * @brief Renders clean name, distance, and health indicator above an entity.
     * @param dl ImGui foreground draw list.
     * @param p Player snapshot data.
     * @param cx Entity horizontal center.
     * @param top_y Entity topmost boundary Y position.
     */
    inline void drawNamePanel(ImDrawList* dl, const PlayerData& p,
                              float cx, float top_y) {
        if (!cfg::esp::name && !cfg::esp::distance && !cfg::esp::health) return;

        char nameBuf[24] = {0};
        if (cfg::esp::name) {
            truncName(p.name, nameBuf, sizeof(nameBuf));
            if (nameBuf[0] == 0)
                snprintf(nameBuf, sizeof(nameBuf), "%s", p.isBot ? "BOT" : "Player");
        }

        char distBuf[16] = {0};
        if (cfg::esp::distance) {
            fmtDist(p.distance, distBuf, sizeof(distBuf));
        }

        ImU32 nameCol = toU32(cfg::esp::name_col);
        ImU32 distCol = toU32(cfg::esp::distance_col);
        ImU32 bgMain  = p.isVisible ? IM_COL32(12, 12, 16, 230) : IM_COL32(28, 28, 34, 200);

        const float fs   = ESP_FONT_SIZE;
        const float padX = 4.f;
        const float padY = 2.f;
        const float gap  = 4.f;

        float nameW = cfg::esp::name ? ImGui::CalcTextSize(nameBuf).x + padX * 2.f : 0.f;
        float distW = cfg::esp::distance ? ImGui::CalcTextSize(distBuf).x + padX * 2.f : 0.f;

        float totalW = nameW;
        if (distW > 0.f) totalW += (totalW > 0.f ? gap : 0.f) + distW;

        if (totalW > 0.f) {
            float rowH = PANEL_ROW_H;
            float bx = cx - totalW * 0.5f;
            float by = top_y - rowH - PANEL_TOP_OFFSET;

            dl->AddRectFilled({bx, by}, {bx + totalW, by + rowH}, bgMain, 3.f);
            dl->AddRect({bx, by}, {bx + totalW, by + rowH}, C_BORDER, 3.f, 0, 1.f);

            float cursorX = bx;
            if (cfg::esp::name) {
                ImVec2 ts = ImGui::CalcTextSize(nameBuf);
                dl->AddText(ImGui::GetFont(), fs,
                    {cursorX + padX, by + padY},
                    p.isVisible ? nameCol : withAlpha(nameCol, 0.70f), nameBuf);
                cursorX += nameW;
            }

            if (cfg::esp::distance) {
                if (cursorX > bx) cursorX += gap;
                dl->AddText(ImGui::GetFont(), fs,
                    {cursorX + padX, by + padY},
                    p.isVisible ? distCol : withAlpha(distCol, 0.70f), distBuf);
            }

            // Health bar directly under the info pill
            if (cfg::esp::health) {
                float barY = by + rowH + 2.f;
                float barH = 2.5f;
                float hp01 = p.maxHP > 0 ? (float)p.curHP / (float)p.maxHP : 0.f;
                hp01 = ImClamp(hp01, 0.f, 1.f);

                dl->AddRectFilled({bx, barY}, {bx + totalW, barY + barH},
                                  IM_COL32(0, 0, 0, 200), 1.5f);
                dl->AddRectFilled({bx, barY}, {bx + totalW * hp01, barY + barH},
                                  hpColor(hp01), 1.5f);
            }
        } else if (cfg::esp::health) {
            // Standalone health bar if name and distance are disabled
            float barW = 40.f;
            float barH = 3.f;
            float bx = cx - barW * 0.5f;
            float by = top_y - barH - 8.f;
            float hp01 = p.maxHP > 0 ? (float)p.curHP / (float)p.maxHP : 0.f;
            hp01 = ImClamp(hp01, 0.f, 1.f);

            dl->AddRectFilled({bx, by}, {bx + barW, by + barH},
                              IM_COL32(0, 0, 0, 200), 1.5f);
            dl->AddRectFilled({bx, by}, {bx + barW * hp01, by + barH},
                              hpColor(hp01), 1.5f);
        }
    }

    /**
     * @brief Renders 2D bounding box (corners or clean outline).
     * @param dl ImGui foreground draw list.
     * @param tl Top-left coordinate.
     * @param br Bottom-right coordinate.
     * @param col Primary outline color.
     * @param type Box rendering variant (0 = Corner brackets, 1 = Full outline).
     */
    inline void drawBox(ImDrawList* dl, ImVec2 tl, ImVec2 br, ImU32 col, int type) {
        float thick = cfg::esp::box_thickness;
        float round = cfg::esp::box_rounding;

        if (type == 0) {
            float len = (br.x - tl.x) * cfg::esp::bracket_frac;
            ImVec2 pts[8][2] = {
                {tl,                 {tl.x + len, tl.y}},
                {tl,                 {tl.x, tl.y + len}},
                {{br.x - len, tl.y}, {br.x, tl.y}},
                {{br.x, tl.y},       {br.x, tl.y + len}},
                {{tl.x, br.y - len}, {tl.x, br.y}},
                {{tl.x, br.y},       {tl.x + len, br.y}},
                {{br.x - len, br.y}, {br.x, br.y}},
                {{br.x, br.y - len}, {br.x, br.y}},
            };
            for (auto& s : pts) {
                dl->AddLine(s[0], s[1], col, thick);
            }
        } else {
            dl->AddRect(tl, br, col, round, 0, thick);
        }
    }

    /** Bone link segment renderer */
    inline void sLine(ImDrawList* dl, const Vector3& a, const Vector3& b,
                      ImU32 col, float t = 1.0f) {
        if (!ok(a) || !ok(b)) return;
        dl->AddLine({a.x, a.y}, {b.x, b.y}, col, t);
    }

    /** Bone joint dot renderer */
    inline void sDot(ImDrawList* dl, const Vector3& v, ImU32 col, float r = 2.0f) {
        if (!ok(v)) return;
        dl->AddCircleFilled({v.x, v.y}, r, col);
    }

    /**
     * @brief Renders anatomical bone connections and joint dots.
     * @param dl ImGui foreground draw list.
     * @param p Player snapshot data.
     * @param col Skeleton line color.
     */
    inline void drawSkeleton(ImDrawList* dl, const PlayerData& p, ImU32 col) {
        sLine(dl, p.Head, p.Breast, col, 1.2f);
        sLine(dl, p.Breast, p.Hip, col, 1.2f);

        sLine(dl, p.Breast, p.LShoulder, col);
        sLine(dl, p.Breast, p.RShoulder, col);
        sLine(dl, p.LShoulder, p.LForeArm, col);
        sLine(dl, p.RShoulder, p.RForeArm, col);
        sLine(dl, p.LForeArm, p.LHand, col);
        sLine(dl, p.RForeArm, p.RHand, col);

        sLine(dl, p.Hip, p.LAnkle, col, 1.2f);
        sLine(dl, p.Hip, p.RAnkle, col, 1.2f);
        sLine(dl, p.LAnkle, p.LToe, col);
        sLine(dl, p.RAnkle, p.RToe, col);

        sDot(dl, p.Head, col, 2.2f);
        sDot(dl, p.Breast, col, 2.0f);
        sDot(dl, p.Hip, col, 2.0f);
        sDot(dl, p.LShoulder, col, 1.8f);
        sDot(dl, p.RShoulder, col, 1.8f);
        sDot(dl, p.LHand, col, 1.8f);
        sDot(dl, p.RHand, col, 1.8f);
        sDot(dl, p.LAnkle, col, 1.8f);
        sDot(dl, p.RAnkle, col, 1.8f);
    }

    /**
     * @brief Renders center-screen crosshair with smooth animated pulsing ring.
     * @param dl ImGui foreground draw list.
     */
    inline void drawCrosshair(ImDrawList* dl) {
        if (g_sw < 10.f || g_sh < 10.f) return;
        float cx = g_sw * 0.5f, cy = g_sh * 0.5f;

        ImU32 c      = IM_COL32(220, 40, 40, 240);
        float gap    = 4.f;
        float len    = 8.f;
        float thick  = 1.0f;

        // Thin crosshair lines
        dl->AddLine({cx - gap - len, cy}, {cx - gap, cy}, c, thick);
        dl->AddLine({cx + gap,       cy}, {cx + gap + len, cy}, c, thick);
        dl->AddLine({cx, cy - gap - len}, {cx, cy - gap}, c, thick);
        dl->AddLine({cx, cy + gap},       {cx, cy + gap + len}, c, thick);

        // Center dot
        dl->AddCircleFilled({cx, cy}, 1.2f, c);

        // Outer pulsing ring
        float t      = (float)ImGui::GetTime();
        float pulse  = 0.5f + 0.5f * sinf(t * 2.5f);
        float radius = 26.f + 3.f * pulse;
        int   alpha  = (int)(160 + 80.f * pulse);
        dl->AddCircle({cx, cy}, radius, IM_COL32(220, 40, 40, alpha), 64, 1.0f);
    }

    /**
     * @brief Master ESP dispatch routine. Iterates player snapshots and draws enabled visuals.
     * Strictly gated by cfg::esp::enabled for 100% manual menu control.
     */
    inline void draw() {
        if (!cfg::esp::enabled) return;
        if (!data::inMatch.load()) return;
        if (g_sw < 10.f || g_sh < 10.f) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();

        std::vector<PlayerData> snap;
        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            snap = data::players;
        }

        // Top-center anchor (matching reference project: screenWidth / 2, 40)
        ImVec2 anchor(g_sw * 0.5f, 40.f);

        for (const auto& p : snap) {
            // Skip dead or invalid entities
            if (p.curHP <= 0) continue;
            if (p.distance <= 0.f || p.distance > cfg::esp::max_range) continue;

            // Target must be strictly in front of camera (positive screen coordinates)
            if (!ok(p.Head) || !ok(p.Toe)) continue;

            // Target must be within screen dimensions (no drawing off-screen)
            if (p.Head.x < 0.f || p.Head.x > g_sw || p.Head.y < 0.f || p.Head.y > g_sh) continue;
            if (p.Toe.x  < 0.f || p.Toe.x  > g_sw || p.Toe.y  < 0.f || p.Toe.y  > g_sh) continue;

            // Minimum height sanity check
            if (fabsf(p.Head.y - p.Toe.y) < 8.f) continue;

            SkeletonBox sb = computeSkeletonBox(p);
            if (!sb.valid) continue;

            ImVec2 tl = sb.tl;
            ImVec2 br = sb.br;

            // Strictly within screen viewport bounds
            if (br.x < 0.f || tl.x > g_sw || br.y < 0.f || tl.y > g_sh) continue;

            float cx = (tl.x + br.x) * 0.5f;

            ImU32 colBase = p.isVisible ? toU32(cfg::esp::box_visible)
                                        : toU32(cfg::esp::box_col);
            colBase = fadeDist(colBase, p.distance, cfg::esp::max_range);

            // Thin single ESP Line (hairline thickness = 1.0f)
            if (cfg::esp::tracer) {
                dl->AddLine(anchor, ImVec2(cx, tl.y), colBase, 1.0f);
            }

            // ESP Box
            if (cfg::esp::box) {
                drawBox(dl, tl, br, colBase, cfg::esp::box_type);
            }

            // ESP Skeleton
            if (cfg::esp::skeleton) {
                drawSkeleton(dl, p, colBase);
            }

            // ESP Name, Distance & Health Panel
            if (cfg::esp::name || cfg::esp::distance || cfg::esp::health) {
                drawNamePanel(dl, p, cx, tl.y);
            }
        }

        // ESP Crosshair
        if (cfg::esp::crosshair) {
            drawCrosshair(dl);
        }
    }
}
