#pragma once
#include "imgui.h"

/**
 * @file style.hpp
 * @brief Standard ImGui styling and color palette for ninmods client.
 */

namespace ui::theme {

// Core Color Palette (Light Pink Frosted Glass Theme - Borderless)
inline ImVec4 WindowBg         = ImVec4(0.14f, 0.06f, 0.12f, 0.88f); // Soft translucent pink glass
inline ImVec4 SidebarBg        = ImVec4(0.10f, 0.04f, 0.08f, 0.88f);
inline ImVec4 Primary          = ImVec4(1.00f, 0.28f, 0.60f, 1.00f); // #FF4799 Vibrant Pink
inline ImVec4 PrimaryBg        = ImVec4(0.85f, 0.20f, 0.50f, 0.80f); // Translucent pink button
inline ImVec4 PrimaryLight     = ImVec4(1.00f, 0.55f, 0.78f, 1.00f);
inline ImVec4 PrimaryDim       = ImVec4(0.45f, 0.12f, 0.28f, 0.35f);
inline ImVec4 TextWhite        = ImVec4(0.96f, 0.96f, 0.97f, 1.00f);
inline ImVec4 TextGray         = ImVec4(0.65f, 0.55f, 0.62f, 1.00f);
inline ImVec4 BorderColor      = ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // No border
inline ImVec4 FrameBg          = ImVec4(0.20f, 0.08f, 0.16f, 0.80f); // Glass input frame
inline ImVec4 FrameHover       = ImVec4(0.26f, 0.11f, 0.21f, 0.90f);
inline ImVec4 FrameActive      = ImVec4(0.32f, 0.14f, 0.26f, 0.95f);

inline void ApplyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 16.0f;
    style.ChildRounding     = 10.0f;
    style.FrameRounding     = 8.0f;
    style.PopupRounding     = 10.0f;
    style.WindowBorderSize  = 0.0f; // Completely borderless
    style.FrameBorderSize   = 0.0f;
    style.AntiAliasedLines   = true;
    style.AntiAliasedLinesUseTex = true;
    style.AntiAliasedFill    = true;
    style.CurveTessellationTol = 1.0f;

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
