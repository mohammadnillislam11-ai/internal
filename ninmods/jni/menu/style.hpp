#pragma once
#include "imgui.h"

/**
 * @file style.hpp
 * @brief Standard ImGui styling and color palette for ninmods client.
 */

namespace ui::theme {

// Core Color Palette (Theme Accent: Royal Neon Purple)
inline ImVec4 WindowBg         = ImVec4(0.06f, 0.05f, 0.08f, 0.98f);
inline ImVec4 SidebarBg        = ImVec4(0.04f, 0.03f, 0.06f, 1.00f);
inline ImVec4 Primary          = ImVec4(0.68f, 0.35f, 0.98f, 1.00f); // Royal Neon Purple Accent
inline ImVec4 PrimaryBg        = ImVec4(0.24f, 0.10f, 0.38f, 0.65f); // Deep Purple Card Accent
inline ImVec4 PrimaryLight     = ImVec4(0.80f, 0.50f, 1.00f, 1.00f); // Light Purple Hover
inline ImVec4 PrimaryDim       = ImVec4(0.40f, 0.18f, 0.60f, 0.35f);
inline ImVec4 TextWhite        = ImVec4(0.96f, 0.96f, 0.98f, 1.00f);
inline ImVec4 TextGray         = ImVec4(0.50f, 0.46f, 0.58f, 1.00f);
inline ImVec4 BorderColor      = ImVec4(0.20f, 0.15f, 0.28f, 1.00f);
inline ImVec4 FrameBg          = ImVec4(0.10f, 0.08f, 0.14f, 1.00f);
inline ImVec4 FrameHover       = ImVec4(0.15f, 0.11f, 0.20f, 1.00f);
inline ImVec4 FrameActive      = ImVec4(0.20f, 0.14f, 0.28f, 1.00f);

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
