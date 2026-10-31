#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "styles/styles.h"
#include "cfg.hpp"
#include "engine/game_data.hpp"
#include "config_store.h"
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
    static ImVec2 g_menu_pos(-1.f, -1.f);

    static constexpr float R  = 14.f;

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

    static inline ImU32 C_TextMain()    { return col(UITheme::text,       content_alpha); }
    static inline ImU32 C_TextSec()     { return col(UITheme::text_dim,   content_alpha); }
    static inline ImU32 C_TextDim()     { return col(UITheme::text_dim,   content_alpha * 0.75f); }
    static inline ImU32 C_BgDark()      { return col(UITheme::bg,         content_alpha); }
    static inline ImU32 C_BgSidebar()   { return col(UITheme::sidebar,    content_alpha); }
    static inline ImU32 C_BgCard()      { return col(UITheme::panel,      content_alpha); }
    static inline ImU32 C_BgCardHov()   { return col(UITheme::widget,     content_alpha); }
    static inline ImU32 C_Border()      { return col(UITheme::border,     content_alpha); }
    static inline ImU32 C_Accent()      { return col(UITheme::accent,     content_alpha); }
    static inline ImU32 C_AccentBright(){ return col(UITheme::accent_light, content_alpha); }
    static inline ImU32 C_AccentDim()   { return col(UITheme::accent, content_alpha * 0.28f); }
    static inline ImU32 C_Success()     { return IM_COL32(52, 211, 110, (int)(content_alpha * 255)); }
    static inline ImU32 C_Danger()      { return IM_COL32(255, 69, 58, (int)(content_alpha * 255)); }

    static void NS_CardBegin(ImDrawList* dl, ImVec2 pos, float w, float h) {
        dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, C_BgCard(), 10.f);
        dl->AddRect(pos, {pos.x+w, pos.y+h}, C_Border(), 10.f, 0, 1.4f);
    }

    static void NS_CardTitle(ImDrawList* dl, ImVec2 pos, const char* title, const char* hint = nullptr) {
        dl->AddRectFilled({pos.x, pos.y+2.f}, {pos.x+4.0f, pos.y+18.f}, C_Accent(), 2.f);
        dl->AddText(ImGui::GetFont(), 16.0f, {pos.x+12.f, pos.y+1.f}, C_TextMain(), title);
        if (hint) {
            float hw = NS_TextSize(hint, 13.5f).x;
            dl->AddText(ImGui::GetFont(), 13.5f,
                {pos.x + ImGui::GetContentRegionAvail().x - hw - 10.f, pos.y+2.f},
                C_Accent(), hint);
        }
    }

    static bool NS_Toggle(const char* id, const char* label, bool* value) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = 42.f;
        ImGui::InvisibleButton("btn", ImVec2(avail, h));
        bool clicked = ImGui::IsItemClicked();
        bool hover = ImGui::IsItemHovered();
        if (clicked) *value = !*value;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            std::string key = std::string(id) + "#t";
            float on = NS_Anim(key+"on", *value ? 1.f : 0.f, 16.f);
            float hov = NS_Anim(key+"hov", hover ? 1.f : 0.f, 18.f);
            ImU32 lblCol = NS_Lerp(C_TextSec(), C_TextMain(), ImMax(on, hov*0.5f));
            dl->AddText(ImGui::GetFont(), 16.0f,
                {pos.x, pos.y + (h - 16.0f) * 0.5f}, lblCol, label);
            float tw = 54.f, th = 26.f;
            float tx = pos.x + avail - tw, ty = pos.y + (h - th) * 0.5f;
            dl->AddRectFilled({tx, ty}, {tx+tw, ty+th},
                NS_Lerp(IM_COL32(40,30,60,(int)(content_alpha*255)), C_Accent(), on), th*0.5f);
            if (on > 0.05f) {
                dl->AddRect({tx, ty}, {tx+tw, ty+th},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)(110*on) << 24),
                    th*0.5f, 0, 1.4f);
            }
            float knobR = th*0.5f - 2.8f;
            float knobX = tx + 2.8f + (tw - th) * on;
            dl->AddCircleFilled({knobX+knobR, ty+th*0.5f}, knobR,
                on > 0.5f ? IM_COL32(255,255,255,255) : IM_COL32(220,210,235,255));
        }
        ImGui::PopID();
        return clicked;
    }

    static bool NS_Slider(const char* id, const char* label, float* value,
                          float minV, float maxV, const char* suffix = "") {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            char valStr[32];
            if (suffix && suffix[0] != '\0') {
                snprintf(valStr, sizeof(valStr), "%.0f%s", *value, suffix);
            } else {
                snprintf(valStr, sizeof(valStr), "%.0f", *value);
            }
            float valW = NS_TextSize(valStr, 15.0f).x;
            dl->AddText(ImGui::GetFont(), 15.5f, {pos.x, pos.y}, C_TextSec(), label);
            dl->AddText(ImGui::GetFont(), 15.0f, {pos.x + avail - valW, pos.y}, C_TextDim(), valStr);

            float ty = pos.y + 24.f, tw2 = avail;
            float rawPct = (maxV > minV) ? ImClamp((*value-minV)/(maxV-minV), 0.f, 1.f) : 0.f;
            float pct = NS_Anim(std::string(id)+"#pct", rawPct, 22.f);

            // Track background
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2, ty+5.f},
                IM_COL32(40, 30, 60, (int)(content_alpha*255)), 2.5f);
            // Track active fill
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2*pct, ty+5.f}, C_Accent(), 2.5f);
            if (pct > 0.01f) {
                dl->AddRect({pos.x, ty}, {pos.x+tw2*pct, ty+5.f},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)90 << 24), 2.5f, 0, 1.f);
            }
            // Thumb knob
            float thumbX = pos.x + tw2 * pct;
            dl->AddCircleFilled({thumbX, ty + 2.5f}, 8.0f, IM_COL32(255, 255, 255, (int)(255 * content_alpha)));
            dl->AddCircle({thumbX, ty + 2.5f}, 8.0f, C_Accent(), 16, 1.8f);

            ImGui::SetCursorScreenPos({pos.x, ty - 6.f});
            ImGui::SetNextItemWidth(tw2);
        }
        ImGui::PushStyleColor(ImGuiCol_FrameBg,          IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrab,       IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(0,0,0,0));
        bool changed = ImGui::SliderFloat("##s", value, minV, maxV, "");
        ImGui::PopStyleColor(5);
        ImGui::Dummy(ImVec2(0, 14.f));
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
            dl->AddRectFilled(a, b, bg, 8.f);
            if (accent && hov > 0.1f) {
                dl->AddRect(a, b, (C_AccentBright() & 0x00FFFFFF) | ((ImU32)(120*hov) << 24),
                            8.f, 0, 1.2f);
            }
            if (!accent) {
                dl->AddRect(a, b, C_Border(), 8.f, 0, 1.2f);
            }
            float lw = NS_TextSize(label, 15.5f).x;
            dl->AddText(ImGui::GetFont(), 15.5f,
                {pos.x + (size.x-lw)*0.5f, pos.y + (size.y - 15.5f)*0.5f},
                accent ? IM_COL32(255,255,255,255) : C_TextMain(), label);
        }
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("btn", size);
        bool clicked = ImGui::IsItemClicked();
        ImGui::Dummy(size);
        ImGui::PopID();
        return clicked;
    }

    static void NS_InfoRow(const char* label, const char* value, ImU32 valueColor = 0) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float avail = ImGui::GetContentRegionAvail().x;
        float rh = 36.f;
        bool hov = ImGui::IsMouseHoveringRect(pos, {pos.x+avail, pos.y+rh});
        if (dl) {
            if (hov) {
                dl->AddRectFilled({pos.x, pos.y}, {pos.x+avail, pos.y+rh}, C_BgCardHov(), 4.f);
                dl->AddRectFilled({pos.x, pos.y+6}, {pos.x+3.5f, pos.y+rh-6}, C_Accent(), 1.5f);
            }
            dl->AddText(ImGui::GetFont(), 15.0f, {pos.x+10.f, pos.y + (rh - 15.0f)*0.5f}, C_TextDim(), label);
            float vw = NS_TextSize(value, 15.0f).x;
            ImU32 fc = valueColor ? valueColor : C_TextMain();
            dl->AddText(ImGui::GetFont(), 15.0f, {pos.x+avail-vw-12.f, pos.y + (rh - 15.0f)*0.5f}, fc, value);
        }
        ImGui::Dummy(ImVec2(0, rh+2.f));
    }

    static bool NS_ComboRow(const char* id, const char* label, int* value,
                            const char** items, int itemCount) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = 40.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            dl->AddRectFilled(pos, {pos.x+avail, pos.y+h}, C_BgDark(), 6.f);
            dl->AddRect(pos, {pos.x+avail, pos.y+h}, C_Border(), 6.f, 0, 1.f);
            dl->AddText(ImGui::GetFont(), 15.0f, {pos.x+12.f, pos.y + (h - 15.0f)*0.5f}, C_TextDim(), label);
            int cur = *value;
            const char* curText = (cur>=0 && cur<itemCount) ? items[cur] : "?";
            float cw = NS_TextSize(curText, 15.0f).x;
            dl->AddText(ImGui::GetFont(), 15.0f,
                {pos.x+avail-cw-32.f, pos.y + (h - 15.0f)*0.5f}, C_TextMain(), curText);
            dl->AddText(ImGui::GetFont(), 12.f,
                {pos.x+avail-20.f, pos.y + (h - 12.f)*0.5f}, C_TextDim(), "\u25BC");
        }
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg,        C_BgCard());
        ImGui::PushStyleColor(ImGuiCol_Header,         C_Accent());
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  C_AccentBright());
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,   C_AccentBright());
        ImGui::PushStyleColor(ImGuiCol_Text,           C_TextMain());
        ImGui::SetCursorScreenPos({pos.x+4, pos.y+4});
        ImGui::SetNextItemWidth(avail-8);
        bool changed = ImGui::Combo("##combo", value, items, itemCount);
        ImGui::PopStyleColor(8);
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 4.f));
        return changed;
    }

    static void NS_ColorEdit(const char* id, const char* label, ImVec4* color) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            dl->AddText(ImGui::GetFont(), 15.0f, {pos.x, pos.y+6.f}, C_TextSec(), label);
            ImU32 colU32 = ImGui::ColorConvertFloat4ToU32(*color);
            float sw = 30.f;
            ImVec2 sp = {pos.x + avail - sw - 8.f, pos.y + 2.f};
            dl->AddRectFilled(sp, {sp.x+sw, sp.y+sw}, colU32, 6.f);
            dl->AddRect(sp, {sp.x+sw, sp.y+sw}, C_Border(), 6.f, 0, 1.2f);
        }
        ImGui::SetCursorScreenPos({pos.x + avail - 70.f, pos.y + 2.f});
        ImGui::ColorEdit4("##ce_edit", &color->x,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
            ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview);
        ImGui::Dummy({0, 6.f});
        ImGui::PopID();
    }

    /**
     * @brief Tab 0: Combat & Aim Assist configuration. Features in-menu FOV radar preview.
     */
    static void tab_aimbot(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        ImVec2 cp = ImGui::GetCursorScreenPos();

        float card1H = 150.f;
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("Aim Silent and Nobita"), "Combat");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        ImGui::PushID("aim_toggles");
        float colW = (cw - 32 - 24) * 0.5f;
        ImGui::Columns(2, "aim_cols_id", false);
        ImGui::SetColumnWidth(0, colW + 12);
        ImGui::SetColumnWidth(1, colW + 12);

        NS_Toggle("aim_silent", oxorany("Aim Silent"),  &cfg::aim::silent);
        NS_Toggle("aim_nobita", oxorany("Nobita Mode"), &cfg::aim::nobita);

        ImGui::NextColumn();

        NS_Toggle("aim_draw",   oxorany("FOV Circle"),  &cfg::aim::draw_fov);

        // Status badge
        {
            bool active = cfg::aim::silent || cfg::aim::nobita;
            char st[64];
            snprintf(st, sizeof(st), "%s", active ? "ACTIVE" : "STANDBY");
            ImU32 stCol = active ? C_Success() : C_TextDim();
            float sw = NS_TextSize(st, 14.0f).x;
            dl->AddText(ImGui::GetFont(), 14.0f,
                {cp.x + cw - sw - 16, cp.y + 14}, stCol, st);
        }

        ImGui::Columns(1);
        ImGui::PopID();

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0,0});

        // Card 2: Aim Parameters and Bones
        float card2H = 270.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Target and Trigger Settings"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        const char* bones[] = { oxorany("Head"), oxorany("Chest"), oxorany("Hip") };
        NS_ComboRow("aim_bone", oxorany("Target Bone"), &cfg::aim::target_bone, bones, 3);

        const char* triggers[] = { oxorany("On Fire"), oxorany("On Scope"), oxorany("Always") };
        NS_ComboRow("aim_trig", oxorany("Trigger Mode"), &cfg::aim::trigger_type, triggers, 3);

        NS_Slider("aim_fov",  oxorany("FOV Radius"),   &cfg::aim::fov,      30.f, 350.f, "px");
        NS_Slider("aim_dist", oxorany("Aim Distance"), &cfg::aim::distance, 30.f, 400.f, "m");
        NS_Slider("aim_smth", oxorany("Aim Smooth"),   &cfg::aim::smooth,   1.f,  20.f, "");

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+14});
        ImGui::Dummy({0, 30});

        cfg::aim::enabled = cfg::aim::silent || cfg::aim::nobita;
    }

    static void tab_visuals(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float colW = (cw - 32 - 24) * 0.5f;
        float card1H = 175.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("ESP Visual Overlays"), "Master");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+48});

        NS_Toggle("esp_tracer",  oxorany("ESP Line"),   &cfg::esp::tracer);
        NS_Toggle("esp_box",     oxorany("ESP Box"),    &cfg::esp::box);
        NS_Toggle("esp_health",  oxorany("ESP Health"), &cfg::esp::health);

        cfg::esp::enabled = cfg::esp::tracer || cfg::esp::box || cfg::esp::health;

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0,0});

        float card2H = 260.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Box & Line Configuration"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+48});

        const char* box_types[] = { oxorany("Corner"), oxorany("Full"), oxorany("Glow") };
        NS_ComboRow("box_type", oxorany("Box Style"), &cfg::esp::box_type, box_types, 3);
        NS_Slider("box_th", oxorany("Thickness"),  &cfg::esp::box_thickness, 0.5f, 5.0f, "");
        NS_Slider("box_rd", oxorany("Rounding"),   &cfg::esp::box_rounding,  0.f,  10.f, "");
        NS_Slider("box_rg", oxorany("Max Range"),  &cfg::esp::max_range,     50.f, 500.f, "m");

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+14});
        ImGui::Dummy({0,0});

        float card3H = 145.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Color Palette"));
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+48});
        NS_ColorEdit("col_box",  oxorany("Box & Line"), &cfg::esp::box_col);
        NS_ColorEdit("col_vis",  oxorany("Visible"),    &cfg::esp::box_visible);

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+14});
        ImGui::Dummy({0, 30});
    }

    static void tab_settings(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        ImVec2 cp = ImGui::GetCursorScreenPos();

        float card1H = 135.f;
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("Display Engine"));
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+48});

        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f x %.0f", g_sw, g_sh);
        NS_InfoRow(oxorany("Resolution"), buf);
        snprintf(buf, sizeof(buf), "%.1f FPS", ImGui::GetIO().Framerate);
        NS_InfoRow(oxorany("Framerate"), buf);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0,0});

        float card2H = 135.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Target Process"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+48});

        snprintf(buf, sizeof(buf), "0x%lx", proc::lib);
        NS_InfoRow(oxorany("libil2cpp Base"), buf);
        snprintf(buf, sizeof(buf), "%d", proc::pid);
        NS_InfoRow(oxorany("Process ID"), buf);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+14});
        ImGui::Dummy({0,0});

        float card3H = 105.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Client Termination"));
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+48});

        if (NS_Button("unload_btn", oxorany("Exit & Unload Overlay"), {cw-32, 40}, true)) {
            exit(0);
        }

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+14});
        ImGui::Dummy({0, 30});
    }

    /**
     * @brief Tab 3: INFO / Dashboard. Renders 4 website-style metric stat cards in one row.
     */
    static void tab_info(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        ImVec2 cp = ImGui::GetCursorScreenPos();

        float gap = 10.f;
        float cardW = (cw - 3 * gap) * 0.25f;
        float cardH = 82.f;

        struct MetricCard {
            const char* title;
            const char* val;
            ImU32 col;
        };

        char pidStr[32];
        if (proc::pid > 0) snprintf(pidStr, sizeof(pidStr), "%d", proc::pid);
        else snprintf(pidStr, sizeof(pidStr), "Searching");

        bool inM = data::in_match();

        MetricCard metrics[4] = {
            { "CLIENT",     "NINMODS",               C_Accent() },
            { "VERSION",    "2.0",                   C_TextMain() },
            { "TARGET PID", pidStr,                  (proc::pid > 0 ? C_TextMain() : C_TextDim()) },
            { "MATCH",      (inM ? "ACTIVE" : "LOBBY"), (inM ? C_Success() : C_TextDim()) }
        };

        for (int i = 0; i < 4; i++) {
            ImVec2 p = { cp.x + i * (cardW + gap), cp.y };
            NS_CardBegin(dl, p, cardW, cardH);

            dl->AddText(ImGui::GetFont(), 13.0f, {p.x + 12.f, p.y + 12.f}, C_TextDim(), metrics[i].title);

            float vw = NS_TextSize(metrics[i].val, 17.0f).x;
            float vx = ImMax(p.x + 12.f, p.x + (cardW - vw) * 0.5f);
            dl->AddText(ImGui::GetFont(), 17.0f, {vx, p.y + 40.f}, metrics[i].col, metrics[i].val);
        }

        ImGui::SetCursorScreenPos({cp.x, cp.y + cardH + 16.f});
        ImGui::Dummy({0,0});

        float card2H = 115.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Performance & Live Display"));

        char fpsT[16]; snprintf(fpsT, sizeof(fpsT), "%.0f", ImGui::GetIO().Framerate);
        ImU32 fc = ImGui::GetIO().Framerate > 50 ? C_Success()
                 : (ImGui::GetIO().Framerate > 28 ? IM_COL32(255,180,40,(int)(content_alpha*255)) : C_Danger());
        dl->AddText(ImGui::GetFont(), 36.f, {cp2.x+20, cp2.y+46}, fc, fpsT);
        float fpsTW = NS_TextSize(fpsT, 36.f).x;
        dl->AddText(ImGui::GetFont(), 14.f, {cp2.x+28+fpsTW, cp2.y+62}, C_TextDim(), "FPS");

        char scr[32]; snprintf(scr, sizeof(scr), "%.0f x %.0f", g_sw, g_sh);
        float sx = cp2.x + cw * 0.40f;
        dl->AddText(ImGui::GetFont(), 13.0f, {sx, cp2.y+48}, C_TextDim(), "DISPLAY RESOLUTION");
        dl->AddText(ImGui::GetFont(), 16.0f, {sx, cp2.y+72}, C_TextMain(), scr);

        char pbuf[32]; snprintf(pbuf, sizeof(pbuf), "%zu ENTITIES", data::player_count());
        float px = cp2.x + cw * 0.72f;
        dl->AddText(ImGui::GetFont(), 13.0f, {px, cp2.y+48}, C_TextDim(), "RADAR TRACKED");
        dl->AddText(ImGui::GetFont(), 16.0f, {px, cp2.y+72}, C_Accent(), pbuf);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y + card2H + 16.f});
        ImGui::Dummy({0,0});

        float card3H = 175.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("System Architecture & Security"));
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+48});

        char libBuf[64]; snprintf(libBuf, sizeof(libBuf), "0x%lx", (unsigned long)proc::lib);
        NS_InfoRow(oxorany("libil2cpp Base"), libBuf, C_Accent());
        NS_InfoRow(oxorany("Protection Layer"), oxorany("NativeShield Active"), C_Success());
        NS_InfoRow(oxorany("Memory Bus"), oxorany("process_vm_readv (Direct Kernel)"));
        NS_InfoRow(oxorany("Streamer Mode"), cfg::streamer_mode ? oxorany("ACTIVE (Screen-Record Blocked)") : oxorany("OFF (Recording Allowed)"), cfg::streamer_mode ? C_Success() : C_TextDim());

        ImGui::SetCursorScreenPos({cp3.x, cp3.y + card3H + 16.f});
        ImGui::Dummy({0, 30});
    }

    /**
     * @brief Main ImGui Menu frame renderer with mobile scaling and light-purple visual styling.
     */
    void render() {
        func::esp::draw();
        bar::render();

        float dt = ImGui::GetIO().DeltaTime;
        ma = ma + ((bar::g_open ? 1.f : 0.f) - ma) * ImClamp(12.f*dt, 0.f, 1.f);

        // Dim background only lightly without blocking ESP
        if (ma > 0.01f) {
            int da = (int)(60 * ma * bar::game_alpha());
            ImGui::GetBackgroundDrawList()->AddRectFilled({0,0},{g_sw,g_sh},IM_COL32(0,0,0,da));
        }
        if (ma < 0.01f) return;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, R);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 10.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 5.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 5.f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 22.f);
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 14.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_Border, {0,0,0,0});

        ImVec2 wsz(860.f, 530.f);
        if (wsz.x > g_sw && g_sw > 0.f) wsz.x = g_sw * 0.94f;
        if (wsz.y > g_sh && g_sh > 0.f) wsz.y = g_sh * 0.94f;
        
        static bool s_menu_pos_init = false;
        if (!s_menu_pos_init && g_sw > 0.f && g_sh > 0.f) {
            g_menu_pos = {(g_sw - wsz.x) * 0.5f, (g_sh - wsz.y) * 0.5f};
            s_menu_pos_init = true;
        }
        if (g_sw > 0.f)
            g_menu_pos.x = ImClamp(g_menu_pos.x, 0.f, ImMax(0.f, g_sw - wsz.x));
        if (g_sh > 0.f)
            g_menu_pos.y = ImClamp(g_menu_pos.y, 0.f, ImMax(0.f, g_sh - wsz.y));

        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        ImGui::SetNextWindowPos(g_menu_pos, ImGuiCond_Always);

        ImGui::Begin("##m", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImVec2 mp = ImGui::GetMousePos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImDrawList* bg = ImGui::GetWindowDrawList();

        bg->AddRectFilled({wp.x-4, wp.y-4}, {wp.x+ws.x+4, wp.y+ws.y+4},
                          IM_COL32(180, 105, 255, (int)(45 * ma)), R+4.f);
        bg->AddRectFilled({wp.x-10, wp.y-10}, {wp.x+ws.x+10, wp.y+ws.y+10},
                          IM_COL32(180, 105, 255, (int)(18 * ma)), R+8.f);

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::bg, ma), R);
        bg->AddRect(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::border_light, ma), R, 0, 1.5f);

        const float headerH = 64.f;
        const float sidebarW = 135.f;
        const float footerH = 48.f;

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+headerH},
                          col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersTop);
        bg->AddLine({wp.x, wp.y+headerH}, {wp.x+ws.x, wp.y+headerH},
                    col(UITheme::border, ma), 1.2f);

        float logoSize = 38.f;
        float logoX = wp.x + 18.f;
        float logoY = wp.y + (headerH - logoSize) * 0.5f;
        float lcx = logoX + logoSize * 0.5f, lcy = logoY + logoSize * 0.5f;
        bg->AddCircleFilled({lcx, lcy}, logoSize * 0.5f, IM_COL32(45, 30, 75, (int)(250 * ma)));
        bg->AddCircle({lcx, lcy}, logoSize * 0.5f, col(UITheme::accent, ma), 32, 2.0f);
        float nW = NS_TextSize("N", 20.f).x;
        bg->AddText(ImGui::GetFont(), 20.f,
                    {lcx - nW * 0.5f, lcy - 10.f},
                    IM_COL32(255,255,255,255), "N");

        dl->AddText(ImGui::GetFont(), 17.5f, {logoX + logoSize + 14.f, wp.y + 12.f},
                    col(UITheme::text, ma), "NINMODS STANDALONE");
        dl->AddText(ImGui::GetFont(), 13.0f, {logoX + logoSize + 14.f, wp.y + 36.f},
                    col(UITheme::accent_light, ma), "Global 64/32");

        // Window control buttons: [ ] (center), — (minimize), X (close) - Exactly matching Mockup Image 2
        float btnW = 42.f, btnH = 42.f;
        float closeX = wp.x + ws.x - btnW - 12.f;
        float minX   = closeX - btnW - 6.f;
        float winX   = minX - btnW - 6.f;
        float btnY   = wp.y + (headerH - btnH) * 0.5f;

        // 1. Close Button [X]
        bool hoverClose = (mp.x >= closeX - 4.f && mp.x <= closeX + btnW + 4.f &&
                           mp.y >= btnY - 4.f && mp.y <= btnY + btnH + 4.f);
        if (hoverClose) {
            bg->AddRectFilled({closeX, btnY}, {closeX + btnW, btnY + btnH},
                              col(UITheme::danger, ma * 0.85f), 6.f);
            if (ImGui::IsMouseClicked(0)) {
                bar::g_open = false;
                drag = false;
                storage::save_config();
                logger::log_info("[MENU] Close button triggered. Config saved. Switched to floating puck.");
            }
        }
        float cX = closeX + btnW * 0.5f, cY = btnY + btnH * 0.5f, cr = 7.f;
        ImU32 closeCol = hoverClose ? IM_COL32(255, 255, 255, 255) : col(UITheme::text_dim, ma);
        bg->AddLine({cX - cr, cY - cr}, {cX + cr, cY + cr}, closeCol, 2.2f);
        bg->AddLine({cX - cr, cY + cr}, {cX + cr, cY - cr}, closeCol, 2.2f);

        // 2. Minimize Button [—]
        bool hoverMin = (mp.x >= minX - 4.f && mp.x <= minX + btnW + 4.f &&
                         mp.y >= btnY - 4.f && mp.y <= btnY + btnH + 4.f);
        if (hoverMin) {
            bg->AddRectFilled({minX, btnY}, {minX + btnW, btnY + btnH},
                              col(UITheme::widget, ma * 0.85f), 6.f);
            if (ImGui::IsMouseClicked(0)) {
                bar::g_open = false;
                drag = false;
                storage::save_config();
                logger::log_info("[MENU] Minimize button triggered. Config saved. Switched to floating puck.");
            }
        }
        float mX = minX + btnW * 0.5f, mY = btnY + btnH * 0.5f, mr = 7.f;
        ImU32 minCol = hoverMin ? IM_COL32(255, 255, 255, 255) : col(UITheme::text_dim, ma);
        bg->AddLine({mX - mr, mY}, {mX + mr, mY}, minCol, 2.2f);

        // 3. Center / Window Button [ ]
        bool hoverWin = (mp.x >= winX - 4.f && mp.x <= winX + btnW + 4.f &&
                         mp.y >= btnY - 4.f && mp.y <= btnY + btnH + 4.f);
        if (hoverWin) {
            bg->AddRectFilled({winX, btnY}, {winX + btnW, btnY + btnH},
                              col(UITheme::widget, ma * 0.85f), 6.f);
            if (ImGui::IsMouseClicked(0)) {
                g_menu_pos = {(g_sw - wsz.x) * 0.5f, (g_sh - wsz.y) * 0.5f};
                logger::log_info("[MENU] Menu recentered on screen.");
            }
        }
        float wX = winX + btnW * 0.5f, wY = btnY + btnH * 0.5f, wr = 6.f;
        ImU32 winCol = hoverWin ? IM_COL32(255, 255, 255, 255) : col(UITheme::text_dim, ma);
        bg->AddRect({wX - wr, wY - wr}, {wX + wr, wY + wr}, winCol, 2.f, 0, 1.8f);

        float sbY = wp.y + headerH;
        float sbBot = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, sbY}, {wp.x+sidebarW, sbBot}, col(UITheme::sidebar, ma));
        bg->AddLine({wp.x+sidebarW, sbY}, {wp.x+sidebarW, sbBot}, col(UITheme::border, ma), 1.2f);

        struct NavTab {
            const char* icon;
            const char* label;
        };
        NavTab navTabs[4] = {
            { ICON_FA_CROSSHAIRS, "AIMBOT" },
            { ICON_FA_EYE,        "VISUALS" },
            { ICON_FA_COG,        "SETTINGS" },
            { ICON_FA_INFO_CIRCLE,"INFO" }
        };

        float navStartY = sbY + 16.f;
        float navItemH = 64.f;
        for (int i = 0; i < 4; i++) {
            float ny = navStartY + i * (navItemH + 8.f);
            bool act = (tab == i);
            bool hov = (mp.x >= wp.x && mp.x <= wp.x + sidebarW &&
                        mp.y >= ny && mp.y <= ny + navItemH);

            if (act) {
                bg->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+navItemH},
                                  col(UITheme::accent, ma * 0.25f));
                bg->AddRectFilled({wp.x, ny}, {wp.x+4.0f, ny+navItemH},
                                  col(UITheme::accent, ma));
            } else if (hov) {
                bg->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+navItemH},
                                  col(UITheme::widget, ma * 0.65f));
            }

            ImU32 itemCol = act ? col(UITheme::accent_light, ma) : (hov ? col(UITheme::text, ma) : col(UITheme::text_dim, ma));

            float iw = NS_TextSize(navTabs[i].icon, 20.f).x;
            dl->AddText(ImGui::GetFont(), 20.f,
                {wp.x + (sidebarW - iw) * 0.5f, ny + 12.f}, itemCol, navTabs[i].icon);

            float lw = NS_TextSize(navTabs[i].label, 13.0f).x;
            dl->AddText(ImGui::GetFont(), 13.0f,
                {wp.x + (sidebarW - lw) * 0.5f, ny + 38.f}, itemCol, navTabs[i].label);

            if (hov && ImGui::IsMouseClicked(0)) {
                tab = i;
            }
        }

        float contentX = wp.x + sidebarW + 22.f;
        float contentY = wp.y + headerH + 20.f;
        float contentW = ws.x - sidebarW - 40.f;
        float contentH = ws.y - headerH - footerH - 36.f;

        ImGui::SetCursorScreenPos({contentX, contentY});

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,           IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,         col(UITheme::accent, ma * 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,  col(UITheme::accent_light, ma));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,   col(UITheme::accent_light, ma));

        ImGui::BeginChild("content_pane", {contentW, contentH}, false,
                          ImGuiWindowFlags_NoBackground);

        dl = ImGui::GetWindowDrawList();
        content_w = ImGui::GetContentRegionAvail().x;
        content_alpha = ma;

        switch (tab) {
            case 0: tab_aimbot(ma);   break;
            case 1: tab_visuals(ma);  break;
            case 2: tab_settings(ma); break;
            case 3: tab_info(ma);     break;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor(4);

        float fy = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, fy}, {wp.x+ws.x, wp.y+ws.y},
                          col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersBottom);
        bg->AddLine({wp.x, fy}, {wp.x+ws.x, fy}, col(UITheme::border, ma), 1.2f);

        float fDotX = wp.x + 22.f, fDotY = fy + footerH * 0.5f;
        bg->AddCircleFilled({fDotX, fDotY}, 5.5f, IM_COL32(52, 211, 110, (int)(255 * ma)));
        bg->AddCircle({fDotX, fDotY}, 8.0f, IM_COL32(52, 211, 110, (int)(90 * ma)), 16, 1.4f);
        dl->AddText(ImGui::GetFont(), 14.0f,
            {fDotX + 16.f, fDotY - 7.0f},
            col(UITheme::text, ma), "PROTECTION ACTIVE");

        const char* extText = "External";
        float extW = NS_TextSize(extText, 13.0f).x;
        float bW = 66.f, bH = 26.f;
        float bX = wp.x + ws.x - bW - 20.f;
        float bY = fy + (footerH - bH) * 0.5f;

        dl->AddText(ImGui::GetFont(), 13.0f, {bX - extW - 14.f, fy + (footerH - 13.0f) * 0.5f}, C_TextDim(), extText);
        bg->AddRectFilled({bX, bY}, {bX + bW, bY + bH}, IM_COL32(20, 60, 35, (int)(230 * ma)), 5.f);
        bg->AddRect({bX, bY}, {bX + bW, bY + bH}, IM_COL32(52, 211, 110, (int)(180 * ma)), 5.f, 0, 1.2f);
        float actW = NS_TextSize("ACTIVE", 12.5f).x;
        dl->AddText(ImGui::GetFont(), 12.5f, {bX + (bW - actW) * 0.5f, bY + (bH - 12.5f) * 0.5f},
                    IM_COL32(52, 211, 110, (int)(255 * ma)), "ACTIVE");

        bool inHeaderControls = (mp.x >= winX - 10.f && mp.y <= wp.y + headerH);
        if (!inHeaderControls && (mp.x >= wp.x && mp.x <= wp.x + ws.x && mp.y >= wp.y && mp.y <= wp.y + headerH)) {
            if (ImGui::IsMouseClicked(0) || (!drag && ImGui::IsMouseDown(0))) {
                drag = true;
                doff = {mp.x - g_menu_pos.x, mp.y - g_menu_pos.y};
            }
        }
        if (drag) {
            if (ImGui::IsMouseDown(0)) {
                ImVec2 np{mp.x-doff.x, mp.y-doff.y};
                float maxX = ImMax(0.f, g_sw - ws.x);
                float maxY = ImMax(0.f, g_sh - ws.y);
                np.x = ImClamp(np.x, 0.f, maxX);
                np.y = ImClamp(np.y, 0.f, maxY);
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
