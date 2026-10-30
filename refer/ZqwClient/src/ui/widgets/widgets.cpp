#define IMGUI_DEFINE_MATH_OPERATORS
#include "widgets.hpp"
#include "../Icon/Icon.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <string>

#ifndef ICON_FA_CROWN
#define ICON_FA_CROWN "\xef\x94\xa1"
#endif

/**
 * @brief Renders an animated capsule toggle button with outline, circle indicator, and optional crown icon.
 * @param label Button display name.
 * @param v Pointer to boolean state variable.
 * @param has_crown True to render gold crown icon on the right.
 * @return True if button was clicked this frame.
 */
bool BooyahToggle(const char* label, bool* v, bool has_crown) {
    if (!v) return false;

    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    float height = 36.0f; 
    float width = ImGui::GetColumnWidth() - 15.0f;
    if (width < 60.0f) width = 140.0f;
    float radius = height * 0.5f;

    ImGui::InvisibleButton(label, ImVec2(width, height));
    bool clicked = ImGui::IsItemClicked();
    if (clicked) *v = !*v;

    ImGuiID id = ImGui::GetID(label);
    float animPos = ImGui::GetStateStorage()->GetFloat(id, *v ? 1.0f : 0.0f);
    animPos = ImLerp(animPos, *v ? 1.0f : 0.0f, ImClamp(ImGui::GetIO().DeltaTime * 14.0f, 0.0f, 1.0f));
    ImGui::GetStateStorage()->SetFloat(id, animPos);

    ImU32 bg_col = ImGui::GetColorU32(ImLerp(ImVec4(0.12f, 0.09f, 0.14f, 1.0f), BooyahPinkBg, animPos));
    ImU32 outline_col = ImGui::GetColorU32(ImLerp(ImVec4(0.20f, 0.15f, 0.22f, 1.0f), BooyahPink, animPos));
    ImU32 circle_col = ImGui::GetColorU32(ImLerp(BooyahGray, BooyahPink, animPos));
    ImU32 text_col = ImGui::GetColorU32(ImLerp(BooyahGray, BooyahWhite, animPos));

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg_col, radius);
    draw_list->AddRect(p, ImVec2(p.x + width, p.y + height), outline_col, radius, 0, 1.5f);
    
    draw_list->AddCircleFilled(ImVec2(p.x + 20.0f, p.y + height / 2.0f), 8.0f, circle_col);
    
    draw_list->AddText(ImVec2(p.x + 40.0f, p.y + height / 2.0f - ImGui::GetTextLineHeight() / 2.0f), text_col, label);

    if (has_crown) {
        draw_list->AddText(ImVec2(p.x + width - 25.0f, p.y + height / 2.0f - ImGui::GetTextLineHeight() / 2.0f),
                           ImColor(255, 180, 0), ICON_FA_CROWN);
    }
    
    return clicked;
}

/**
 * @brief Renders a styled slider with title, live percentage/value readout, and pink grabber.
 * @param label Slider display name.
 * @param v Pointer to float value variable.
 * @param v_min Minimum range value.
 * @param v_max Maximum range value.
 * @param fmt Format string for value display (default: "%.2f").
 */
void BooyahSlider(const char* label, float* v, float v_min, float v_max, const char* fmt) {
    if (!v) return;

    ImGui::TextColored(BooyahWhite, "%s", label);
    ImGui::SameLine(ImGui::GetWindowWidth() - 75.0f);
    ImGui::TextColored(BooyahPink, fmt, *v);
    
    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, BooyahFrameBg);
    ImGui::SliderFloat((std::string("##") + label).c_str(), v, v_min, v_max, "");
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::Spacing();
}
