#pragma once
#include "imgui.h"

/**
 * @file style.hpp
 * @brief Standard ImGui styling and color palette for ninmods client.
 */

namespace ui::theme {

// Core Color Palette (BOYHHA Premium Pink Theme Accent from refer/ZqwClient)
inline ImVec4 WindowBg         = ImVec4(0.07f, 0.05f, 0.08f, 0.98f);
inline ImVec4 SidebarBg        = ImVec4(0.04f, 0.03f, 0.05f, 1.00f);
inline ImVec4 Primary          = ImVec4(1.00f, 0.28f, 0.60f, 1.00f); // #FF4799 Vibrant Premium Pink
inline ImVec4 PrimaryBg        = ImVec4(0.35f, 0.08f, 0.20f, 0.85f); // Deep pink translucent pill bg
inline ImVec4 PrimaryLight     = ImVec4(1.00f, 0.55f, 0.78f, 1.00f); // Highlight pink
inline ImVec4 PrimaryDim       = ImVec4(0.45f, 0.12f, 0.28f, 0.35f);
inline ImVec4 TextWhite        = ImVec4(0.96f, 0.96f, 0.97f, 1.00f);
inline ImVec4 TextGray         = ImVec4(0.48f, 0.44f, 0.50f, 1.00f);
inline ImVec4 BorderColor      = ImVec4(0.22f, 0.14f, 0.22f, 1.00f);
inline ImVec4 FrameBg          = ImVec4(0.12f, 0.09f, 0.14f, 1.00f);
inline ImVec4 FrameHover       = ImVec4(0.16f, 0.12f, 0.18f, 1.00f);
inline ImVec4 FrameActive      = ImVec4(0.22f, 0.15f, 0.25f, 1.00f);

inline void ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 12.0f;
    style.ChildRounding     = 8.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 8.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 0.0f;

    style.Colors[ImGuiCol_WindowBg]         = WindowBg;
    style.Colors[ImGuiCol_ChildBg]          = SidebarBg;
    style.Colors[ImGuiCol_Border]           = BorderColor;
    style.Colors[ImGuiCol_Text]             = TextWhite;
    style.Colors[ImGuiCol_CheckMark]        = Primary;
    style.Colors[ImGuiCol_SliderGrab]       = Primary;
    style.Colors[ImGuiCol_SliderGrabActive] = PrimaryLight;
    style.Colors[ImGuiCol_FrameBg]          = FrameBg;
    style.Colors[ImGuiCol_FrameBgHovered]   = FrameHover;
    style.Colors[ImGuiCol_FrameBgActive]    = FrameActive;
    style.Colors[ImGuiCol_Separator]        = BorderColor;
    style.Colors[ImGuiCol_Button]           = ImVec4(0.18f, 0.12f, 0.26f, 0.5f);
    style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(0.25f, 0.16f, 0.36f, 0.8f);
    style.Colors[ImGuiCol_ButtonActive]     = ImVec4(0.32f, 0.20f, 0.46f, 1.0f);
}

} // namespace ui::theme
