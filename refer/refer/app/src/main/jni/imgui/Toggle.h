#pragma once
#include "imgui_internal.h"

static std::map<ImGuiID, float> s_toggle_anim_state;


bool Toggle(const char* label, bool* state) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    
    // --- SIZE THODA BADA KIYA ---
    const float toggle_height = label_size.y * 1.4f; // Height thodi badhai (1.1f -> 1.4f)
    const float toggle_width = toggle_height * 2.3f; // Proportional width
    const float radius = toggle_height * 0.5f;
    const float vertical_margin = 4.0f; // Spacing increase ki
    
    const float total_widget_height = toggle_height + vertical_margin * 2.0f;
    const float total_width = ImGui::GetContentRegionAvail().x;
    const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + total_width, window->DC.CursorPos.y + total_widget_height));

    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) {
        return false;
    }

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    if (pressed) {
        *state = !*state;
        ImGui::MarkItemEdited(id);
    }

    // Animation logic
    float target_state = *state ? 1.0f : 0.0f;
    if (s_toggle_anim_state.find(id) == s_toggle_anim_state.end()) {
        s_toggle_anim_state[id] = target_state;
    }
    // Animation Speed (12.0f rakha hai smooth transition ke liye)
    s_toggle_anim_state[id] = ImLerp(s_toggle_anim_state[id], target_state, g.IO.DeltaTime * 12.0f);
    float current_anim_state = s_toggle_anim_state[id];

    // --- PREMIUM COLORS (Red Theme) ---
    const ImU32 bg_color_off = IM_COL32(40, 40, 42, 255);      // Dark Gray/Blackish
    const ImU32 bg_color_on  = IM_COL32(191, 0, 0, 255);       // Elegant Red (Theme Match)
    const ImU32 knob_color   = IM_COL32(255, 255, 255, 255);   // Pure White Knob

    float centered_y = bb.Min.y + vertical_margin;

    // Text Position
    float text_padding = 15.0f;
    ImVec2 text_pos(bb.Min.x + toggle_width + text_padding, centered_y + (toggle_height - label_size.y) * 0.5f);
    window->DrawList->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), label);

    // Toggle Background
    ImVec2 toggle_pos(bb.Min.x, centered_y);
    ImU32 bg_color = ImGui::ColorConvertFloat4ToU32(ImLerp(ImColor(bg_color_off).Value, ImColor(bg_color_on).Value, current_anim_state));
    
    // Background Draw
    window->DrawList->AddRectFilled(toggle_pos, ImVec2(toggle_pos.x + toggle_width, toggle_pos.y + toggle_height), bg_color, radius);

    // Knob Drawing
    float knob_radius = radius - 2.5f; // Knob size adjustment
    ImVec2 knob_center(toggle_pos.x + radius + current_anim_state * (toggle_width - 2.0f * radius), toggle_pos.y + radius);
    
    // Subtle Shadow for Knob
    window->DrawList->AddCircleFilled(knob_center, knob_radius + 1.0f, IM_COL32(0, 0, 0, 80)); 
    window->DrawList->AddCircleFilled(knob_center, knob_radius, knob_color);

    return pressed;
}

