#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "styles/styles.h"
#include "cfg.hpp"
#include "engine/game_data.hpp"
#include "config_store.h"
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

    static constexpr float R  = 10.f;
    static constexpr float Rt = 6.f;

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
    static inline ImU32 C_TextDim()     { return col(UITheme::text_dim,   content_alpha * 0.7f); }
    static inline ImU32 C_BgDark()      { return col(UITheme::bg,         content_alpha); }
    static inline ImU32 C_BgSidebar()   { return col(UITheme::sidebar,    content_alpha); }
    static inline ImU32 C_BgCard()      { return col(UITheme::panel,      content_alpha); }
    static inline ImU32 C_BgCardHov()   { return col(UITheme::widget,     content_alpha); }
    static inline ImU32 C_Border()      { return col(UITheme::border,     content_alpha); }
    static inline ImU32 C_Accent()      { return col(UITheme::accent,     content_alpha); }
    static inline ImU32 C_AccentBright(){ return col(UITheme::accent_light, content_alpha); }
    static inline ImU32 C_AccentDim()   { return col(UITheme::accent, content_alpha * 0.25f); }
    static inline ImU32 C_Success()     { return IM_COL32(52, 199, 89, (int)(content_alpha * 255)); }
    static inline ImU32 C_Danger()      { return IM_COL32(255, 69, 58, (int)(content_alpha * 255)); }

    static void NS_CardBegin(ImDrawList* dl, ImVec2 pos, float w, float h) {
        dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, C_BgCard(), 8.f);
        dl->AddRect(pos, {pos.x+w, pos.y+h}, C_Border(), 8.f, 0, 1.f);
    }

    static void NS_CardTitle(ImDrawList* dl, ImVec2 pos, const char* title, const char* hint = nullptr) {
        dl->AddRectFilled({pos.x, pos.y+2.f}, {pos.x+3.5f, pos.y+17.f}, C_Accent(), 2.f);
        dl->AddText(ImGui::GetFont(), 15.0f, {pos.x+10.f, pos.y+1.f}, C_TextSec(), title);
        if (hint) {
            float hw = NS_TextSize(hint, 13.0f).x;
            dl->AddText(ImGui::GetFont(), 13.0f,
                {pos.x + ImGui::GetContentRegionAvail().x - hw - 10.f, pos.y+2.f},
                C_TextDim(), hint);
        }
    }

    static bool NS_Toggle(const char* id, const char* label, bool* value) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = 36.f;
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
            dl->AddText(ImGui::GetFont(), 15.f,
                {pos.x, pos.y + (h - 15.f) * 0.5f}, lblCol, label);
            float tw = 44.f, th = 22.f;
            float tx = pos.x + avail - tw, ty = pos.y + (h - th) * 0.5f;
            dl->AddRectFilled({tx, ty}, {tx+tw, ty+th},
                NS_Lerp(IM_COL32(42,42,50,(int)(content_alpha*255)), C_Accent(), on), th*0.5f);
            if (on > 0.05f) {
                dl->AddRect({tx, ty}, {tx+tw, ty+th},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)(90*on) << 24),
                    th*0.5f, 0, 1.2f);
            }
            float knobR = th*0.5f - 2.5f;
            float knobX = tx + 2.5f + (tw - th) * on;
            dl->AddCircleFilled({knobX+knobR, ty+th*0.5f}, knobR,
                on > 0.5f ? IM_COL32(255,255,255,255) : IM_COL32(200,200,210,255));
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
            char valStr[32]; snprintf(valStr, sizeof(valStr), "%.1f%s", *value, suffix);
            float valW = NS_TextSize(valStr, 13.5f).x + 16.f;
            dl->AddText(ImGui::GetFont(), 15.f, {pos.x, pos.y+2.f}, C_TextSec(), label);
            float vx = pos.x + avail - valW;
            dl->AddRectFilled({vx, pos.y}, {vx+valW, pos.y+22.f}, C_BgDark(), 4.f);
            dl->AddRect({vx, pos.y}, {vx+valW, pos.y+22.f}, C_Border(), 4.f, 0, 1.f);
            dl->AddText(ImGui::GetFont(), 13.5f,
                {vx + 8.f, pos.y + 4.f}, C_TextMain(), valStr);

            float ty = pos.y + 32.f, tw2 = avail;
            float rawPct = (maxV > minV) ? ImClamp((*value-minV)/(maxV-minV),0.f,1.f) : 0.f;
            float pct = NS_Anim(std::string(id)+"#pct", rawPct, 22.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2, ty+4.f},
                IM_COL32(42,42,50,(int)(content_alpha*255)), 2.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2*pct, ty+4.f}, C_Accent(), 2.f);
            if (pct > 0.01f) {
                dl->AddRect({pos.x, ty}, {pos.x+tw2*pct, ty+4.f},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)90 << 24), 2.f, 0, 1.f);
            }
            dl->AddCircleFilled({pos.x+tw2*pct, ty+2.f}, 7.5f, IM_COL32(255,255,255,255));
            dl->AddCircle({pos.x+tw2*pct, ty+2.f}, 7.5f, C_Accent(), 16, 1.5f);
            ImGui::SetCursorScreenPos({pos.x, ty-8.f});
            ImGui::SetNextItemWidth(tw2);
        }
        bool changed = ImGui::SliderFloat("##s", value, minV, maxV, "");
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
            dl->AddRectFilled(a, b, bg, 6.f);
            if (accent && hov > 0.1f) {
                dl->AddRect(a, b, (C_AccentBright() & 0x00FFFFFF) | ((ImU32)(120*hov) << 24),
                            6.f, 0, 1.f);
            }
            if (!accent) {
                dl->AddRect(a, b, C_Border(), 6.f, 0, 1.f);
            }
            float lw = NS_TextSize(label, 14.5f).x;
            dl->AddText(ImGui::GetFont(), 14.5f,
                {pos.x + (size.x-lw)*0.5f, pos.y + (size.y - 14.5f)*0.5f},
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
        float rh = 34.f;
        bool hov = ImGui::IsMouseHoveringRect(pos, {pos.x+avail, pos.y+rh});
        if (dl) {
            if (hov) {
                dl->AddRectFilled({pos.x, pos.y}, {pos.x+avail, pos.y+rh}, C_BgCardHov(), 4.f);
                dl->AddRectFilled({pos.x, pos.y+6}, {pos.x+3.f, pos.y+rh-6}, C_Accent(), 1.5f);
            }
            dl->AddText(ImGui::GetFont(), 14.5f, {pos.x+10.f, pos.y + (rh - 14.5f)*0.5f}, C_TextDim(), label);
            float vw = NS_TextSize(value, 14.5f).x;
            ImU32 fc = valueColor ? valueColor : C_TextMain();
            dl->AddText(ImGui::GetFont(), 14.5f, {pos.x+avail-vw-12.f, pos.y + (rh - 14.5f)*0.5f}, fc, value);
        }
        ImGui::Dummy(ImVec2(0, rh+2.f));
    }

    static bool NS_ComboRow(const char* id, const char* label, int* value,
                            const char** items, int itemCount) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = 38.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            dl->AddRectFilled(pos, {pos.x+avail, pos.y+h}, C_BgDark(), 6.f);
            dl->AddRect(pos, {pos.x+avail, pos.y+h}, C_Border(), 6.f, 0, 1.f);
            dl->AddText(ImGui::GetFont(), 14.5f, {pos.x+12.f, pos.y + (h - 14.5f)*0.5f}, C_TextDim(), label);
            int cur = *value;
            const char* curText = (cur>=0 && cur<itemCount) ? items[cur] : "?";
            float cw = NS_TextSize(curText, 14.5f).x;
            dl->AddText(ImGui::GetFont(), 14.5f,
                {pos.x+avail-cw-32.f, pos.y + (h - 14.5f)*0.5f}, C_TextMain(), curText);
            dl->AddText(ImGui::GetFont(), 11.f,
                {pos.x+avail-20.f, pos.y + (h - 11.f)*0.5f}, C_TextDim(), "\u25BC");
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
            dl->AddText(ImGui::GetFont(), 14.5f, {pos.x, pos.y+6.f}, C_TextSec(), label);
            ImU32 colU32 = ImGui::ColorConvertFloat4ToU32(*color);
            float sw = 28.f;
            ImVec2 sp = {pos.x + avail - sw - 8.f, pos.y + 2.f};
            dl->AddRectFilled(sp, {sp.x+sw, sp.y+sw}, colU32, 5.f);
            dl->AddRect(sp, {sp.x+sw, sp.y+sw}, C_Border(), 5.f, 0, 1.f);
        }
        ImGui::SetCursorScreenPos({pos.x + avail - 70.f, pos.y + 2.f});
        ImGui::ColorEdit4("##ce_edit", &color->x,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
            ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview);
        ImGui::Dummy({0, 6.f});
        ImGui::PopID();
    }

    static void tab_dashboard(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 195.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("Information"), "Info");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_InfoRow(oxorany("Client"), oxorany("NEXORA"), C_Accent());
        NS_InfoRow(oxorany("Version"), oxorany("1.0"));
        NS_InfoRow(oxorany("Dev"), oxorany("@Zelewin"));
        char libBuf[64];
        snprintf(libBuf, sizeof(libBuf), "0x%lx", proc::lib);
        NS_InfoRow(oxorany("Library"), libBuf);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 110.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Live Stats"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        char fpsT[16]; snprintf(fpsT, sizeof(fpsT), "%.0f", ImGui::GetIO().Framerate);
        ImU32 fc = ImGui::GetIO().Framerate > 55 ? C_Success()
                 : (ImGui::GetIO().Framerate > 30 ? IM_COL32(255,180,40,(int)(content_alpha*255)) : C_Danger());
        dl->AddText(ImGui::GetFont(), 30.f, {cp2.x+20, cp2.y+46}, fc, fpsT);
        float fpsTW = NS_TextSize(fpsT, 30.f).x;
        dl->AddText(ImGui::GetFont(), 13.f,
            {cp2.x+24+fpsTW, cp2.y+58},
            C_TextDim(), "FPS");

        char scr[32];
        snprintf(scr, sizeof(scr), "%.0f x %.0f", g_sw, g_sh);
        float sx = cp2.x + cw*0.5f;
        dl->AddText(ImGui::GetFont(), 12.f, {sx, cp2.y+48}, C_TextDim(), "SCREEN");
        dl->AddText(ImGui::GetFont(), 15.f, {sx, cp2.y+66}, C_TextMain(), scr);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0,0});

        float card3H = 75.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Match Status"));
        bool inM = data::in_match();
        ImU32 mCol = inM ? C_Success() : C_TextDim();
        dl->AddText(ImGui::GetFont(), 15.f,
            {cp3.x+16, cp3.y+44}, mCol,
            inM ? "In Match" : "Not in Match");

        char pbuf[32];
        snprintf(pbuf, sizeof(pbuf), "%zu players", data::player_count());
        float pw = NS_TextSize(pbuf, 14.f).x;
        dl->AddText(ImGui::GetFont(), 14.f,
            {cp3.x+cw-pw-16, cp3.y+44}, C_TextSec(), pbuf);

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+12});
        ImGui::Dummy({0, 40});
    }

    static void tab_esp(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float colW = (cw - 32 - 24) * 0.5f;
        float card1H = 280.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("ESP"), "Master");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        ImGui::PushID("esp_cols");
        ImGui::Columns(2, "esp_cols_id", false);
        ImGui::SetColumnWidth(0, colW + 12);
        ImGui::SetColumnWidth(1, colW + 12);

        NS_Toggle("esp_box",     oxorany("Box"),       &cfg::esp::box);
        NS_Toggle("esp_name",    oxorany("Name"),      &cfg::esp::name);
        NS_Toggle("esp_health",  oxorany("Health"),    &cfg::esp::health);
        NS_Toggle("esp_armor",   oxorany("Armor"),     &cfg::esp::armor);
        NS_Toggle("esp_dist",    oxorany("Distance"),  &cfg::esp::distance);
        NS_Toggle("esp_bot",     oxorany("Bot Tag"),   &cfg::esp::bot_tag);

        ImGui::NextColumn();

        NS_Toggle("esp_tracer",  oxorany("Tracer"),    &cfg::esp::tracer);
        NS_Toggle("esp_skel",    oxorany("Skeleton"),  &cfg::esp::skeleton);
        NS_Toggle("esp_cross",   oxorany("Crosshair"), &cfg::esp::crosshair);
        NS_Toggle("esp_weapon",  oxorany("Weapon"),    &cfg::esp::weapon);
        NS_Toggle("esp_fire",    oxorany("Firing"),    &cfg::esp::firing_ind);
        NS_Toggle("esp_loot",    oxorany("Loot"),      &cfg::esp::loot);

        ImGui::Columns(1);
        ImGui::PopID();

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float cardAimH = 265.f;
        ImVec2 cpAim = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cpAim, cw, cardAimH);
        NS_CardTitle(dl, {cpAim.x+16, cpAim.y+14}, oxorany("Aim & FOV Circle"), "2D Aim");
        ImGui::SetCursorScreenPos({cpAim.x+16, cpAim.y+44});

        NS_Toggle("aim_fov",     oxorany("Circle Aim (FOV)"), &cfg::aim::draw_fov);
        NS_Slider("fov_radius",  oxorany("Circle Radius"),    &cfg::aim::fov, 30.f, 350.f, "px");
        NS_Toggle("aim_silent",  oxorany("Aim Silent"),       &cfg::aim::silent);
        NS_Toggle("aim_nobita",  oxorany("Nobita Mode"),      &cfg::aim::nobita);
        NS_Slider("aim_dist",    oxorany("Aim Distance"),     &cfg::aim::distance, 30.f, 400.f, "m");

        cfg::esp::enabled = cfg::esp::box || cfg::esp::name || cfg::esp::health || cfg::esp::armor ||
                            cfg::esp::distance || cfg::esp::bot_tag || cfg::esp::tracer || cfg::esp::skeleton ||
                            cfg::esp::crosshair || cfg::esp::weapon || cfg::esp::firing_ind || cfg::esp::loot;
        cfg::aim::enabled = cfg::aim::silent || cfg::aim::nobita || cfg::aim::draw_fov;

        // Status
        {
            bool active = cfg::aim::enabled;
            char st[64];
            snprintf(st, sizeof(st), "%s", active ? "ACTIVE" : "INACTIVE");
            ImU32 stCol = active ? C_Success() : C_TextDim();
            float sw = NS_TextSize(st, 13.5f).x;
            dl->AddText(ImGui::GetFont(), 13.5f,
                {cpAim.x + cw - sw - 16, cpAim.y + 14}, stCol, st);
        }

        ImGui::SetCursorScreenPos({cpAim.x, cpAim.y+cardAimH+12});
        ImGui::Dummy({0,0});

        float card2H = 245.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Box Settings"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        const char* box_types[] = { oxorany("Corner"), oxorany("Full"), oxorany("Glow") };
        NS_ComboRow("box_type", oxorany("Box Type"), &cfg::esp::box_type, box_types, 3);
        NS_Slider("box_th", oxorany("Thickness"),  &cfg::esp::box_thickness, 0.5f, 5.0f, "");
        NS_Slider("box_rd", oxorany("Rounding"),   &cfg::esp::box_rounding,  0.f,  10.f, "");
        NS_Slider("box_rg", oxorany("Max Range"),  &cfg::esp::max_range,     50.f, 500.f, "m");

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0,0});

        float card3H = 115.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Loot Settings"));
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+44});

        NS_Slider("loot_rg", oxorany("Loot Range"), &cfg::esp::loot_range, 30.f, 300.f, "m");

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+12});
        ImGui::Dummy({0,0});

        float card4H = 195.f;
        ImVec2 cp4 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp4, cw, card4H);
        NS_CardTitle(dl, {cp4.x+16, cp4.y+14}, oxorany("Colors"));
        ImGui::SetCursorScreenPos({cp4.x+16, cp4.y+44});
        NS_ColorEdit("col_box",  oxorany("Box"),       &cfg::esp::box_col);
        NS_ColorEdit("col_vis",  oxorany("Visible"),   &cfg::esp::box_visible);
        NS_ColorEdit("col_name", oxorany("Name"),      &cfg::esp::name_col);
        NS_ColorEdit("col_dist", oxorany("Distance"),  &cfg::esp::distance_col);

        ImGui::SetCursorScreenPos({cp4.x, cp4.y+card4H+12});
        ImGui::Dummy({0, 40});
    }

    static void tab_settings(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 125.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("Display"));
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f x %.0f", g_sw, g_sh);
        NS_InfoRow(oxorany("Screen"), buf);
        snprintf(buf, sizeof(buf), "%.1f", ImGui::GetIO().Framerate);
        NS_InfoRow(oxorany("FPS"), buf);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 125.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("System"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        snprintf(buf, sizeof(buf), "0x%lx", proc::lib);
        NS_InfoRow(oxorany("Library"), buf);
        snprintf(buf, sizeof(buf), "%d", proc::pid);
        NS_InfoRow(oxorany("PID"), buf);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0,0});

        float card3H = 105.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Actions"));
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+44});

        if (NS_Button("unload", oxorany("Unload"), {cw-32, 36}, true)) {
            exit(0);
        }

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+12});
        ImGui::Dummy({0, 40});
    }

    void render() {
        func::esp::draw();
        bar::render();

        float dt = ImGui::GetIO().DeltaTime;
        ma = ma + ((bar::g_open ? 1.f : 0.f) - ma) * ImClamp(12.f*dt, 0.f, 1.f);

        if (ma > 0.01f) {
            int da = (int)(200*ma*bar::game_alpha());
            ImGui::GetBackgroundDrawList()->AddRectFilled({0,0},{g_sw,g_sh},IM_COL32(0,0,0,da));
        }
        if (ma < 0.01f) return;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 20.f);
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_Border, {0,0,0,0});

        ImVec2 wsz(980.f, 630.f);
        if (wsz.x > g_sw && g_sw > 0.f) wsz.x = g_sw * 0.95f;
        if (wsz.y > g_sh && g_sh > 0.f) wsz.y = g_sh * 0.95f;
        if (g_menu_pos.x < 0.f || (g_sw > 0.f && g_menu_pos.x > g_sw)) {
            g_menu_pos = {(g_sw - wsz.x) * 0.5f, (g_sh - wsz.y) * 0.5f};
        }
        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        ImGui::SetNextWindowPos(g_menu_pos, ImGuiCond_Always);

        ImGui::Begin("##m", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImVec2 mp = ImGui::GetMousePos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImDrawList* bg = ImGui::GetBackgroundDrawList();

        bg->AddRectFilled({wp.x-6, wp.y-6}, {wp.x+ws.x+6, wp.y+ws.y+6},
                          IM_COL32(0,0,0,(int)(35*ma)), R+5.f);
        bg->AddRectFilled({wp.x-12, wp.y-12}, {wp.x+ws.x+12, wp.y+ws.y+12},
                          IM_COL32(0,0,0,(int)(15*ma)), R+10.f);

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::bg, ma), R);
        bg->AddRect(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::border, ma), R);

        const float headerH = 56.f;
        const float sidebarW = 145.f;
        const float footerH = 50.f;

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+headerH},
                          col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersTop);
        bg->AddLine({wp.x, wp.y+headerH}, {wp.x+ws.x, wp.y+headerH},
                    col(UITheme::border, ma), 1.f);

        float logoSize = 34.f;
        float logoX = wp.x + 18.f;
        float logoY = wp.y + (headerH - logoSize) * 0.5f;
        bg->AddRectFilled({logoX, logoY}, {logoX+logoSize, logoY+logoSize},
                          col(UITheme::accent, ma), 7.f);
        float nW = NS_TextSize("N", 18.f).x;
        bg->AddText(ImGui::GetFont(), 18.f,
                    {logoX + (logoSize-nW)*0.5f, logoY + (logoSize-18.f)*0.5f},
                    IM_COL32(255,255,255,255), "N");

        dl->AddText(ImGui::GetFont(), 16.f, {logoX + logoSize + 12.f, wp.y + 12.f},
                    col(UITheme::text, ma), "NEXORA");
        dl->AddText(ImGui::GetFont(), 11.f, {logoX + logoSize + 12.f, wp.y + 32.f},
                    col(UITheme::text_dim, ma), "FREE FIRE");

        bg->AddLine({logoX + logoSize + 105.f, wp.y+16.f},
                    {logoX + logoSize + 105.f, wp.y+40.f},
                    col(UITheme::border, ma), 1.f);

        const char* tabTitles[] = {"Dashboard", "ESP", "Settings"};
        bg->AddText(ImGui::GetFont(), 13.f,
            {logoX + logoSize + 120.f, wp.y + 22.f},
            col(UITheme::text_dim, ma), "/");
        bg->AddText(ImGui::GetFont(), 15.f,
            {logoX + logoSize + 134.f, wp.y + 20.f},
            col(UITheme::text, ma), tabTitles[tab]);

        float closeW = 32.f;
        float closeX = wp.x + ws.x - closeW - 14.f;
        float closeY = wp.y + (headerH - closeW) * 0.5f;
        bool hoverClose = ImGui::IsMouseHoveringRect({closeX, closeY},
                                                      {closeX+closeW, closeY+closeW});
        if (hoverClose) bg->AddRectFilled({closeX, closeY},
                                          {closeX+closeW, closeY+closeW},
                                          col(UITheme::danger, ma), 6.f);
        float cX = closeX + closeW * 0.5f, cY = closeY + closeW * 0.5f, cr = 6.f;
        bg->AddLine({cX-cr,cY-cr},{cX+cr,cY+cr},
            hoverClose ? IM_COL32(255,255,255,255) : col(UITheme::text_dim, ma), 2.0f);
        bg->AddLine({cX-cr,cY+cr},{cX+cr,cY-cr},
            hoverClose ? IM_COL32(255,255,255,255) : col(UITheme::text_dim, ma), 2.0f);

        ImGui::SetCursorScreenPos({closeX, closeY});
        ImGui::PushID(9999);
        if (ImGui::InvisibleButton("hide", {closeW, closeW})) bar::g_open = false;
        ImGui::PopID();

        float sbY = wp.y + headerH;
        float sbBot = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, sbY}, {wp.x+sidebarW, sbBot},
                          col(UITheme::sidebar, ma));
        bg->AddLine({wp.x+sidebarW, sbY}, {wp.x+sidebarW, sbBot},
                    col(UITheme::border, ma), 1.f);

        float navStartY = sbY + 14.f;
        float navItemH = 44.f;
        for (int i = 0; i < 3; i++) {
            float ny = navStartY + i * (navItemH + 4);
            bool act = (tab == i);
            bool hov = ImGui::IsMouseHoveringRect({wp.x, ny},
                                                   {wp.x+sidebarW, ny+navItemH});
            if (act) {
                bg->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+navItemH},
                                  col(UITheme::accent, ma * 0.25f));
                bg->AddRectFilled({wp.x, ny}, {wp.x+3.5f, ny+navItemH},
                                  col(UITheme::accent, ma));
            } else if (hov) {
                bg->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+navItemH},
                                  col(UITheme::widget, ma));
            }
            char numBuf[4]; snprintf(numBuf, sizeof(numBuf), "%02d", i+1);
            dl->AddText(ImGui::GetFont(), 12.f,
                {wp.x+14, ny + (navItemH - 12.f)*0.5f}, act ? col(UITheme::accent, ma) : col(UITheme::text_dim, ma), numBuf);
            ImU32 txtCol = act ? col(UITheme::text, ma) :
                          (hov ? col(UITheme::text, ma) : col(UITheme::text_dim, ma));
            dl->AddText(ImGui::GetFont(), 15.f,
                {wp.x+38, ny + (navItemH - 15.f)*0.5f}, txtCol, tabTitles[i]);
            ImGui::SetCursorScreenPos({wp.x, ny});
            ImGui::PushID(i+500);
            if (ImGui::InvisibleButton("nav", {sidebarW, navItemH})) tab = i;
            ImGui::PopID();
        }

        float sFootY = sbBot - 48.f;
        bg->AddLine({wp.x+12, sFootY}, {wp.x+sidebarW-12, sFootY},
                    col(UITheme::border, ma), 1.f);
        dl->AddText(ImGui::GetFont(), 12.f, {wp.x+14, sFootY+10},
                    col(UITheme::text_dim, ma), "NEXORA v1.0");
        dl->AddText(ImGui::GetFont(), 11.f, {wp.x+14, sFootY+26},
                    col(UITheme::accent, ma), "@Zelewin");

        float contentX = wp.x + sidebarW + 20.f;
        float contentY = wp.y + headerH + 18.f;
        float contentW = ws.x - sidebarW - 40.f;
        float contentH = ws.y - headerH - footerH - 42.f;

        ImGui::SetCursorScreenPos({contentX, contentY});

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,           IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,         col(UITheme::accent, ma * 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,  col(UITheme::accent_light, ma));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,   col(UITheme::accent_light, ma));

        ImGui::BeginChild("content", {contentW, contentH}, false,
                          ImGuiWindowFlags_NoBackground);

        dl = ImGui::GetWindowDrawList();
        content_w = ImGui::GetContentRegionAvail().x;
        content_alpha = ma;

        switch (tab) {
            case 0: tab_dashboard(ma); break;
            case 1: tab_esp(ma); break;
            case 2: tab_settings(ma); break;
        }

        ImGui::EndChild();

        ImGui::PopStyleColor(4);

        float fy = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, fy}, {wp.x+ws.x, wp.y+ws.y},
                          col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersBottom);
        bg->AddLine({wp.x, fy}, {wp.x+ws.x, fy}, col(UITheme::border, ma), 1.f);

        float fbX = wp.x + 18.f;
        float fbY = fy + (footerH - 34.f) * 0.5f;
        ImGui::SetCursorScreenPos({fbX, fbY});
        if (NS_Button("f_save", "Save", {80, 34}, true)) {
            storage::save_config();
        }
        ImGui::SameLine(0, 10);
        if (NS_Button("f_load", "Load", {80, 34}, false)) {
            storage::load_config();
        }
        ImGui::SameLine(0, 10);
        if (NS_Button("f_reset", "Reset", {80, 34}, false)) {
            cfg::esp::enabled = false;
            cfg::esp::box = false;
            cfg::esp::name = false;
            cfg::esp::health = false;
            cfg::esp::armor = false;
            cfg::esp::distance = false;
            cfg::esp::tracer = false;
            cfg::esp::skeleton = false;
            cfg::esp::loot = false;
            cfg::aim::enabled = false;
            cfg::aim::silent = false;
            cfg::aim::nobita = false;
            cfg::aim::draw_fov = false;
            storage::save_config();
        }

        const char* fh = "@Zelewin";
        float fhw = NS_TextSize(fh, 12.f).x;
        dl->AddText(ImGui::GetFont(), 12.f,
            {wp.x + ws.x - fhw - 90, fy + (footerH - 12.f) * 0.5f}, col(UITheme::accent, ma), fh);
        const char* fv = "V1.0";
        float fvw = NS_TextSize(fv, 12.f).x;
        dl->AddText(ImGui::GetFont(), 12.f,
            {wp.x + ws.x - fvw - 18, fy + (footerH - 12.f) * 0.5f}, col(UITheme::text_dim, ma), fv);

        if ((mp.x>=wp.x && mp.x<=wp.x+ws.x && mp.y>=wp.y && mp.y<=wp.y+headerH)) {
            if (ImGui::IsMouseClicked(0)) {
                drag = true;
                doff = {mp.x-wp.x, mp.y-wp.y};
            }
        }
        if (drag) {
            if (ImGui::IsMouseDown(0)) {
                ImVec2 np{mp.x-doff.x, mp.y-doff.y};
                np.x = ImClamp(np.x, 0.f, g_sw-ws.x);
                np.y = ImClamp(np.y, 0.f, g_sh-ws.y);
                g_menu_pos = np;
            } else {
                drag = false;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(12);
    }
}