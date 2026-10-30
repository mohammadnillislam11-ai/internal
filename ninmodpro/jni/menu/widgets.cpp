#define IMGUI_DEFINE_MATH_OPERATORS
#include "widgets.hpp"
#include "style.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Icon.h"
#include <string>

#ifndef ICON_FA_CROWN
#define ICON_FA_CROWN "\xef\x94\xa1"
#endif

namespace ui::widgets {

bool Toggle(const char* label, bool* v, bool has_crown) {
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

    ImU32 bg_col = ImGui::GetColorU32(ImLerp(ImVec4(0.12f, 0.09f, 0.14f, 1.0f), ui::theme::PrimaryBg, animPos));
    ImU32 outline_col = ImGui::GetColorU32(ImLerp(ImVec4(0.20f, 0.15f, 0.22f, 1.0f), ui::theme::Primary, animPos));
    ImU32 circle_col = ImGui::GetColorU32(ImLerp(ui::theme::TextGray, ui::theme::Primary, animPos));
    ImU32 text_col = ImGui::GetColorU32(ImLerp(ui::theme::TextGray, ui::theme::TextWhite, animPos));

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

void Slider(const char* label, float* v, float v_min, float v_max, const char* fmt) {
    if (!v) return;

    ImGui::TextColored(ui::theme::TextWhite, "%s", label);
    ImGui::SameLine(ImGui::GetWindowWidth() - 75.0f);
    ImGui::TextColored(ui::theme::Primary, fmt, *v);

    ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ui::theme::FrameBg);
    ImGui::SliderFloat((std::string("##") + label).c_str(), v, v_min, v_max, "");
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::Spacing();
}

void Combo(const char* label, int* current_item, const char* const items[], int items_count) {
    ImGui::PushID(label);
    ImGui::TextColored(ui::theme::TextWhite, "%s", label);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ui::theme::FrameBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ui::theme::FrameHover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ui::theme::FrameActive);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ui::theme::SidebarBg);
    ImGui::PushStyleColor(ImGuiCol_Border, ui::theme::BorderColor);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    const char* preview = (items && *current_item >= 0 && *current_item < items_count) ? items[*current_item] : "Select...";
    if (ImGui::BeginCombo("##combo", preview)) {
        for (int i = 0; i < items_count; i++) {
            bool is_selected = (*current_item == i);
            if (ImGui::Selectable(items[i], is_selected)) {
                *current_item = i;
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopStyleColor(5);
    ImGui::PopID();
}

void ColorEdit(const char* label, ImVec4* col) {
    ImGui::PushID(label);
    ImGui::TextColored(ui::theme::TextWhite, "%s", label);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
    float c[4] = { col->x, col->y, col->z, col->w };
    if (ImGui::ColorEdit4("##coloredit", c, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar)) {
        col->x = c[0];
        col->y = c[1];
        col->z = c[2];
        col->w = c[3];
    }
    ImGui::PopID();
}

} // namespace ui::widgets
