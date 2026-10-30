#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>

inline float g_sw, g_sh;

namespace UITheme {
    inline ImVec4 bg           = ImVec4(22/255.f, 16/255.f, 36/255.f, 0.94f);
    inline ImVec4 bg_two       = ImVec4(28/255.f, 20/255.f, 44/255.f, 0.98f);
    inline ImVec4 sidebar      = ImVec4(26/255.f, 18/255.f, 42/255.f, 0.98f);
    inline ImVec4 panel        = ImVec4(36/255.f, 26/255.f, 56/255.f, 0.90f);
    inline ImVec4 widget       = ImVec4(46/255.f, 34/255.f, 72/255.f, 1.0f);

    inline ImVec4 accent       = ImVec4(180/255.f, 105/255.f, 255/255.f, 1.0f);
    inline ImVec4 accent_light = ImVec4(205/255.f, 145/255.f, 255/255.f, 1.0f);
    inline ImVec4 accent_dark  = ImVec4(140/255.f, 60/255.f, 220/255.f, 1.0f);

    inline ImVec4 text         = ImVec4(245/255.f, 245/255.f, 252/255.f, 1.0f);
    inline ImVec4 text_light   = ImVec4(255/255.f, 255/255.f, 255/255.f, 1.0f);
    inline ImVec4 text_dim     = ImVec4(175/255.f, 165/255.f, 195/255.f, 1.0f);

    inline ImVec4 success      = ImVec4(52/255.f, 211/255.f, 110/255.f, 1.0f);
    inline ImVec4 warning      = ImVec4(255/255.f, 180/255.f, 40/255.f, 1.0f);
    inline ImVec4 danger       = ImVec4(255/255.f, 69/255.f, 58/255.f, 1.0f);

    inline ImVec4 border       = ImVec4(115/255.f, 70/255.f, 175/255.f, 0.75f);
    inline ImVec4 border_light = ImVec4(180/255.f, 105/255.f, 255/255.f, 0.95f);
    inline ImVec4 border_dark  = ImVec4(16/255.f, 10/255.f, 26/255.f, 1.0f);
    inline ImVec4 subtab_bg    = ImVec4(26/255.f, 18/255.f, 42/255.f, 1.0f);

    inline ImVec4 ButtonColor   = ImVec4(160/255.f, 85/255.f, 240/255.f, 1.0f);
    inline ImVec4 ButtonHovered = ImVec4(185/255.f, 115/255.f, 255/255.f, 1.0f);
    inline ImVec4 ButtonActive  = ImVec4(130/255.f, 55/255.f, 210/255.f, 1.0f);

    inline void ButtonStyle() {
        ImGui::PushStyleColor(ImGuiCol_Button, ButtonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ButtonHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ButtonActive);
    }
    inline void PopButtonStyle() {
        ImGui::PopStyleColor(3);
    }
}

namespace style {
    inline std::unordered_map<std::string, float> anims;
    inline std::unordered_map<std::string, ImVec4> anim_colors;
    inline float content_w = 0.f;
    inline float content_alpha = 1.f;
    inline bool popup_open = false;
    inline std::string active_popup = "";
    inline constexpr float S = 2.5f;

    static float dt = 0.016f;
    static bool clicked = false;

    static inline float lrp(float a, float b, float t) { return a + (b - a) * t; }
    static inline ImVec4 lrp_col(const ImVec4& a, const ImVec4& b, float t) {
        return ImVec4(lrp(a.x, b.x, t), lrp(a.y, b.y, t), lrp(a.z, b.z, t), lrp(a.w, b.w, t));
    }

    inline void tick() {
        clicked = false;
        static float lt = 0.f;
        float ct = ImGui::GetTime();
        if (lt > 0.f) { dt = ct - lt; dt = ImClamp(dt, 0.001f, 0.1f); }
        lt = ct;
    }

    inline float anim(const std::string& id, float tgt, float spd = 12.f) {
        auto it = anims.find(id);
        if (it == anims.end()) { anims[id] = tgt; return tgt; }
        float t = ImClamp(spd * dt, 0.f, 1.f);
        it->second = lrp(it->second, tgt, t);
        if (fabsf(it->second - tgt) < 0.001f) it->second = tgt;
        return it->second;
    }

    inline ImVec4 anim_col(const std::string& id, const ImVec4& tgt, float spd = 12.f) {
        auto it = anim_colors.find(id);
        if (it == anim_colors.end()) { anim_colors[id] = tgt; return tgt; }
        float t = ImClamp(spd * dt, 0.f, 1.f);
        it->second = lrp_col(it->second, tgt, t);
        return it->second;
    }

    inline ImU32 col(const ImVec4& c, float a = 1.f) {
        return IM_COL32((int)(c.x * 255), (int)(c.y * 255), (int)(c.z * 255), (int)(c.w * 255 * a * content_alpha));
    }

    inline bool popup() { return popup_open || !active_popup.empty(); }
    inline void close_popup() { active_popup = ""; popup_open = false; }

    inline void popups() {
        if (active_popup.empty()) return;
        float pa = anim(active_popup + "_pa", 1.f, 18.f);
        if (pa < 0.01f) { active_popup = ""; return; }
        if (pa > 0.15f && ImGui::IsMouseClicked(0) && !clicked) { active_popup = ""; popup_open = false; }
    }
}
