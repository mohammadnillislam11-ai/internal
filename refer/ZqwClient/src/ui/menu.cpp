#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "cfg.hpp"
#include "styles/styles.h"
#include "widgets/widgets.hpp"
#include "Icon/Icon.h"
#include "game/AimSilent.h"
#include "Android_touch/Touch.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdio>
#include <string>

#ifndef ICON_FA_BARS
#define ICON_FA_BARS "\xef\x83\x89"
#endif

#ifndef ICON_FA_CROWN
#define ICON_FA_CROWN "\xef\x94\xa1"
#endif

/**
 * @file menu.cpp
 * @brief Complete BOYHHA ImGui UI ported directly to ZqwClient.
 * Fully styled in Premium Pink. Omit project name, project logo, and seller credit.
 */
namespace ui::menu {

namespace {
    static bool   itsmk        = true;
    static int    currentTab   = 0;
    static ImVec2 pos          = ImVec2(50.f, 150.f);
    static ImVec2 menuPos      = ImVec2(100.f, 80.f);
    static bool   menuPlaced   = false;

    // Feature preference state
    static float  s_aimFov     = 60.0f;
    static float  s_aimDist    = 250.0f;
    static bool   s_targetHead = true;
    static bool   s_prioClosest= true;
    static bool   s_grenade    = false;
    static bool   s_alert360   = false;
    static bool   s_speedHack  = false;
    static bool   s_fastSwitch = false;
}

/**
 * @brief Central UI render function executing BOYHHA Floating Button & Main Menu.
 */
void render() {
    SetBooyahTheme();

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    if (ds.x < 100.f || ds.y < 100.f) return;

    // Sync menu state with bar state
    itsmk = ui::bar::g_open;

    // ═════════════════════════════════════════════════════════════
    // 1. BOYHHA FLOATING BUTTON (if menu is minimized / !itsmk)
    // ═════════════════════════════════════════════════════════════
    if (!itsmk) {
        float size = 60.0f;

        // Keep within screen bounds
        if (pos.x < 10.f) pos.x = 10.f;
        if (pos.y < 10.f) pos.y = 10.f;
        if (pos.x > ds.x - size - 10.f) pos.x = ds.x - size - 10.f;
        if (pos.y > ds.y - size - 10.f) pos.y = ds.y - size - 10.f;

        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::Begin("##FloatingBtn", NULL,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));

        if (ImGui::Button("##OpenMenuBtn", ImVec2(size, size))) {
            itsmk = true;
            ui::bar::g_open = true;
        }

        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
            pos.x += ImGui::GetIO().MouseDelta.x;
            pos.y += ImGui::GetIO().MouseDelta.y;
        }

        ImGui::PopStyleColor(3);

        // Premium Pink circular floating icon (clean, procedural vector graphics)
        ImVec2 center(p.x + size * 0.5f, p.y + size * 0.5f);
        float radius = size * 0.5f;

        draw->AddCircleFilled(center, radius + 3.0f,
                              ImColor(BooyahPink.x, BooyahPink.y, BooyahPink.z, 0.20f), 32);
        draw->AddCircleFilled(center, radius,
                              ImColor(BooyahSidebar.x, BooyahSidebar.y, BooyahSidebar.z, 0.95f), 32);
        draw->AddCircle(center, radius, ImColor(BooyahPink), 32, 1.8f);

        ImFont* f = ImGui::GetFont();
        const char* glyph = ICON_FA_CROWN;
        float iconSize = f->FontSize * 1.3f;
        ImVec2 tsz = f->CalcTextSizeA(iconSize, FLT_MAX, 0.f, glyph);
        draw->AddText(f, iconSize,
                      ImVec2(center.x - tsz.x * 0.5f, center.y - tsz.y * 0.5f),
                      ImColor(BooyahPink), glyph);

        ImGui::End();
        return;
    }

    // ═════════════════════════════════════════════════════════════
    // 2. BOYHHA MAIN MENU PANEL (if itsmk)
    // ═════════════════════════════════════════════════════════════
    if (itsmk) {
        float panelW = 750.0f;
        float panelH = 420.0f;
        if (panelW > ds.x - 20.0f) panelW = ds.x - 20.0f;
        if (panelH > ds.y - 20.0f) panelH = ds.y - 20.0f;

        if (!menuPlaced) {
            menuPos = ImVec2((ds.x - panelW) * 0.5f, (ds.y - panelH) * 0.5f);
            menuPlaced = true;
        }

        // Clamp menu within display
        if (menuPos.x < 10.0f) menuPos.x = 10.0f;
        if (menuPos.y < 10.0f) menuPos.y = 10.0f;
        if (menuPos.x > ds.x - panelW - 10.0f) menuPos.x = ds.x - panelW - 10.0f;
        if (menuPos.y > ds.y - panelH - 10.0f) menuPos.y = ds.y - panelH - 10.0f;

        ImGui::SetNextWindowSize(ImVec2(panelW, panelH), ImGuiCond_Always);
        ImGui::SetNextWindowPos(menuPos, ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::Begin("BooyahPanel", &itsmk,
                         ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoScrollbar)) {

            ImDrawList* draw = ImGui::GetWindowDrawList();

            // ── Sidebar (Width 180) ──
            ImGui::BeginChild("Sidebar", ImVec2(180, 0), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::Spacing(); ImGui::Spacing();

            // Top Header (Neutral pink tile, no project logo/name)
            ImGui::SetCursorPosX(15);
            ImGui::BeginGroup();
            ImVec2 logoPos = ImGui::GetCursorScreenPos();
            draw->AddRectFilled(logoPos, ImVec2(logoPos.x + 35, logoPos.y + 35), ImColor(BooyahPink), 8.0f);
            draw->AddText(ImVec2(logoPos.x + 11, logoPos.y + 8), ImColor(0, 0, 0), ICON_FA_BARS);

            ImGui::SetCursorPos(ImVec2(60, ImGui::GetCursorPosY() + 2));
            ImGui::TextColored(BooyahWhite, "MENU");
            ImGui::SetCursorPos(ImVec2(60, ImGui::GetCursorPosY() - 4));
            ImGui::TextColored(BooyahGray, "64-bit client");
            ImGui::EndGroup();

            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing(); ImGui::Spacing();

            // 5 Tabs from BOYHHA
            const char* tabs[] = { " ESP", " ESP 2", " AIMBOT", " MISC", " INFO" };
            for (int i = 0; i < 5; i++) {
                ImGui::SetCursorPosX(10);
                bool isSelected = (currentTab == i);

                ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ImVec4(0.25f, 0.08f, 0.16f, 0.6f) : ImVec4(0, 0, 0, 0));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.08f, 0.16f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.10f, 0.22f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? BooyahWhite : BooyahGray);

                if (ImGui::Button(tabs[i], ImVec2(160, 35))) {
                    currentTab = i;
                }

                if (isSelected) {
                    ImVec2 pMin = ImGui::GetItemRectMin();
                    ImVec2 pMax = ImGui::GetItemRectMax();
                    draw->AddRectFilled(ImVec2(pMin.x, pMin.y + 5), ImVec2(pMin.x + 3, pMax.y - 5), ImColor(BooyahPink), 2.0f);
                }

                ImGui::PopStyleColor(4);
                ImGui::Spacing();
            }
            ImGui::EndChild();

            ImGui::SameLine(0, 0);

            // ── ContentArea ──
            ImGui::BeginChild("ContentArea", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

            ImGui::SetCursorPos(ImVec2(20, 16));
            ImGui::BeginGroup();

            // LIVE Pill
            ImVec2 livePos = ImGui::GetCursorScreenPos();
            draw->AddRectFilled(livePos, ImVec2(livePos.x + 60, livePos.y + 25), ImColor(0.25f, 0.08f, 0.16f), 12.0f);
            draw->AddRect(livePos, ImVec2(livePos.x + 60, livePos.y + 25), ImColor(BooyahPink), 12.0f, 0, 1.0f);
            draw->AddCircleFilled(ImVec2(livePos.x + 15, livePos.y + 12.5f), 4.0f, ImColor(BooyahPink));
            draw->AddText(ImVec2(livePos.x + 25, livePos.y + 5), ImColor(BooyahPink), "LIVE");

            // Header drag zone
            float headerDragW = panelW - 180.0f - 190.0f;
            if (headerDragW > 20.0f) {
                ImGui::SetCursorPos(ImVec2(85, 16));
                ImGui::InvisibleButton("##HeaderDrag", ImVec2(headerDragW, 25));
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                    menuPos.x += ImGui::GetIO().MouseDelta.x;
                    menuPos.y += ImGui::GetIO().MouseDelta.y;
                }
            }

            // Version Pill (Replacing seller credit)
            float vPillX = panelW - 180.0f - 105.0f;
            ImGui::SetCursorPos(ImVec2(vPillX, 16));
            ImVec2 vPos = ImGui::GetCursorScreenPos();
            draw->AddRectFilled(vPos, ImVec2(vPos.x + 62, vPos.y + 25), ImColor(0.25f, 0.08f, 0.16f), 12.0f);
            draw->AddText(ImVec2(vPos.x + 8, vPos.y + 5), ImColor(BooyahPink), "v90.107");

            // Close / Minimize Button
            float closeBtnX = panelW - 180.0f - 35.0f;
            ImGui::SetCursorPos(ImVec2(closeBtnX, 16));
            if (ImGui::Button("X", ImVec2(24, 25))) {
                itsmk = false;
                ui::bar::g_open = false;
            }

            ImGui::EndGroup();

            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing(); ImGui::Spacing();

            // ── Tab 0: PLAYER ESP ──
            if (currentTab == 0) {
                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahPink, "PLAYER ESP");
                ImGui::Spacing(); ImGui::Spacing();

                ImGui::Columns(2, "ESP_Columns", false);

                BooyahToggle("Enable ESP", &cfg::esp::enabled, false);
                ImGui::Spacing();
                BooyahToggle("Box ESP", &cfg::esp::box, false);
                ImGui::Spacing();
                BooyahToggle("Target ESP", &cfg::esp::firing_ind, true);

                ImGui::NextColumn();

                BooyahToggle("Line ESP", &cfg::esp::tracer, false);
                ImGui::Spacing();
                BooyahToggle("Skeleton ESP", &cfg::esp::skeleton, true);
                ImGui::Spacing();
                BooyahToggle("Distance ESP", &cfg::esp::distance, true);

                ImGui::Columns(1);
                ImGui::Spacing(); ImGui::Spacing();

                ImGui::SetCursorPosX(20);
                ImGui::BeginChild("SliderArea", ImVec2(ImGui::GetWindowWidth() - 40, 60), false);
                BooyahSlider("Fix Esp / Range", &cfg::esp::max_range, 50.0f, 600.0f, "%.0f m");
                ImGui::EndChild();
            }
            // ── Tab 1: EXTRA VISUALS (ESP 2) ──
            else if (currentTab == 1) {
                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahPink, "EXTRA VISUALS");
                ImGui::Spacing(); ImGui::Spacing();

                ImGui::Columns(2, "ESP2_Columns", false);

                BooyahToggle("Health ESP", &cfg::esp::health, true);
                ImGui::Spacing();
                BooyahToggle("Name ESP", &cfg::esp::name, false);
                ImGui::Spacing();
                BooyahToggle("Grenade ESP", &s_grenade, true);

                ImGui::NextColumn();

                BooyahToggle("Crosshair", &cfg::esp::crosshair, false);
                ImGui::Spacing();
                BooyahToggle("Bot Tag", &cfg::esp::bot_tag, false);
                ImGui::Spacing();
                BooyahToggle("360 Alert", &s_alert360, true);

                ImGui::Columns(1);
                ImGui::Spacing(); ImGui::Spacing();

                ImGui::SetCursorPosX(20);
                ImGui::BeginChild("SliderArea2", ImVec2(ImGui::GetWindowWidth() - 40, 100), false);
                BooyahSlider("Box Thickness", &cfg::esp::box_thickness, 0.5f, 3.0f, "%.1f px");
                BooyahSlider("Box Rounding", &cfg::esp::box_rounding, 0.0f, 8.0f, "%.0f px");
                ImGui::EndChild();
            }
            // ── Tab 2: AIMBOT PLAYER ──
            else if (currentTab == 2) {
                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahPink, "AIMBOT PLAYER");
                ImGui::Spacing(); ImGui::Spacing();

                ImGui::Columns(2, "AIM_Columns", false);

                BooyahToggle("Silent Aim", &AimSilentEnabled, false);
                ImGui::Spacing();
                BooyahToggle("Target Head", &s_targetHead, true);

                ImGui::NextColumn();

                BooyahToggle("Nobita Mode", &AimSilentNobita, true);
                ImGui::Spacing();
                BooyahToggle("Prioritize Closest", &s_prioClosest, false);

                ImGui::Columns(1);
                ImGui::Spacing();

                ImGui::SetCursorPosX(20);
                ImGui::BeginChild("SliderAimArea", ImVec2(ImGui::GetWindowWidth() - 40, 100), false);
                BooyahSlider("FOV", &s_aimFov, 0.0f, 360.0f, "%.0f");
                BooyahSlider("Distance", &s_aimDist, 0.0f, 500.0f, "%.0f m");
                ImGui::EndChild();
            }
            // ── Tab 3: MISC PLAYER ──
            else if (currentTab == 3) {
                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahPink, "MISC PLAYER");
                ImGui::Spacing(); ImGui::Spacing();

                ImGui::Columns(2, "MISC_Columns", false);
                BooyahToggle("Speed Hack", &s_speedHack, true);
                ImGui::Spacing();

                float btnW = ImGui::GetColumnWidth() - 20.0f;
                if (ImGui::Button("Flip Orientation", ImVec2(btnW, 36.0f))) {
                    touch::toggleOrientation();
                }

                ImGui::NextColumn();
                BooyahToggle("Fast Switch", &s_fastSwitch, false);
                ImGui::Spacing();

                if (ImGui::Button("Minimize Menu", ImVec2(btnW, 36.0f))) {
                    itsmk = false;
                    ui::bar::g_open = false;
                }
                ImGui::Columns(1);
            }
            // ── Tab 4: SYSTEM INFO ──
            else if (currentTab == 4) {
                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahPink, "SYSTEM INFO");
                ImGui::Spacing(); ImGui::Spacing();

                char fpsBuf[64];
                snprintf(fpsBuf, sizeof(fpsBuf), "Framerate: %.1f FPS", ImGui::GetIO().Framerate);
                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahWhite, "%s", fpsBuf);
                ImGui::Spacing();

                char resBuf[64];
                snprintf(resBuf, sizeof(resBuf), "Display: %.0f x %.0f", ds.x, ds.y);
                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahWhite, "%s", resBuf);
                ImGui::Spacing();

                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahPinkLight, "Architecture: Android Standalone (arm64-v8a)");
                ImGui::Spacing();

                ImGui::SetCursorPosX(20);
                ImGui::TextColored(BooyahGray, "Theme: Premium Pink (BOYHHA Adapted)");
            }

            // ── Footer ──
            ImVec2 bottomPos = ImVec2(ImGui::GetWindowPos().x + 180, ImGui::GetWindowPos().y + panelH - 40);
            draw->AddLine(ImVec2(bottomPos.x, bottomPos.y),
                          ImVec2(bottomPos.x + (panelW - 180), bottomPos.y),
                          ImColor(BooyahBorder));

            draw->AddCircleFilled(ImVec2(bottomPos.x + 25, bottomPos.y + 20), 4.0f, ImColor(BooyahPink));
            draw->AddText(ImVec2(bottomPos.x + 35, bottomPos.y + 13), ImColor(BooyahGray), "External");

            draw->AddCircleFilled(ImVec2(bottomPos.x + 120, bottomPos.y + 20), 4.0f, ImColor(0.9f, 0.6f, 0.1f));
            draw->AddText(ImVec2(bottomPos.x + 130, bottomPos.y + 13), ImColor(BooyahGray), "BYPASS ACTIVE");

            draw->AddText(ImVec2(ImGui::GetWindowPos().x + panelW - 70, bottomPos.y + 13),
                          ImColor(BooyahPink), "ACTIVE");

            ImGui::EndChild();
            ImGui::End();
        }
        ImGui::PopStyleVar();
    }
}

} // namespace ui::menu
