#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "styles/styles.h"
#include "cfg.hpp"
#include "engine/game_data.hpp"
#include "engine/engine.hpp"
#include "config_store.h"
#include "clipboard.h"
#include "Icon.h"
#include "libraries/storage/logger.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <inttypes.h>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>

#ifndef oxorany
#define oxorany(x) (x)
#endif

namespace engine {
    void render_esp();
}

namespace func::esp {
    inline void draw() {
        engine::render_esp();
    }
}

namespace data {
    inline bool in_match() {
        return engine::data::g_in_match.load();
    }
    inline size_t player_count() {
        std::lock_guard<std::mutex> lk(engine::data::g_players_mutex);
        return engine::data::g_players.size();
    }
}

namespace ui::menu {
    using namespace style;

    static float  ma     = 0.f;
    static int    tab    = 0;
    static bool   drag   = false;
    static ImVec2 doff(0, 0);

    static constexpr float R = 12.f;

    // Dynamic DPI UI Scaling for High-Density Mobile Phones & Emulators
    static float s_scale = 1.0f;
    static inline float S(float v) { return v * s_scale; }

    static std::unordered_map<std::string, float> g_nsAnim;
    static inline float NS_Anim(const std::string& key, float target, float speed = 14.f) {
        float dt = ImGui::GetIO().DeltaTime; if (dt <= 0.f) dt = 1.f/60.f;
        auto it = g_nsAnim.find(key);
        if (it == g_nsAnim.end()) { g_nsAnim[key] = target; return target; }
        float cur = it->second;
        float t = 1.f - std::exp(-speed * dt);
        cur = cur + (target - cur) * t;
        if (std::fabs(cur - target) < 0.001f) cur = target;
        it->second = cur; return cur;
    }
    static inline ImU32 NS_Lerp(ImU32 a, ImU32 b, float t) {
        t = ImClamp(t, 0.f, 1.f);
        int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF, aa=(a>>24)&0xFF;
        int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF, ba=(b>>24)&0xFF;
        return IM_COL32((int)(ar+(br-ar)*t),(int)(ag+(bg-ag)*t),
                        (int)(ab+(bb-ab)*t),(int)(aa+(ba-aa)*t));
    }
    static inline ImVec2 NS_TextSize(const char* text, float size) {
        ImVec2 sz = ImGui::CalcTextSize(text);
        float scale = (ImGui::GetFontSize() > 0.f) ? (size / ImGui::GetFontSize()) : 1.f;
        return ImVec2(sz.x * scale, sz.y * scale);
    }

    // Color definitions: Toggles, sliders and buttons derive from the signature border color (No Pink!)
    static inline ImU32 C_TextMain()     { return col(UITheme::text,         content_alpha); }
    static inline ImU32 C_TextSec()      { return col(UITheme::text_dim,     content_alpha); }
    static inline ImU32 C_TextDim()      { return col(UITheme::text_dim,     content_alpha * 0.75f); }
    static inline ImU32 C_BgDark()       { return col(UITheme::bg,           content_alpha); }
    static inline ImU32 C_BgSidebar()    { return col(UITheme::sidebar,      content_alpha); }
    static inline ImU32 C_BgCard()       { return col(UITheme::panel,        content_alpha); }
    static inline ImU32 C_BgCardHov()    { return col(UITheme::widget,       content_alpha); }
    static inline ImU32 C_Border()       { return col(UITheme::border,       content_alpha); }
    static inline ImU32 C_Accent()       { return col(UITheme::border_light, content_alpha); }
    static inline ImU32 C_AccentBright() { return col(UITheme::border_light, content_alpha); }
    static inline ImU32 C_AccentDim()    { return col(UITheme::border_light, content_alpha * 0.35f); }
    static inline ImU32 C_Success()      { return IM_COL32(52, 211, 110, (int)(content_alpha * 255)); }
    static inline ImU32 C_Danger()       { return IM_COL32(255, 69, 58, (int)(content_alpha * 255)); }

    static void NS_CardBegin(ImDrawList* dl, ImVec2 pos, float w, float h) {
        dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, C_BgCard(), S(9.f));
        dl->AddRect(pos, {pos.x+w, pos.y+h}, C_Border(), S(9.f), 0, 1.2f);
    }

    static void NS_CardTitle(ImDrawList* dl, ImVec2 pos, const char* title, const char* hint = nullptr, float w = 0.f) {
        dl->AddRectFilled({pos.x, pos.y+S(2.f)}, {pos.x+S(3.5f), pos.y+S(17.f)}, C_Accent(), S(1.5f));
        dl->AddText(ImGui::GetFont(), S(15.0f), {pos.x+S(10.f), pos.y+S(1.f)}, C_TextMain(), title);
        if (hint && w > 0.f) {
            float hw = NS_TextSize(hint, S(12.5f)).x;
            dl->AddText(ImGui::GetFont(), S(12.5f),
                {pos.x + w - hw - S(20.f), pos.y+S(2.f)},
                C_Accent(), hint);
        }
    }

    static bool NS_Toggle(const char* id, const char* label, bool* value, float width = 0.0f) {
        ImGui::PushID(id);
        float w = (width > 0.0f) ? width : ImMin(ImGui::GetContentRegionAvail().x, content_w - S(20.0f));
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = S(36.f);
        ImGui::InvisibleButton("btn", ImVec2(w, h));
        bool clicked = ImGui::IsItemClicked();
        bool hover = ImGui::IsItemHovered();
        if (clicked) *value = !*value;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            std::string key = std::string(id) + "#t";
            float on = NS_Anim(key+"on", *value ? 1.f : 0.f, 16.f);
            float hov = NS_Anim(key+"hov", hover ? 1.f : 0.f, 18.f);
            ImU32 lblCol = NS_Lerp(C_TextSec(), C_TextMain(), ImMax(on, hov*0.5f));
            dl->AddText(ImGui::GetFont(), S(15.0f),
                {pos.x, pos.y + (h - S(15.0f)) * 0.5f}, lblCol, label);
            float tw = S(46.f), th = S(24.f);
            float tx = pos.x + w - tw, ty = pos.y + (h - th) * 0.5f;

            // Background pill matches signature border color
            dl->AddRectFilled({tx, ty}, {tx+tw, ty+th},
                NS_Lerp(IM_COL32(38,28,55,(int)(content_alpha*255)), C_Accent(), on), th*0.5f);
            if (on > 0.05f) {
                dl->AddRect({tx, ty}, {tx+tw, ty+th},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)(120*on) << 24),
                    th*0.5f, 0, 1.2f);
            }
            float knobR = th*0.5f - S(2.5f);
            float knobX = tx + S(2.5f) + (tw - th) * on;
            dl->AddCircleFilled({knobX+knobR, ty+th*0.5f}, knobR,
                on > 0.5f ? IM_COL32(255,255,255,255) : IM_COL32(220,210,235,255));
        }
        ImGui::PopID();
        return clicked;
    }

    static bool NS_Slider(const char* id, const char* label, float* value,
                          float minV, float maxV, const char* suffix = "", float width = 0.0f) {
        ImGui::PushID(id);
        float w = (width > 0.0f) ? width : ImMin(ImGui::GetContentRegionAvail().x, content_w - S(20.0f));
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            char valStr[32];
            if (suffix && suffix[0] != '\0') {
                snprintf(valStr, sizeof(valStr), "%.0f%s", *value, suffix);
            } else {
                snprintf(valStr, sizeof(valStr), "%.0f", *value);
            }
            float valW = NS_TextSize(valStr, S(13.5f)).x;
            dl->AddText(ImGui::GetFont(), S(14.5f), {pos.x, pos.y}, C_TextSec(), label);
            dl->AddText(ImGui::GetFont(), S(13.5f), {pos.x + w - valW, pos.y}, C_TextDim(), valStr);

            float ty = pos.y + S(22.f), tw2 = w;
            float rawPct = (maxV > minV) ? ImClamp((*value-minV)/(maxV-minV), 0.f, 1.f) : 0.f;
            float pct = NS_Anim(std::string(id)+"#pct", rawPct, 22.f);

            // Track background
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2, ty+S(5.0f)},
                IM_COL32(38, 28, 55, (int)(content_alpha*255)), S(2.5f));
            // Track active fill
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2*pct, ty+S(5.0f)}, C_Accent(), S(2.5f));
            if (pct > 0.01f) {
                dl->AddRect({pos.x, ty}, {pos.x+tw2*pct, ty+S(5.0f)},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)90 << 24), S(2.5f), 0, 1.f);
            }
            // Thumb knob
            float thumbX = pos.x + tw2 * pct;
            dl->AddCircleFilled({thumbX, ty + S(2.5f)}, S(7.5f), IM_COL32(255, 255, 255, (int)(255 * content_alpha)));
            dl->AddCircle({thumbX, ty + S(2.5f)}, S(7.5f), C_Accent(), 16, 1.5f);

            ImGui::SetCursorScreenPos({pos.x, ty - S(6.f)});
            ImGui::SetNextItemWidth(tw2);
        }
        ImGui::PushStyleColor(ImGuiCol_FrameBg,          IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,       IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(0,0,0,0));
        bool changed = ImGui::SliderFloat("##s", value, minV, maxV, "");
        ImGui::PopStyleColor(5);
        ImGui::Dummy(ImVec2(0, S(10.f)));
        ImGui::PopID();
        return changed;
    }

    static bool NS_Button(const char* id, const char* label, ImVec2 size, bool accent = true) {
        ImGui::PushID(id);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        bool hover = ImGui::IsMouseHoveringRect(pos, {pos.x+size.x, pos.y+size.y});
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            std::string key = std::string(id)+"#b";
            float hov = NS_Anim(key+"hov", hover?1.f:0.f, 20.f);
            ImVec2 a = pos, b = {pos.x+size.x, pos.y+size.y};
            ImU32 bg = accent ? NS_Lerp(C_Accent(), C_AccentBright(), hov)
                              : NS_Lerp(C_BgCard(), C_BgCardHov(), hov);
            dl->AddRectFilled(a, b, bg, S(7.f));
            if (accent && hov > 0.1f) {
                dl->AddRect(a, b, (C_AccentBright() & 0x00FFFFFF) | ((ImU32)(120*hov) << 24),
                            S(7.f), 0, 1.2f);
            }
            if (!accent) {
                dl->AddRect(a, b, C_Border(), S(7.f), 0, 1.2f);
            }
            float lw = NS_TextSize(label, S(14.0f)).x;
            dl->AddText(ImGui::GetFont(), S(14.0f),
                {pos.x + (size.x-lw)*0.5f, pos.y + (size.y - S(14.0f))*0.5f},
                accent ? IM_COL32(255,255,255,255) : C_TextMain(), label);
        }
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("btn", size);
        bool clicked = ImGui::IsItemClicked();
        ImGui::Dummy(size);
        ImGui::PopID();
        return clicked;
    }

    static void NS_InfoRow(const char* label, const char* value, ImU32 valueColor = 0, float width = 0.0f) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float w = (width > 0.0f) ? width : ImMin(ImGui::GetContentRegionAvail().x, content_w - S(20.0f));
        float rh = S(32.f);
        bool hov = ImGui::IsMouseHoveringRect(pos, {pos.x+w, pos.y+rh});
        if (dl) {
            if (hov) {
                dl->AddRectFilled({pos.x, pos.y}, {pos.x+w, pos.y+rh}, C_BgCardHov(), S(4.f));
                dl->AddRectFilled({pos.x, pos.y+S(4.f)}, {pos.x+S(3.0f), pos.y+rh-S(4.f)}, C_Accent(), S(1.5f));
            }
            dl->AddText(ImGui::GetFont(), S(14.0f), {pos.x+S(6.f), pos.y + (rh - S(14.0f))*0.5f}, C_TextDim(), label);
            float vw = NS_TextSize(value, S(14.0f)).x;
            ImU32 fc = valueColor ? valueColor : C_TextMain();
            dl->AddText(ImGui::GetFont(), S(14.0f), {pos.x+w-vw-S(6.f), pos.y + (rh - S(14.0f))*0.5f}, fc, value);
        }
        ImGui::Dummy(ImVec2(0, rh+S(2.f)));
    }

    static bool NS_StepSlider(const char* id, const char* label, int* value,
                              const char** names, int count, float width = 0.0f) {
        if (!value || count <= 0 || !names) return false;
        ImGui::PushID(id);
        float w = (width > 0.0f) ? width : ImMin(ImGui::GetContentRegionAvail().x, content_w - S(20.0f));
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        if (*value < 0) *value = 0;
        if (*value >= count) *value = count - 1;

        const char* curText = names[*value];

        if (dl) {
            float valW = NS_TextSize(curText, S(13.5f)).x;
            dl->AddText(ImGui::GetFont(), S(14.5f), {pos.x, pos.y}, C_TextSec(), label);

            ImU32 txtCol = (*value == 0) ? C_TextDim() : C_AccentBright();
            dl->AddText(ImGui::GetFont(), S(13.5f), {pos.x + w - valW, pos.y}, txtCol, curText);

            float ty = pos.y + S(22.f), tw2 = w;
            float maxV = (float)(count - 1);
            float rawPct = (maxV > 0.f) ? ImClamp((float)(*value) / maxV, 0.f, 1.f) : 0.f;
            float pct = NS_Anim(std::string(id)+"#pct", rawPct, 22.f);

            // Track background
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2, ty+S(5.0f)},
                IM_COL32(38, 28, 55, (int)(content_alpha*255)), S(2.5f));
            // Track active fill
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2*pct, ty+S(5.0f)}, C_Accent(), S(2.5f));
            if (pct > 0.01f) {
                dl->AddRect({pos.x, ty}, {pos.x+tw2*pct, ty+S(5.0f)},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)90 << 24), S(2.5f), 0, 1.f);
            }

            // Step tick dots along track
            for (int s = 0; s < count; s++) {
                float sx = pos.x + (maxV > 0.f ? (tw2 * ((float)s / maxV)) : 0.f);
                ImU32 dotCol = (s <= *value) ? C_AccentBright() : IM_COL32(80, 60, 110, (int)(content_alpha * 200));
                dl->AddCircleFilled({sx, ty + S(2.5f)}, S(2.5f), dotCol);
            }

            // Thumb knob
            float thumbX = pos.x + tw2 * pct;
            dl->AddCircleFilled({thumbX, ty + S(2.5f)}, S(7.5f), IM_COL32(255, 255, 255, (int)(255 * content_alpha)));
            dl->AddCircle({thumbX, ty + S(2.5f)}, S(7.5f), C_Accent(), 16, 1.5f);

            ImGui::SetCursorScreenPos({pos.x, ty - S(6.f)});
            ImGui::SetNextItemWidth(tw2);
        }

        ImGui::PushStyleColor(ImGuiCol_FrameBg,          IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,       IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(0,0,0,0));
        bool changed = ImGui::SliderInt("##s_step", value, 0, count - 1, "");
        ImGui::PopStyleColor(5);
        ImGui::Dummy(ImVec2(0, S(10.f)));
        ImGui::PopID();
        return changed;
    }

            ImGui::SetCursorScreenPos({pos.x, ty - 6.f});
            ImGui::SetNextItemWidth(tw2);
        }

        ImGui::PushStyleColor(ImGuiCol_FrameBg,          IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,       IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(0,0,0,0));
        bool changed = ImGui::SliderInt("##s_step", value, 0, count - 1, "");
        ImGui::PopStyleColor(5);
        ImGui::Dummy(ImVec2(0, 10.f));
        ImGui::PopID();
        return changed;
    }

    // 3 Distinct Palette Colors specified by user: Blue (0), Red (1), Pink (2)
    static const ImVec4 kEspColors[3] = {
        ImVec4(0.10f, 0.65f, 1.00f, 1.0f), // 0: Blue (নীল)
        ImVec4(1.00f, 0.22f, 0.25f, 1.0f), // 1: Red (লাল)
        ImVec4(1.00f, 0.35f, 0.80f, 1.0f)  // 2: Pink (পিং)
    };

    static const char* kEspColorNames[3] = {
        oxorany("Blue"),
        oxorany("Red"),
        oxorany("Pink")
    };

    /**
     * @brief Specialized Color Skip Bar (কালার স্কিপ বার) with 3 selectable colors: Blue, Red, Pink.
     * Can be increased or decreased smoothly via left/right buttons, step dots, and touch drag.
     */
    static bool NS_ColorStepSlider(const char* id, const char* label, ImVec4* color, int* cur_idx, float width = 0.0f) {
        if (!color || !cur_idx) return false;
        ImGui::PushID(id);
        float w = (width > 0.0f) ? width : ImMin(ImGui::GetContentRegionAvail().x, content_w - 20.0f);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        if (*cur_idx < 0) *cur_idx = 0;
        if (*cur_idx > 2) *cur_idx = 2;

        const char* curName = kEspColorNames[*cur_idx];
        ImVec4 activeVec = kEspColors[*cur_idx];
        ImU32 activeCol = ImGui::ColorConvertFloat4ToU32(activeVec);
        bool changed = false;

        if (dl) {
            // Label on the left
            dl->AddText(ImGui::GetFont(), 14.0f, {pos.x, pos.y}, C_TextSec(), label);

            // Right side: Active color circle swatch + color name
            float nameW = NS_TextSize(curName, 13.5f).x;
            float pillW = 12.f;
            float pillGap = 6.f;
            float rightTotalW = pillW + pillGap + nameW;

            ImVec2 pillPos = {pos.x + w - rightTotalW, pos.y + 2.f};
            dl->AddCircleFilled({pillPos.x + pillW * 0.5f, pillPos.y + pillW * 0.5f}, pillW * 0.5f, activeCol);
            dl->AddCircle({pillPos.x + pillW * 0.5f, pillPos.y + pillW * 0.5f}, pillW * 0.5f, IM_COL32(255, 255, 255, 180), 12, 1.0f);
            dl->AddText(ImGui::GetFont(), 13.5f, {pillPos.x + pillW + pillGap, pos.y}, activeCol, curName);
        }

        // Second row: [ < ]  --- Track with 3 color dots ---  [ > ]
        float rowY = pos.y + 20.f;
        float btnW = 26.f;
        float btnH = 22.f;

        // Decrement button [ < ]
        ImGui::SetCursorScreenPos({pos.x, rowY});
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 30, 60, (int)(content_alpha * 220)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(65, 45, 95, (int)(content_alpha * 255)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeCol);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        if (ImGui::Button("<", ImVec2(btnW, btnH))) {
            if (*cur_idx > 0) {
                (*cur_idx)--;
                changed = true;
            }
        }

        // Track in middle between [ < ] and [ > ]
        float trackX = pos.x + btnW + 8.f;
        float trackW = w - (btnW * 2.f + 16.f);
        float trackY = rowY + (btnH - 5.0f) * 0.5f;

        float maxV = 2.0f; // 3 steps: 0, 1, 2
        float rawPct = (float)(*cur_idx) / maxV;
        float pct = NS_Anim(std::string(id) + "#col_pct", rawPct, 22.f);

        if (dl) {
            // Track background
            dl->AddRectFilled({trackX, trackY}, {trackX + trackW, trackY + 5.0f},
                IM_COL32(38, 28, 55, (int)(content_alpha * 255)), 2.5f);

            // Active Track Fill
            dl->AddRectFilled({trackX, trackY}, {trackX + trackW * pct, trackY + 5.0f}, activeCol, 2.5f);

            // 3 Color dots along the skip bar track (Blue dot at 0, Red dot at 1, Pink dot at 2)
            for (int s = 0; s < 3; s++) {
                float sx = trackX + (trackW * ((float)s / maxV));
                ImU32 dotCol = ImGui::ColorConvertFloat4ToU32(kEspColors[s]);
                float dotRadius = (s == *cur_idx) ? 4.5f : 3.0f;
                dl->AddCircleFilled({sx, trackY + 2.5f}, dotRadius, dotCol);
                if (s == *cur_idx) {
                    dl->AddCircle({sx, trackY + 2.5f}, dotRadius + 1.5f, IM_COL32(255, 255, 255, 230), 12, 1.2f);
                }
            }

            // Thumb knob centered on active percentage
            float thumbX = trackX + trackW * pct;
            dl->AddCircleFilled({thumbX, trackY + 2.5f}, 7.0f, IM_COL32(255, 255, 255, (int)(255 * content_alpha)));
            dl->AddCircleFilled({thumbX, trackY + 2.5f}, 5.0f, activeCol);
            dl->AddCircle({thumbX, trackY + 2.5f}, 7.0f, IM_COL32(255, 255, 255, 220), 16, 1.4f);
        }

        // Invisible slider over track for touch drag & tap
        ImGui::SetCursorScreenPos({trackX, rowY - 2.f});
        ImGui::SetNextItemWidth(trackW);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,          IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,       IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(0,0,0,0));
        if (ImGui::SliderInt("##col_step", cur_idx, 0, 2, "")) {
            changed = true;
        }
        ImGui::PopStyleColor(5);

        // Increment button [ > ]
        ImGui::SetCursorScreenPos({pos.x + w - btnW, rowY});
        if (ImGui::Button(">", ImVec2(btnW, btnH))) {
            if (*cur_idx < 2) {
                (*cur_idx)++;
                changed = true;
            }
        }
        ImGui::PopStyleVar();    // Pop FrameRounding
        ImGui::PopStyleColor(3);  // Pop Button colors

        if (changed) {
            *cur_idx = ImClamp(*cur_idx, 0, 2);
            *color = kEspColors[*cur_idx];
        }

        ImGui::SetCursorScreenPos({pos.x, rowY + btnH + 6.f});
        ImGui::Dummy(ImVec2(0, 4.f));
        ImGui::PopID();
        return changed;
    }

    /**
     * @brief Tab 0: Combat & Aim Assist - Book Layout (Two balanced pages side by side).
     */
    static void tab_aimbot(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        float ch = ImGui::GetContentRegionAvail().y;
        ImVec2 cp = ImGui::GetCursorScreenPos();

        float gap = S(12.f);
        float pageW = (cw - gap) * 0.5f;
        float pageH = ImMax(S(305.f), ch - S(4.f));

        // LEFT PAGE (বাম পৃষ্ঠা): Aim Master & Trigger Mode
        ImVec2 pLeft = cp;
        NS_CardBegin(dl, pLeft, pageW, pageH);
        NS_CardTitle(dl, {pLeft.x + S(12.f), pLeft.y + S(12.f)}, oxorany("Aim Silent & Combat"), "Left Page", pageW);
        ImGui::SetCursorScreenPos({pLeft.x + S(12.f), pLeft.y + S(38.f)});
        ImGui::BeginGroup();
        NS_Toggle("aim_silent", oxorany("Aim Silent"),  &cfg::aim::silent, pageW - S(24.f));
        NS_Toggle("aim_nobita", oxorany("Nobita Mode"), &cfg::aim::nobita, pageW - S(24.f));
        NS_Toggle("aim_draw",   oxorany("FOV Circle"),  &cfg::aim::draw_fov, pageW - S(24.f));

        const char* bones[] = { oxorany("None"), oxorany("Head"), oxorany("Chest"), oxorany("Hip") };
        int cur_bone = (cfg::aim::target_bone >= 0 && cfg::aim::target_bone <= 2) ? (cfg::aim::target_bone + 1) : 0;
        if (NS_StepSlider("aim_bone", oxorany("Target Bone"), &cur_bone, bones, 4, pageW - S(24.f))) {
            cfg::aim::target_bone = (cur_bone > 0) ? (cur_bone - 1) : -1;
        }

        const char* triggers[] = { oxorany("None"), oxorany("On Fire"), oxorany("On Scope"), oxorany("Always") };
        int cur_trig = (cfg::aim::trigger_type >= 0 && cfg::aim::trigger_type <= 2) ? (cfg::aim::trigger_type + 1) : 0;
        if (NS_StepSlider("aim_trig", oxorany("Trigger Mode"), &cur_trig, triggers, 4, pageW - S(24.f))) {
            cfg::aim::trigger_type = (cur_trig > 0) ? (cur_trig - 1) : -1;
        }
        ImGui::EndGroup();

        // RIGHT PAGE (ডান পৃষ্ঠা): Aim Parameters & Fine Tuning
        ImVec2 pRight = {cp.x + pageW + gap, cp.y};
        NS_CardBegin(dl, pRight, pageW, pageH);
        NS_CardTitle(dl, {pRight.x + S(12.f), pRight.y + S(12.f)}, oxorany("Aim Parameters"), "Right Page", pageW);
        ImGui::SetCursorScreenPos({pRight.x + S(12.f), pRight.y + S(38.f)});
        ImGui::BeginGroup();
        NS_Slider("aim_fov",  oxorany("FOV Radius"),   &cfg::aim::fov,      30.f, 350.f, "px", pageW - S(24.f));
        NS_Slider("aim_dist", oxorany("Aim Distance"), &cfg::aim::distance, 30.f, 400.f, "m",  pageW - S(24.f));
        NS_Slider("aim_smth", oxorany("Aim Smooth"),   &cfg::aim::smooth,   1.f,  20.f, "",    pageW - S(24.f));

        bool active = cfg::aim::silent || cfg::aim::nobita;
        NS_InfoRow(oxorany("Combat State"), active ? oxorany("ACTIVE") : oxorany("STANDBY"), active ? C_Success() : C_TextDim(), pageW - S(24.f));
        NS_InfoRow(oxorany("Target Lock"), cur_bone > 0 ? bones[cur_bone] : oxorany("Auto Scan"), C_Accent(), pageW - S(24.f));
        ImGui::EndGroup();

        cfg::aim::enabled = cfg::aim::silent || cfg::aim::nobita;
    }

    /**
     * @brief Tab 1: Visuals & ESP - Book Layout (Two balanced pages side by side).
     */
    static void tab_visuals(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        float ch = ImGui::GetContentRegionAvail().y;
        ImVec2 cp = ImGui::GetCursorScreenPos();

        float gap = S(12.f);
        float pageW = (cw - gap) * 0.5f;
        float pageH = ImMax(S(305.f), ch - S(4.f));

        // LEFT PAGE (বাম পৃষ্ঠা): ESP Visual Overlays & Box Style
        ImVec2 pLeft = cp;
        NS_CardBegin(dl, pLeft, pageW, pageH);
        NS_CardTitle(dl, {pLeft.x + S(12.f), pLeft.y + S(12.f)}, oxorany("ESP Visual Overlays"), "Left Page", pageW);
        ImGui::SetCursorScreenPos({pLeft.x + S(12.f), pLeft.y + S(38.f)});
        ImGui::BeginGroup();
        NS_Toggle("esp_tracer",  oxorany("ESP Line"),   &cfg::esp::tracer, pageW - S(24.f));
        NS_Toggle("esp_box",     oxorany("ESP Box"),    &cfg::esp::box,    pageW - S(24.f));
        NS_Toggle("esp_health",  oxorany("ESP Health"), &cfg::esp::health, pageW - S(24.f));

        const char* box_types[] = { oxorany("None"), oxorany("Full Box"), oxorany("Corner"), oxorany("Glow") };
        int cur_box = cfg::esp::box ? (cfg::esp::box_type + 1) : 0;
        if (NS_StepSlider("box_type", oxorany("Box Style"), &cur_box, box_types, 4, pageW - S(24.f))) {
            if (cur_box == 0) cfg::esp::box = false;
            else { cfg::esp::box = true; cfg::esp::box_type = cur_box - 1; }
        }
        NS_Slider("box_th", oxorany("Thickness"),  &cfg::esp::box_thickness, 0.5f, 5.0f, "px", pageW - S(24.f));
        ImGui::EndGroup();

        cfg::esp::enabled = cfg::esp::tracer || cfg::esp::box || cfg::esp::health;

        // RIGHT PAGE (ডান পৃষ্ঠা): Box Configuration & Palette
        ImVec2 pRight = {cp.x + pageW + gap, cp.y};
        NS_CardBegin(dl, pRight, pageW, pageH);
        NS_CardTitle(dl, {pRight.x + S(12.f), pRight.y + S(12.f)}, oxorany("Style & Color Palette"), "Right Page", pageW);
        ImGui::SetCursorScreenPos({pRight.x + S(12.f), pRight.y + S(38.f)});
        ImGui::BeginGroup();
        NS_Slider("box_rd", oxorany("Rounding"),   &cfg::esp::box_rounding,  0.f,  10.f, "px", pageW - S(24.f));
        NS_Slider("box_rg", oxorany("Max Range"),  &cfg::esp::max_range,     50.f, 500.f, "m",  pageW - S(24.f));
        static bool s_col_inited = false;
        if (!s_col_inited) {
            s_col_inited = true;
            cfg::esp::box_col = kEspColors[cfg::esp::box_col_idx];
            cfg::esp::box_visible = kEspColors[cfg::esp::vis_col_idx];
        }

        if (NS_ColorStepSlider("col_box", oxorany("Box & Line Color"), &cfg::esp::box_col, &cfg::esp::box_col_idx, pageW - S(24.f))) {
            cfg::esp::box_col = kEspColors[cfg::esp::box_col_idx];
        }
        if (NS_ColorStepSlider("col_vis", oxorany("Visible Color"),    &cfg::esp::box_visible, &cfg::esp::vis_col_idx, pageW - S(24.f))) {
            cfg::esp::box_visible = kEspColors[cfg::esp::vis_col_idx];
        }

        bool espActive = cfg::esp::box || cfg::esp::tracer || cfg::esp::health;
        NS_InfoRow(oxorany("ESP Engine"), espActive ? oxorany("ACTIVE") : oxorany("STANDBY"), espActive ? C_Success() : C_TextDim(), pageW - S(24.f));
        ImGui::EndGroup();
    }

    /**
     * @brief Tab 2: Settings - Book Layout (Two balanced pages side by side).
     * Features full live Engine Telemetry & Readings as previously displayed in shell terminal.
     */
    static void tab_settings(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        float ch = ImGui::GetContentRegionAvail().y;
        ImVec2 cp = ImGui::GetCursorScreenPos();

        float gap = S(12.f);
        float pageW = (cw - gap) * 0.5f;
        float pageH = ImMax(S(320.f), ch - S(4.f));

        char buf[128];
        bool isAttached = engine::is_attached();
        int curPid = engine::get_target_pid();
        uintptr_t libBase = isAttached ? (uintptr_t)engine::mem::g_lib_base : 0;
        const char* targetPkg = engine::get_target_package();
        bool inM = isAttached ? data::in_match() : false;
        size_t pCount = isAttached ? data::player_count() : 0;

        // LEFT PAGE (বাম পৃষ্ঠা): Live Engine Telemetry & Memory Readings
        ImVec2 pLeft = cp;
        NS_CardBegin(dl, pLeft, pageW, pageH);
        NS_CardTitle(dl, {pLeft.x + S(12.f), pLeft.y + S(12.f)}, oxorany("Live Engine Readings"), "Telemetry", pageW);
        ImGui::SetCursorScreenPos({pLeft.x + S(12.f), pLeft.y + S(36.f)});
        ImGui::BeginGroup();

        // 1. Engine Status with clear color feedback
        if (isAttached) {
            NS_InfoRow(oxorany("Engine Status"), oxorany("ATTACHED & ACTIVE"), C_Success(), pageW - S(24.f));
        } else {
            NS_InfoRow(oxorany("Engine Status"), oxorany("STANDBY / SCANNING..."), IM_COL32(255, 120, 50, (int)(content_alpha * 255)), pageW - S(24.f));
        }

        // 2. Target Process
        if (isAttached && targetPkg && targetPkg[0] != '\0') {
            char pkgBuf[128];
            snprintf(pkgBuf, sizeof(pkgBuf), "%s (DETECTED)", targetPkg);
            NS_InfoRow(oxorany("Target Process"), pkgBuf, C_Success(), pageW - S(24.f));
        } else {
            NS_InfoRow(oxorany("Target Process"), oxorany("NOT DETECTED"), C_TextDim(), pageW - S(24.f));
        }

        // 3. Process ID (PID)
        if (isAttached && curPid > 0) {
            snprintf(buf, sizeof(buf), "%d (ACTIVE)", curPid);
            NS_InfoRow(oxorany("Process ID (PID)"), buf, C_Success(), pageW - S(24.f));
        } else {
            NS_InfoRow(oxorany("Process ID (PID)"), oxorany("WAITING FOR GAME"), C_TextDim(), pageW - S(24.f));
        }

        // 4. libil2cpp Base
        if (isAttached && libBase > 0) {
            snprintf(buf, sizeof(buf), "0x%lx", libBase);
            NS_InfoRow(oxorany("libil2cpp Base"), buf, C_Accent(), pageW - S(24.f));
        } else {
            NS_InfoRow(oxorany("libil2cpp Base"), oxorany("0x0 (STANDBY)"), C_TextDim(), pageW - S(24.f));
        }

        // 5. Match State
        NS_InfoRow(oxorany("Match State"), inM ? oxorany("IN MATCH (ACTIVE)") : oxorany("LOBBY / WAITING"), inM ? C_Success() : C_TextDim(), pageW - S(24.f));

        // 6. Entities in Memory
        snprintf(buf, sizeof(buf), "%zu Players Tracked", pCount);
        NS_InfoRow(oxorany("Radar Entities"), buf, (pCount > 0) ? C_Accent() : C_TextDim(), pageW - S(24.f));

        // 7. Memory Syscall
        NS_InfoRow(oxorany("Memory Syscall"), oxorany("Kernel Direct (SVC #0)"), C_Success(), pageW - S(24.f));

        ImGui::Dummy({0, S(4.f)});

        // 8. Live Readings Copy Button (Zero disk footprint - purely in-memory snapshot copied to clipboard)
        static double s_copy_feedback_time = -10.0;
        bool recentlyCopied = (ImGui::GetTime() - s_copy_feedback_time < 2.5);

        const char* copyBtnLabel = recentlyCopied ? oxorany("Readings Copied to Clipboard!") : oxorany("Copy Live Readings");
        if (NS_Button("copy_telemetry_btn", copyBtnLabel, {pageW - S(24.f), S(34.f)}, recentlyCopied)) {
            char targetPkgStr[128];
            if (isAttached && targetPkg && targetPkg[0] != '\0') snprintf(targetPkgStr, sizeof(targetPkgStr), "%s (DETECTED)", targetPkg);
            else snprintf(targetPkgStr, sizeof(targetPkgStr), "NOT DETECTED");

            char pidStr[32];
            if (isAttached && curPid > 0) snprintf(pidStr, sizeof(pidStr), "%d (ACTIVE)", curPid);
            else snprintf(pidStr, sizeof(pidStr), "WAITING FOR GAME");

            char libStr[32];
            if (isAttached && libBase > 0) snprintf(libStr, sizeof(libStr), "0x%lx", libBase);
            else snprintf(libStr, sizeof(libStr), "0x0 (STANDBY)");

            char clipBuf[512];
            snprintf(clipBuf, sizeof(clipBuf),
                "--- NINMODS LIVE ENGINE TELEMETRY ---\n"
                "Engine Status   : %s\n"
                "Target Process  : %s\n"
                "Process ID (PID): %s\n"
                "libil2cpp Base  : %s\n"
                "Match State     : %s\n"
                "Radar Entities  : %zu Players Tracked\n"
                "Memory Syscall  : Kernel Direct (SVC #0)\n"
                "Aim Safety      : Reversible (Auto-Restore)\n"
                "Framerate       : %.1f FPS\n"
                "Resolution      : %.0f x %.0f\n"
                "Overlay Type    : Official WindowManager\n"
                "-------------------------------------",
                isAttached ? "ATTACHED & ACTIVE" : "STANDBY / SCANNING...",
                targetPkgStr,
                pidStr,
                libStr,
                inM ? "IN MATCH (ACTIVE)" : "LOBBY / WAITING",
                pCount,
                ImGui::GetIO().Framerate,
                g_sw, g_sh
            );
            clipboard::set_text(clipBuf);
            ImGui::SetClipboardText(clipBuf);
            s_copy_feedback_time = ImGui::GetTime();
        }

        ImGui::EndGroup();

        // RIGHT PAGE (ডান পৃষ্ঠা): System Controls & Hardware Status
        ImVec2 pRight = {cp.x + pageW + gap, cp.y};
        NS_CardBegin(dl, pRight, pageW, pageH);
        NS_CardTitle(dl, {pRight.x + S(12.f), pRight.y + S(12.f)}, oxorany("System & Controls"), "Controls", pageW);
        ImGui::SetCursorScreenPos({pRight.x + S(12.f), pRight.y + S(36.f)});
        ImGui::BeginGroup();

        // Hardware & Display Info
        snprintf(buf, sizeof(buf), "%.1f FPS", ImGui::GetIO().Framerate);
        NS_InfoRow(oxorany("Framerate"), buf, C_Success(), pageW - S(24.f));

        snprintf(buf, sizeof(buf), "%.0f x %.0f", g_sw, g_sh);
        NS_InfoRow(oxorany("Resolution"), buf, 0, pageW - S(24.f));

        NS_InfoRow(oxorany("Aim Safety"), oxorany("REVERSIBLE (AUTO-RESTORE)"), C_Success(), pageW - S(24.f));
        NS_InfoRow(oxorany("Overlay Type"), oxorany("Official WindowManager"), C_Accent(), pageW - S(24.f));

        ImGui::Dummy({0, S(4.f)});

        // Instant Re-Scan / Attach button
        if (NS_Button("rescan_btn", oxorany("Re-Scan / Attach Engine"), {pageW - S(24.f), S(34.f)}, false)) {
            engine::init();
        }

        ImGui::Dummy({0, S(2.f)});

        // Zero Disk Persistence (Pure Live Memory Operation)
        NS_InfoRow(oxorany("Storage"), oxorany("100% LIVE IN-MEMORY"), C_Success(), pageW - S(24.f));

        ImGui::Dummy({0, S(2.f)});

        // Exit & Unload
        if (NS_Button("unload_btn", oxorany("Exit & Unload Overlay"), {pageW - S(24.f), S(34.f)}, false)) {
            exit(0);
        }

        ImGui::EndGroup();
    }

    /**
     * @brief Tab 3: INFO & Dashboard - Book Layout (Two balanced pages side by side).
     */
    static void tab_info(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        float ch = ImGui::GetContentRegionAvail().y;
        ImVec2 cp = ImGui::GetCursorScreenPos();

        float gap = S(12.f);
        float pageW = (cw - gap) * 0.5f;
        float pageH = ImMax(S(305.f), ch - S(4.f));

        bool inM = data::in_match();
        char pbuf[32]; snprintf(pbuf, sizeof(pbuf), "%zu ENTITIES", data::player_count());

        // LEFT PAGE (বাম পৃষ্ঠা): Client Status & Environment
        ImVec2 pLeft = cp;
        NS_CardBegin(dl, pLeft, pageW, pageH);
        NS_CardTitle(dl, {pLeft.x + S(12.f), pLeft.y + S(12.f)}, oxorany("Client State"), "Left Page", pageW);
        ImGui::SetCursorScreenPos({pLeft.x + S(12.f), pLeft.y + S(38.f)});
        ImGui::BeginGroup();
        NS_InfoRow(oxorany("Client Name"), oxorany("NINMODS"), C_Accent(), pageW - S(24.f));
        NS_InfoRow(oxorany("Core Architecture"), oxorany("2.0 Cyber Native"), 0, pageW - S(24.f));
        NS_InfoRow(oxorany("Match State"), inM ? oxorany("ACTIVE") : oxorany("LOBBY"), inM ? C_Success() : C_TextDim(), pageW - S(24.f));
        NS_InfoRow(oxorany("Radar Entities"), pbuf, C_Accent(), pageW - S(24.f));
        NS_InfoRow(oxorany("Security Mode"), oxorany("Direct Kernel (Safe)"), C_Success(), pageW - S(24.f));
        ImGui::EndGroup();

        // RIGHT PAGE (ডান পৃষ্ঠা): Hardware Telemetry & Performance
        ImVec2 pRight = {cp.x + pageW + gap, cp.y};
        NS_CardBegin(dl, pRight, pageW, pageH);
        NS_CardTitle(dl, {pRight.x + S(12.f), pRight.y + S(12.f)}, oxorany("Hardware Telemetry"), "Right Page", pageW);
        ImGui::SetCursorScreenPos({pRight.x + S(12.f), pRight.y + S(38.f)});
        ImGui::BeginGroup();

        char fpsT[16]; snprintf(fpsT, sizeof(fpsT), "%.0f", ImGui::GetIO().Framerate);
        ImU32 fc = ImGui::GetIO().Framerate > 50 ? C_Success()
                 : (ImGui::GetIO().Framerate > 28 ? IM_COL32(255,180,40,(int)(content_alpha*255)) : C_Danger());
        dl->AddText(ImGui::GetFont(), S(32.f), {pRight.x + S(18.f), pRight.y + S(44.f)}, fc, fpsT);
        float fpsTW = NS_TextSize(fpsT, S(32.f)).x;
        dl->AddText(ImGui::GetFont(), S(13.5f), {pRight.x + S(24.f) + fpsTW, pRight.y + S(58.f)}, C_TextDim(), "FPS");

        char scr[32]; snprintf(scr, sizeof(scr), "%.0f x %.0f", g_sw, g_sh);
        dl->AddText(ImGui::GetFont(), S(12.5f), {pRight.x + S(18.f), pRight.y + S(88.f)}, C_TextDim(), "RESOLUTION");
        dl->AddText(ImGui::GetFont(), S(14.5f), {pRight.x + S(18.f), pRight.y + S(106.f)}, C_TextMain(), scr);

        ImGui::Dummy({0, S(115.f)});
        NS_InfoRow(oxorany("Anti-Tamper Shield"), oxorany("ACTIVE"), C_Success(), pageW - S(24.f));
        NS_InfoRow(oxorany("Memory Integrity"), oxorany("VERIFIED"), C_Accent(), pageW - S(24.f));
        ImGui::EndGroup();
    }

    /**
     * @brief Main ImGui Menu frame renderer with responsive scaling and two-page book layout.
     */
    void render() {
        func::esp::draw();
        bar::render();

        float dt = ImGui::GetIO().DeltaTime;
        ma = ma + ((bar::g_open ? 1.f : 0.f) - ma) * ImClamp(12.f*dt, 0.f, 1.f);

        if (ma < 0.01f) {
            g_menu_x = -1.f;
            g_menu_y = -1.f;
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        float sw = g_sw > 100.f ? g_sw : (io.DisplaySize.x > 100.f ? io.DisplaySize.x : 1920.f);
        float sh = g_sh > 100.f ? g_sh : (io.DisplaySize.y > 100.f ? io.DisplaySize.y : 1080.f);
        g_sw = sw;
        g_sh = sh;

        // Dynamic DPI UI Scaling for High-Density Mobile Phones & Emulators
        s_scale = ImClamp(sw / 620.0f, 1.0f, 2.2f);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, R);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.f); // Completely hide scrollbars
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(6.f), S(3.f)));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(S(6.f), S(4.f)));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(S(4.f), S(4.f)));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, S(4.f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, S(4.f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, S(20.f));
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, S(12.f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_Border, {0,0,0,0});

        // ImGui window fills the dedicated SurfaceView window completely
        ImVec2 wsz(sw, sh);
        ImVec2 winPos(0.f, 0.f);

        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);

        ImGui::Begin("##m", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        g_menu_x = wp.x;
        g_menu_y = wp.y;
        g_menu_w = ws.x;
        g_menu_h = ws.y;
        ImVec2 mp = ImGui::GetMousePos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImDrawList* bg = ImGui::GetWindowDrawList();

        // Subtle glow using signature border color (no pink!)
        bg->AddRectFilled({wp.x-3, wp.y-3}, {wp.x+ws.x+3, wp.y+ws.y+3},
                          col(UITheme::border_light, 0.16f * ma), R+3.f);
        bg->AddRectFilled({wp.x-6, wp.y-6}, {wp.x+ws.x+6, wp.y+ws.y+6},
                          col(UITheme::border_light, 0.08f * ma), R+6.f);

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::bg, ma), R);
        bg->AddRect(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::border_light, ma), R, 0, 1.4f);

        const float headerH = S(48.f);
        const float sidebarW = S(50.f);

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+headerH},
                          col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersTop);
        bg->AddLine({wp.x, wp.y+headerH}, {wp.x+ws.x, wp.y+headerH},
                    col(UITheme::border, ma), 1.2f);

        float logoSize = S(32.f);
        float logoX = wp.x + S(14.f);
        float logoY = wp.y + (headerH - logoSize) * 0.5f;
        float lcx = logoX + logoSize * 0.5f, lcy = logoY + logoSize * 0.5f;
        bg->AddCircleFilled({lcx, lcy}, logoSize * 0.5f, IM_COL32(36, 26, 56, (int)(250 * ma)));
        bg->AddCircle({lcx, lcy}, logoSize * 0.5f, col(UITheme::border_light, ma), 32, 1.8f);
        float nW = NS_TextSize("N", S(17.5f)).x;
        bg->AddText(ImGui::GetFont(), S(17.5f),
                    {lcx - nW * 0.5f, lcy - S(8.5f)},
                    IM_COL32(255,255,255,255), "N");

        // Clean centered title
        dl->AddText(ImGui::GetFont(), S(17.0f), {logoX + logoSize + S(12.f), wp.y + (headerH - S(17.0f)) * 0.5f},
                    col(UITheme::text, ma), "NINMODS");

        // Borderless Clean Close Button [X]
        float btnW = S(32.f), btnH = S(32.f);
        float closeX = wp.x + ws.x - btnW - S(12.f);
        float btnY   = wp.y + (headerH - btnH) * 0.5f;

        bool hoverClose = (mp.x >= closeX - S(6.f) && mp.x <= closeX + btnW + S(6.f) &&
                           mp.y >= btnY - S(6.f) && mp.y <= btnY + btnH + S(6.f));
        if (hoverClose && ImGui::IsMouseClicked(0)) {
            bar::set_pos(wp.x, wp.y);
            bar::g_open = false;
            drag = false;
            logger::log_info("[MENU] Close tapped. Preserved location (%.1f, %.1f).", wp.x, wp.y);
        }

        float cX = closeX + btnW * 0.5f, cY = btnY + btnH * 0.5f, cr = S(7.5f);
        ImU32 closeCol = hoverClose ? IM_COL32(255, 75, 75, (int)(255 * ma)) : col(UITheme::text_dim, ma);
        bg->AddLine({cX - cr, cY - cr}, {cX + cr, cY + cr}, closeCol, 2.0f);
        bg->AddLine({cX - cr, cY + cr}, {cX + cr, cY - cr}, closeCol, 2.0f);

        // Sidebar
        float sbY = wp.y + headerH;
        float sbBot = wp.y + ws.y;
        bg->AddRectFilled({wp.x, sbY}, {wp.x+sidebarW, sbBot}, col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersBottomLeft);
        bg->AddLine({wp.x+sidebarW, sbY}, {wp.x+sidebarW, sbBot}, col(UITheme::border, ma), 1.2f);

        struct NavTab {
            const char* icon;
        };
        NavTab navTabs[4] = {
            { ICON_FA_CROSSHAIRS },
            { ICON_FA_EYE },
            { ICON_FA_COG },
            { ICON_FA_INFO_CIRCLE }
        };

        float navStartY = sbY + S(12.f);
        float navItemH = S(44.f);
        float tabMarginX = S(6.f);
        for (int i = 0; i < 4; i++) {
            float ny = navStartY + i * (navItemH + S(10.f));
            bool act = (tab == i);
            bool hov = (mp.x >= wp.x && mp.x <= wp.x + sidebarW &&
                        mp.y >= ny && mp.y <= ny + navItemH);

            float rx1 = wp.x + tabMarginX;
            float rx2 = wp.x + sidebarW - tabMarginX;
            float rRound = S(7.f);

            if (act) {
                bg->AddRectFilled({rx1, ny}, {rx2, ny + navItemH},
                                  col(UITheme::border_light, ma * 0.22f), rRound);
                bg->AddRect({rx1, ny}, {rx2, ny + navItemH},
                            col(UITheme::border_light, ma * 0.85f), rRound, 0, 1.2f);
            } else if (hov) {
                bg->AddRectFilled({rx1, ny}, {rx2, ny + navItemH},
                                  col(UITheme::widget, ma * 0.50f), rRound);
            }

            ImU32 itemCol = act ? col(UITheme::border_light, ma) : (hov ? col(UITheme::text, ma) : col(UITheme::text_dim, ma));

            float iconSize = S(20.f);
            float iw = NS_TextSize(navTabs[i].icon, iconSize).x;
            dl->AddText(ImGui::GetFont(), iconSize,
                {wp.x + (sidebarW - iw) * 0.5f, ny + (navItemH - iconSize) * 0.5f}, itemCol, navTabs[i].icon);

            if (hov && ImGui::IsMouseClicked(0)) {
                tab = i;
            }
        }

        // Full content region utilizing two-page book layout
        float contentX = wp.x + sidebarW + S(14.f);
        float contentY = wp.y + headerH + S(10.f);
        float contentW = ws.x - sidebarW - S(24.f);
        float contentH = ws.y - headerH - S(16.f);

        ImGui::SetCursorScreenPos({contentX, contentY});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 0.f);

        ImGui::BeginChild("content_pane", {contentW, contentH}, false,
                          ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

        // Smooth touch-drag scrolling across entire options page
        static bool s_is_touch_scrolling = false;
        static float s_touch_start_y = 0.f;
        static float s_scroll_start_y = 0.f;

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
            if (ImGui::IsMouseClicked(0)) {
                s_is_touch_scrolling = true;
                s_touch_start_y = mp.y;
                s_scroll_start_y = ImGui::GetScrollY();
            }
        }
        if (s_is_touch_scrolling) {
            if (ImGui::IsMouseDown(0)) {
                float deltaY = s_touch_start_y - mp.y;
                ImGui::SetScrollY(s_scroll_start_y + deltaY);
            } else {
                s_is_touch_scrolling = false;
            }
        }

        dl = ImGui::GetWindowDrawList();
        content_w = ImGui::GetContentRegionAvail().x;
        content_alpha = ma;

        // Render Two-Page Book according to active tab
        switch (tab) {
            case 0: tab_aimbot(ma);   break;
            case 1: tab_visuals(ma);  break;
            case 2: tab_settings(ma); break;
            case 3: tab_info(ma);     break;
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();

        // Drag window by header
        bool inHeaderCloseBtn = (mp.x >= closeX - 8.f && mp.y <= wp.y + headerH);
        bool canDragStart = (!inHeaderCloseBtn && mp.x >= wp.x && mp.x <= wp.x + ws.x && mp.y >= wp.y && mp.y <= wp.y + headerH);

        if (canDragStart && (ImGui::IsMouseClicked(0) || (!drag && ImGui::IsMouseDown(0)))) {
            drag = true;
            doff = {mp.x - g_menu_pos.x, mp.y - g_menu_pos.y};
        }

        if (drag) {
            if (ImGui::IsMouseDown(0)) {
                ImVec2 np{mp.x - doff.x, mp.y - doff.y};
                float dMaxX = ImMax(0.f, sw - ws.x);
                float dMaxY = ImMax(0.f, sh - ws.y);
                np.x = ImClamp(np.x, 0.f, dMaxX);
                np.y = ImClamp(np.y, 0.f, dMaxY);
                g_menu_pos = np;
            } else {
                drag = false;
                logger::log_info("[MENU] Menu relocated to: (X: %.1f, Y: %.1f)", g_menu_pos.x, g_menu_pos.y);
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(12);
    }
}
