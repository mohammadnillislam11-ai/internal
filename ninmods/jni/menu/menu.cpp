#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "license_view.h"
#include "config.hpp"
#include "style.hpp"
#include "widgets.hpp"
#include "license_client.h"
#include "game_security.hpp"
#include "engine/engine.hpp"
#include "config_store.h"
#include "Icon.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_notify.h"
#include <cstdio>
#include <string>

#ifndef ICON_FA_BARS
#define ICON_FA_BARS "\xef\x83\x89"
#endif

#ifndef ICON_FA_CROWN
#define ICON_FA_CROWN "\xef\x94\xa1"
#endif

extern void request_exit(int code);

namespace ui::menu {

namespace {
    static int currentTab = 0;
}

void render() {
    ui::theme::ApplyTheme();

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    if (ds.x < 100.f || ds.y < 100.f) return;

    // ==================================================================
    // 0. FIRST-RUN LICENSE / KEY VERIFICATION MODAL
    // ==================================================================
    if (!ui::license::g_licensed) {
        ui::license::render();
        ImGui::RenderNotifications();
        return;
    }

    // ==================================================================
    // 1. FLOATING LAUNCHER BUTTON (Draggable & Slideable Floating Puck)
    // ==================================================================
    static float  s_puck_x = -1.0f;
    static float  s_puck_y = -1.0f;
    static bool   s_puck_drag = false;
    static bool   s_puck_potential = false;
    static ImVec2 s_puck_down(0, 0);
    static ImVec2 s_puck_offset(0, 0);
    static constexpr float PUCK_DRAG_THRESHOLD = 8.0f;

    if (!g_open) {
        const float puckW = 120.0f;
        const float puckH = 48.0f;

        if (s_puck_x < 0.0f || s_puck_y < 0.0f) {
            s_puck_x = 40.0f;
            s_puck_y = ds.y * 0.35f;
        }

        // Clamp to screen bounds
        if (ds.x > puckW) s_puck_x = ImClamp(s_puck_x, 10.0f, ds.x - puckW - 10.0f);
        if (ds.y > puckH) s_puck_y = ImClamp(s_puck_y, 10.0f, ds.y - puckH - 10.0f);

        ImVec2 mp = ImGui::GetMousePos();
        bool hov = (mp.x >= s_puck_x && mp.x <= s_puck_x + puckW &&
                    mp.y >= s_puck_y && mp.y <= s_puck_y + puckH);

        // Detect touch / click start
        if (hov && ImGui::IsMouseClicked(0)) {
            s_puck_potential = true;
            s_puck_down = mp;
            s_puck_offset = ImVec2(mp.x - s_puck_x, mp.y - s_puck_y);
        }

        // Handle smooth sliding / dragging
        if (s_puck_potential && ImGui::IsMouseDown(0)) {
            float dx = mp.x - s_puck_down.x;
            float dy = mp.y - s_puck_down.y;
            if (s_puck_drag || (dx * dx + dy * dy) > (PUCK_DRAG_THRESHOLD * PUCK_DRAG_THRESHOLD)) {
                s_puck_drag = true;
                s_puck_x = mp.x - s_puck_offset.x;
                s_puck_y = mp.y - s_puck_offset.y;
                if (ds.x > puckW) s_puck_x = ImClamp(s_puck_x, 10.0f, ds.x - puckW - 10.0f);
                if (ds.y > puckH) s_puck_y = ImClamp(s_puck_y, 10.0f, ds.y - puckH - 10.0f);
            }
        }

        // Touch release: Click without drag opens the main menu!
        if (ImGui::IsMouseReleased(0)) {
            if (s_puck_potential && !s_puck_drag) {
                g_open = true; // Opens Main Menu, automatically hiding floating logo!
            }
            s_puck_potential = false;
            s_puck_drag = false;
        }

        // Render Draggable Floating Logo & Live FPS Pill
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        float pillR = puckH * 0.5f;

        // Shadow & Background
        dl->AddRectFilled(ImVec2(s_puck_x + 2, s_puck_y + 3),
                          ImVec2(s_puck_x + puckW + 2, s_puck_y + puckH + 3),
                          IM_COL32(0, 0, 0, 70), pillR);

        dl->AddRectFilled(ImVec2(s_puck_x, s_puck_y),
                          ImVec2(s_puck_x + puckW, s_puck_y + puckH),
                          ImColor(ui::theme::SidebarBg), pillR);

        // Glowing border (shifts bright on drag)
        ImU32 borderCol = s_puck_drag ? ImColor(ui::theme::PrimaryLight) : ImColor(ui::theme::Primary);
        dl->AddRect(ImVec2(s_puck_x, s_puck_y),
                    ImVec2(s_puck_x + puckW, s_puck_y + puckH),
                    borderCol, pillR, 0, s_puck_drag ? 2.0f : 1.2f);

        // Neon Accent Strip
        dl->AddRectFilled(ImVec2(s_puck_x + 4.0f, s_puck_y + 5.0f),
                          ImVec2(s_puck_x + 7.0f, s_puck_y + puckH - 5.0f),
                          ImColor(ui::theme::Primary), 2.0f);

        // Circular Badge Letter "N"
        float circleR = 15.0f;
        ImVec2 circleCenter(s_puck_x + 26.0f, s_puck_y + puckH * 0.5f);
        dl->AddCircleFilled(circleCenter, circleR, ImColor(ui::theme::PrimaryBg));
        dl->AddCircle(circleCenter, circleR, ImColor(ui::theme::Primary), 32, 1.5f);
        dl->AddText(ImGui::GetFont(), 18.0f,
                    ImVec2(circleCenter.x - 6.0f, circleCenter.y - 11.0f),
                    ImColor(ui::theme::PrimaryLight), "N");

        // Live Framerate Text (Green >= 50, Yellow >= 30, Red < 30)
        float fps = ImGui::GetIO().Framerate;
        int fpsInt = static_cast<int>(fps + 0.5f);
        char fpsBuf[16];
        snprintf(fpsBuf, sizeof(fpsBuf), "%d FPS", fpsInt);

        ImU32 fpsCol = (fps >= 50.0f) ? IM_COL32(50, 230, 90, 255)
                     : (fps >= 30.0f) ? IM_COL32(255, 190, 40, 255)
                                      : IM_COL32(255, 70, 60, 255);

        dl->AddText(ImGui::GetFont(), 16.0f,
                    ImVec2(s_puck_x + 48.0f, s_puck_y + (puckH - 16.0f) * 0.5f),
                    fpsCol, fpsBuf);

        ImGui::RenderNotifications();
        return;
    }

    // ==================================================================
    // 2. MAIN NINMODS CLIENT PANEL
    // ==================================================================
    float panelW = ImMin(750.0f, ds.x - 40.0f);
    float panelH = ImMin(440.0f, ds.y - 40.0f);

    ImGui::SetNextWindowSize(ImVec2(panelW, panelH), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2((ds.x - panelW) * 0.5f, (ds.y - panelH) * 0.5f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar |
                                ImGuiWindowFlags_NoResize |
                                ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin("NinModsPanel", &g_open, winFlags)) {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // ─────────────────────────────────────────────────────────────
        // LEFT SIDEBAR (180px Wide)
        // ─────────────────────────────────────────────────────────────
        ImGui::BeginChild("Sidebar", ImVec2(180, 0), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::Spacing(); ImGui::Spacing();

        ImGui::SetCursorPosX(15);
        ImGui::BeginGroup();
        ImVec2 logoPos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(logoPos, ImVec2(logoPos.x + 35, logoPos.y + 35), ImColor(ui::theme::Primary), 8.0f);
        draw->AddText(ImVec2(logoPos.x + 10, logoPos.y + 7), ImColor(0, 0, 0), "N!");

        ImGui::SetCursorPos(ImVec2(60, ImGui::GetCursorPosY() + 2));
        ImGui::TextColored(ui::theme::TextWhite, "NINMODS");
        ImGui::SetCursorPos(ImVec2(60, ImGui::GetCursorPosY() - 4));
        char subTitle[64];
        snprintf(subTitle, sizeof(subTitle), "%s 64/32", engine::get_target_region());
        ImGui::TextColored(ui::theme::PrimaryLight, "%s", subTitle);
        ImGui::EndGroup();

        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing(); ImGui::Spacing();

        // Navigation Tabs (5 Tabs)
        const char* tabs[] = { " ESP", " ESP 2", " AIMBOT", " MISC", " INFO" };
        for (int i = 0; i < 5; i++) {
            ImGui::SetCursorPosX(10);
            bool isSelected = (currentTab == i);

            ImGui::PushStyleColor(ImGuiCol_Button, isSelected ? ui::theme::PrimaryBg : ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.12f, 0.35f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ui::theme::PrimaryBg);
            ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? ui::theme::TextWhite : ui::theme::TextGray);

            if (ImGui::Button(tabs[i], ImVec2(160, 36))) {
                currentTab = i;
            }

            // Vertical indicator line for selected tab
            if (isSelected) {
                ImVec2 pMin = ImGui::GetItemRectMin();
                ImVec2 pMax = ImGui::GetItemRectMax();
                draw->AddRectFilled(ImVec2(pMin.x, pMin.y + 5), ImVec2(pMin.x + 3, pMax.y - 5), ImColor(ui::theme::Primary), 2.0f);
            }

            ImGui::PopStyleColor(4);
            ImGui::Spacing();
        }
        ImGui::EndChild();

        ImGui::SameLine(0, 0);

        // ─────────────────────────────────────────────────────────────
        // RIGHT CONTENT AREA
        // ─────────────────────────────────────────────────────────────
        ImGui::BeginChild("ContentArea", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

        // Top Status Header (Capsules)
        ImGui::SetCursorPos(ImVec2(20, 16));
        ImGui::BeginGroup();

        // 1. LIVE Capsule
        ImVec2 livePos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(livePos, ImVec2(livePos.x + 60, livePos.y + 25), ImColor(ui::theme::PrimaryBg), 12.0f);
        draw->AddRect(livePos, ImVec2(livePos.x + 60, livePos.y + 25), ImColor(ui::theme::Primary), 12.0f, 0, 1.0f);
        draw->AddCircleFilled(ImVec2(livePos.x + 15, livePos.y + 12.5f), 4.0f, ImColor(ui::theme::PrimaryLight));
        draw->AddText(ImVec2(livePos.x + 25, livePos.y + 5), ImColor(ui::theme::PrimaryLight), "LIVE");

        // 2. Security Badge
        ImGui::SetCursorPosX(90);
        ImVec2 secPos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(secPos, ImVec2(secPos.x + 150, secPos.y + 25), ImColor(0.12f, 0.08f, 0.18f), 12.0f);
        draw->AddRect(secPos, ImVec2(secPos.x + 150, secPos.y + 25), ImColor(ui::theme::Primary), 12.0f, 0, 1.0f);
        draw->AddText(ImVec2(secPos.x + 15, secPos.y + 5), ImColor(ui::theme::PrimaryLight), "NINMODS SECURE");

        // 3. Version Capsule
        ImGui::SameLine(panelW - 250);
        ImVec2 vPos = ImGui::GetCursorScreenPos();
        draw->AddRectFilled(vPos, ImVec2(vPos.x + 45, vPos.y + 25), ImColor(ui::theme::PrimaryBg), 12.0f);
        draw->AddText(ImVec2(vPos.x + 10, vPos.y + 5), ImColor(ui::theme::TextWhite), "v2.0");

        // 4. Close [X] Button
        ImGui::SameLine(panelW - 195);
        if (ImGui::Button("X##CloseBtn", ImVec2(30, 25))) {
            g_open = false;
        }

        ImGui::EndGroup();

        ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        ImGui::SetCursorPosY(54);
        ImGui::Separator();
        ImGui::Spacing();

        // ─────────────────────────────────────────────────────────────
        // TAB CONTENTS (ALL OPTIONS FROM ZqwClient_old)
        // ─────────────────────────────────────────────────────────────
        ImGui::SetCursorPos(ImVec2(20, 68));
        ImGui::BeginChild("ScrollableTabContent", ImVec2(panelW - 200, panelH - 120), false);

        using namespace ui::widgets;

        if (currentTab == 0) {
            // ── TAB 0: ESP (Master Options) ──
            ImGui::TextColored(ui::theme::Primary, "PLAYER ESP");
            ImGui::Spacing();

            ImGui::Columns(2, "ESP_Columns_1", false);
            Toggle("Enable ESP",     &cfg::esp::enabled, false);
            ImGui::Spacing();
            Toggle("Box ESP",        &cfg::esp::box, false);
            ImGui::Spacing();
            Toggle("Name ESP",       &cfg::esp::name, false);
            ImGui::Spacing();
            Toggle("Health ESP",     &cfg::esp::health, false);
            ImGui::Spacing();
            Toggle("Armor ESP",      &cfg::esp::armor, false);
            ImGui::Spacing();
            Toggle("Distance ESP",   &cfg::esp::distance, false);

            ImGui::NextColumn();

            Toggle("Line / Tracer",  &cfg::esp::tracer, false);
            ImGui::Spacing();
            Toggle("Skeleton ESP",   &cfg::esp::skeleton, true);
            ImGui::Spacing();
            Toggle("Crosshair",      &cfg::esp::crosshair, false);
            ImGui::Spacing();
            Toggle("Weapon ESP",     &cfg::esp::weapon, false);
            ImGui::Spacing();
            Toggle("Firing Ind.",    &cfg::esp::firing_ind, false);
            ImGui::Spacing();
            Toggle("Bot Tag",        &cfg::esp::bot_tag, false);

            ImGui::Columns(1);
            ImGui::Spacing(); ImGui::Spacing();

            Toggle("Loot ESP", &cfg::esp::loot, false);
            ImGui::Spacing();
            Slider("Fix ESP Offset", &cfg::esp::fix_esp, 0.0f, 100.0f, "%.1f");
        }
        else if (currentTab == 1) {
            // ── TAB 1: ESP 2 (Visual Adjustments & Colors from ZqwClient_old) ──
            ImGui::TextColored(ui::theme::Primary, "BOX & VISUAL SETTINGS");
            ImGui::Spacing();

            const char* box_types[] = { "Corner", "Full 2D", "Glow Bracket" };
            Combo("Box Style", &cfg::esp::box_type, box_types, 3);

            Slider("Box Thickness",   &cfg::esp::box_thickness, 0.5f, 5.0f, "%.1f px");
            Slider("Box Rounding",    &cfg::esp::box_rounding,  0.0f, 10.0f, "%.1f");
            Slider("Max Range",       &cfg::esp::max_range,     50.0f, 500.0f, "%.0f m");
            Slider("Loot Range",      &cfg::esp::loot_range,    30.0f, 300.0f, "%.0f m");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ui::theme::Primary, "CUSTOM COLOR PALETTE");
            ImGui::Spacing();

            ColorEdit("Box Default",  &cfg::esp::box_col);
            ColorEdit("Box Visible",  &cfg::esp::box_visible);
            ColorEdit("Player Name",  &cfg::esp::name_col);
            ColorEdit("Distance Tag", &cfg::esp::distance_col);
            ColorEdit("Weapon Label", &cfg::esp::weapon_col);
        }
        else if (currentTab == 2) {
            // ── TAB 2: AIMBOT (Combat Options from ZqwClient_old) ──
            ImGui::TextColored(ui::theme::Primary, "AIMBOT & SILENT AIM");
            ImGui::Spacing();

            ImGui::Columns(2, "AIM_Columns", false);
            Toggle("Master Aimbot",  &cfg::aim::enabled, false);
            ImGui::Spacing();
            Toggle("Aim Silent",     &cfg::aim::silent, true);
            ImGui::Spacing();
            Toggle("Circle Aim (FOV)", &cfg::aim::draw_fov, false);

            ImGui::NextColumn();

            Toggle("Nobita Mode",    &cfg::aim::nobita, true);
            ImGui::Spacing();

            const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
            Combo("Target Bone", &cfg::aim::target_bone, bones, 4);

            const char* triggers[] = { "On Fire", "On Scope", "Always On" };
            Combo("Trigger", &cfg::aim::trigger_type, triggers, 3);

            ImGui::Columns(1);
            ImGui::Spacing(); ImGui::Spacing();

            Slider("Circle Aim Radius", &cfg::aim::fov, 20.0f, 500.0f, "%.0f px");
            Slider("Aim Distance",   &cfg::aim::distance, 10.0f, 500.0f, "%.0f m");
            Slider("Aim Smoothing",  &cfg::aim::smooth,   1.0f,  25.0f,  "%.1f");
        }
        else if (currentTab == 3) {
            // ── TAB 3: MISC (System & Utility from ZqwClient_old) ──
            ImGui::TextColored(ui::theme::Primary, "MISCELLANEOUS MODS");
            ImGui::Spacing();

            ImGui::Columns(2, "MISC_Columns", false);
            Toggle("Speed Hack",     &cfg::misc::speed_hack, true);
            ImGui::Spacing();
            Toggle("Fast Switch",    &cfg::misc::fast_switch, false);

            ImGui::NextColumn();

            Toggle("Grenade Warning", &cfg::misc::grenade_esp, false);
            ImGui::Spacing();
            Toggle("360 Alert",      &cfg::misc::alert_360, false);

            ImGui::Columns(1);
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(ui::theme::Primary, "ENGINE STATUS");
            ImGui::Spacing();

            char fpsBuf[32];
            snprintf(fpsBuf, sizeof(fpsBuf), "%.1f FPS", ImGui::GetIO().Framerate);
            ImGui::TextColored(ui::theme::TextWhite, "Framerate:");
            ImGui::SameLine(180);
            ImGui::TextColored(ui::theme::PrimaryLight, "%s", fpsBuf);

            char resBuf[64];
            snprintf(resBuf, sizeof(resBuf), "%.0f x %.0f", ds.x, ds.y);
            ImGui::TextColored(ui::theme::TextWhite, "Resolution:");
            ImGui::SameLine(180);
            ImGui::TextColored(ui::theme::PrimaryLight, "%s", resBuf);

            ImGui::TextColored(ui::theme::TextWhite, "Target Package:");
            ImGui::SameLine(180);
            char targetBuf[128];
            if (engine::get_target_pid() > 0) {
                snprintf(targetBuf, sizeof(targetBuf), "%s (%s - PID: %d)",
                         engine::get_target_package(), engine::get_target_region(), engine::get_target_pid());
            } else {
                snprintf(targetBuf, sizeof(targetBuf), "%s (%s)",
                         engine::get_target_package(), engine::get_target_region());
            }
            ImGui::TextColored(ui::theme::PrimaryLight, "%s", targetBuf);

            ImGui::TextColored(ui::theme::TextWhite, "Native Security:");
            ImGui::SameLine(180);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "ACTIVE (NativeShield)");
        }
        else if (currentTab == 4) {
            // ── TAB 4: INFO (License & Backend Information) ──
            ImGui::TextColored(ui::theme::Primary, "CLIENT & LICENSE INFORMATION");
            ImGui::Spacing();

            ImGui::TextColored(ui::theme::TextWhite, "Client Build:");
            ImGui::SameLine(180);
            ImGui::TextColored(ui::theme::PrimaryLight, "NINMODS Standalone 2.0");

            ImGui::TextColored(ui::theme::TextWhite, "License Status:");
            ImGui::SameLine(180);
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, 1.0f), "VERIFIED & ACTIVE");

            ImGui::TextColored(ui::theme::TextWhite, "Access Tier:");
            ImGui::SameLine(180);
            ImGui::TextColored(ui::theme::PrimaryLight, "VIP ACCESS");

            ImGui::TextColored(ui::theme::TextWhite, "Expiration:");
            ImGui::SameLine(180);
            char expDisplay[128];
            if (ui::license::g_days_left > 0) {
                snprintf(expDisplay, sizeof(expDisplay), "%s (%d days left)",
                         ui::license::g_expiration_date.c_str(), ui::license::g_days_left);
            } else if (!ui::license::g_expiration_date.empty()) {
                snprintf(expDisplay, sizeof(expDisplay), "%s", ui::license::g_expiration_date.c_str());
            } else {
                snprintf(expDisplay, sizeof(expDisplay), "ACTIVE");
            }
            ImGui::TextColored(ui::theme::PrimaryLight, "%s", expDisplay);

            std::string hwid = auth::get_device_hwid();
            std::string shortHwid = hwid.substr(0, 16) + "...";
            ImGui::TextColored(ui::theme::TextWhite, "Device HWID:");
            ImGui::SameLine(180);
            ImGui::TextColored(ui::theme::TextGray, "%s", shortHwid.c_str());

            ImGui::Spacing(); ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.15f, 0.15f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.20f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.25f, 0.25f, 1.0f));
            if (ImGui::Button("EXIT CLIENT (CODE 0)", ImVec2(220, 38))) {
                ::request_exit(0);
            }
            ImGui::PopStyleColor(3);
        }

        ImGui::EndChild();

        // ─────────────────────────────────────────────────────────────
        // FOOTER STATUS BAR
        // ─────────────────────────────────────────────────────────────
        ImVec2 bottomPos = ImVec2(ImGui::GetWindowPos().x + 180, ImGui::GetWindowPos().y + panelH - 36);
        draw->AddLine(ImVec2(bottomPos.x, bottomPos.y), ImVec2(bottomPos.x + (panelW - 180), bottomPos.y), ImColor(ui::theme::BorderColor));

        draw->AddCircleFilled(ImVec2(bottomPos.x + 25, bottomPos.y + 18), 4.0f, ImColor(ui::theme::Primary));
        draw->AddText(ImVec2(bottomPos.x + 35, bottomPos.y + 11), ImColor(ui::theme::TextGray), "External");

        bool is_secured = engine::security::is_active();
        draw->AddCircleFilled(ImVec2(bottomPos.x + 120, bottomPos.y + 18), 4.0f,
                              is_secured ? ImColor(0.2f, 0.9f, 0.3f) : ImColor(0.9f, 0.6f, 0.1f));
        draw->AddText(ImVec2(bottomPos.x + 130, bottomPos.y + 11), ImColor(ui::theme::TextGray),
                      is_secured ? "PROTECTION ACTIVE" : "PROTECTION PENDING");

        draw->AddText(ImVec2(ImGui::GetWindowPos().x + panelW - 75, bottomPos.y + 11), ImColor(ui::theme::PrimaryLight), "ACTIVE");

        ImGui::EndChild();
        ImGui::End();
    }
    ImGui::PopStyleVar();

    // Render active toasts from imgui-notify library
    ImGui::RenderNotifications();
}

} // namespace ui::menu
