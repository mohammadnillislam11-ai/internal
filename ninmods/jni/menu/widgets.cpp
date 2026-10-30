#define IMGUI_DEFINE_MATH_OPERATORS
#include "widgets.hpp"
#include "style.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Icon.h"
#include "aheasing.h"

#ifndef ICON_FA_CROWN
#define ICON_FA_CROWN "\xef\x94\xa1"
#endif

namespace ui::widgets {

bool Toggle(const char* label, bool* v, bool has_crown) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float height = 22.0f;
    const float width  = 42.0f;
    const float radius = height * 0.5f;

    ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    ImVec2 pos = window->DC.CursorPos;
    ImRect total_bb(pos, pos + ImVec2(ImGui::GetContentRegionAvail().x, height));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed) {
        *v = !(*v);
        ImGui::MarkItemEdited(id);
    }

    // Animation progress (0.0f = OFF, 1.0f = ON) with official AHEasing
    float animPos = *v ? 1.0f : 0.0f;
    float animEased = static_cast<float>(SineEaseInOut(static_cast<AHFloat>(animPos)));

    ImU32 bg_col = ImGui::GetColorU32(ImLerp(ImVec4(0.10f, 0.08f, 0.14f, 1.0f), ui::theme::PrimaryBg, animEased));
    ImU32 outline_col = ImGui::GetColorU32(ImLerp(ImVec4(0.20f, 0.15f, 0.28f, 1.0f), ui::theme::Primary, animEased));
    ImU32 circle_col = ImGui::GetColorU32(ImLerp(ui::theme::TextGray, ui::theme::Primary, animEased));
    ImU32 text_col = ImGui::GetColorU32(ImLerp(ui::theme::TextGray, ui::theme::TextWhite, animEased));

    ImDrawList* draw = window->DrawList;

    // Draw Capsule Switch
    ImVec2 switch_min = ImVec2(total_bb.Max.x - width - 8.0f, pos.y);
    ImVec2 switch_max = ImVec2(total_bb.Max.x - 8.0f, pos.y + height);
    draw->AddRectFilled(switch_min, switch_max, bg_col, radius);
    draw->AddRect(switch_min, switch_max, outline_col, radius, 0, 1.0f);

    float knob_x = switch_min.x + radius + (animEased * (width - height));
    draw->AddCircleFilled(ImVec2(knob_x, pos.y + radius), radius - 3.0f, circle_col, 16);

    // Label & Crown
    float text_y = pos.y + (height - label_size.y) * 0.5f;
    draw->AddText(ImVec2(pos.x + 4.0f, text_y), text_col, label);

    if (has_crown) {
        draw->AddText(ImVec2(pos.x + label_size.x + 10.0f, text_y - 1.0f),
                      ImColor(0.95f, 0.77f, 0.20f, 1.0f), ICON_FA_CROWN);
    }

    return pressed;
}

void Slider(const char* label, float* v, float v_min, float v_max, const char* fmt) {
    ImGui::PushID(label);
    ImGui::TextColored(ui::theme::TextWhite, "%s", label);
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 45.0f);
    ImGui::TextColored(ui::theme::Primary, fmt, *v);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ui::theme::FrameBg);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ui::theme::Primary);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ui::theme::PrimaryLight);
    ImGui::SliderFloat("##slider", v, v_min, v_max, "");
    ImGui::PopStyleColor(3);
    ImGui::PopID();
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
