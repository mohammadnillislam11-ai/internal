#pragma once
#include "imgui.h"

/**
 * @file styles.h
 * @brief Premium Pink Theme Palette and Styling Engine adapted from BOYHHA.
 */

inline float g_sw = 0.f;
inline float g_sh = 0.f;

inline ImVec4 BooyahBg          = ImVec4(0.07f, 0.05f, 0.08f, 1.00f);
inline ImVec4 BooyahSidebar     = ImVec4(0.04f, 0.03f, 0.05f, 1.00f);
inline ImVec4 BooyahPink        = ImVec4(1.00f, 0.28f, 0.60f, 1.00f); // #FF4799 Vibrant Premium Pink
inline ImVec4 BooyahPinkBg      = ImVec4(0.35f, 0.08f, 0.20f, 1.00f); // Deep pink translucent pill bg
inline ImVec4 BooyahPinkLight   = ImVec4(1.00f, 0.55f, 0.78f, 1.00f); // Highlight pink
inline ImVec4 BooyahWhite       = ImVec4(0.96f, 0.96f, 0.97f, 1.00f);
inline ImVec4 BooyahGray        = ImVec4(0.45f, 0.42f, 0.47f, 1.00f);
inline ImVec4 BooyahBorder      = ImVec4(0.22f, 0.14f, 0.22f, 1.00f);
inline ImVec4 BooyahFrameBg     = ImVec4(0.12f, 0.09f, 0.14f, 1.00f);
inline ImVec4 BooyahFrameHover  = ImVec4(0.16f, 0.12f, 0.18f, 1.00f);
inline ImVec4 BooyahFrameActive = ImVec4(0.22f, 0.15f, 0.25f, 1.00f);

/**
 * @brief Applies BOYHHA style and geometry rules configured with Premium Pink theme.
 */
inline void SetBooyahTheme() {
    ImGuiStyle* style = &ImGui::GetStyle();
    style->WindowRounding = 12.0f;
    style->ChildRounding  = 8.0f;
    style->FrameRounding  = 6.0f;
    style->WindowBorderSize = 1.0f;    
    style->FrameBorderSize  = 0.0f;
    
    style->Colors[ImGuiCol_WindowBg]         = BooyahBg;
    style->Colors[ImGuiCol_ChildBg]          = BooyahSidebar;
    style->Colors[ImGuiCol_Border]           = BooyahBorder;
    style->Colors[ImGuiCol_Text]             = BooyahWhite;
    style->Colors[ImGuiCol_CheckMark]        = BooyahPink;
    style->Colors[ImGuiCol_SliderGrab]       = BooyahPink;
    style->Colors[ImGuiCol_SliderGrabActive] = BooyahPinkLight;
    style->Colors[ImGuiCol_FrameBg]          = BooyahFrameBg;
    style->Colors[ImGuiCol_FrameBgHovered]   = BooyahFrameHover;
    style->Colors[ImGuiCol_FrameBgActive]    = BooyahFrameActive;
    style->Colors[ImGuiCol_Separator]        = BooyahBorder;
}
