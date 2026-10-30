#include "license_view.h"
#include "menu.hpp"
#include "style.hpp"
#include "license_client.h"
#include "config_store.h"
#include "logger.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_notify.h"
#include <cstring>
#include <cstdio>
#include <string>
#include <algorithm>

extern void request_exit(int code);

namespace ui::license {

namespace {
    static char s_license_key[128] = "";
    static char s_status_msg[128]  = "Please enter your VIP license key";
    static ImVec4 s_status_color   = ImVec4(0.75f, 0.70f, 0.75f, 1.0f);
    static bool s_key_initialized  = false;
}

bool render() {
    if (g_licensed) return true;

    if (!s_key_initialized) {
        auth::set_revoke_callback([]() {
            g_licensed = false;
            logger::log_warn("[LICENSE-REVOKED] Session terminated by server! Client locked.");
        });

        std::string saved = storage::get_saved_license();
        if (!saved.empty()) {
            snprintf(s_license_key, sizeof(s_license_key), "%s", saved.c_str());
        } else {
            s_license_key[0] = '\0';
        }
        s_key_initialized = true;
    }

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    float winW = std::min(420.0f, ds.x - 24.0f);
    float winH = 175.0f;

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((ds.x - winW) * 0.5f, (ds.y - winH) * 0.5f), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 16));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ui::theme::WindowBg);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

    if (ImGui::Begin("##LicenseLoginModal", nullptr, flags)) {
        float avail_w = ImGui::GetContentRegionAvail().x;

        // Clean label: License Key
        ImGui::TextColored(ui::theme::TextWhite, "License Key:");
        ImGui::Spacing();

        // 1. Single Data Input Field (Android standard text input, paste enabled)
        ImGui::PushItemWidth(avail_w);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 10));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ui::theme::FrameBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ui::theme::FrameHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ui::theme::FrameActive);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ui::theme::TextWhite);

        ImGui::InputTextWithHint("##KeyInput", "Enter VIP license key...", s_license_key, sizeof(s_license_key));

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(2);
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // Feedback / Status message line
        ImGui::TextColored(s_status_color, "%s", s_status_msg);

        ImGui::Spacing();

        // 2. Exactly TWO side-by-side buttons: EXIT (left) and VERIFY (right)
        float spacing = 12.0f;
        float btnW = (avail_w - spacing) * 0.5f;
        float btnH = 40.0f;

        // Button Left: EXIT
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.10f, 0.18f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.14f, 0.24f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.68f, 0.18f, 0.30f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.85f, 0.90f, 1.0f));

        if (ImGui::Button("EXIT", ImVec2(btnW, btnH))) {
            ::request_exit(0);
        }
        ImGui::PopStyleColor(5);

        ImGui::SameLine(0, spacing);

        // Button Right: VERIFY
        ImGui::PushStyleColor(ImGuiCol_Button, ui::theme::PrimaryBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ui::theme::Primary);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ui::theme::PrimaryLight);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ui::theme::TextWhite);

        if (ImGui::Button("VERIFY", ImVec2(btnW, btnH))) {
            if (strlen(s_license_key) >= 2) {
                snprintf(s_status_msg, sizeof(s_status_msg), "Verifying license...");
                s_status_color = ImVec4(0.95f, 0.85f, 0.35f, 1.0f);

                logger::log_info("[LICENSE-SUBMIT] Submitting key for online verification: %s", s_license_key);
                auto result = auth::verify_license_online(s_license_key);
                if (result.success) {
                    g_licensed = true;
                    ui::menu::g_open = false; // Floating logo puck opens FIRST
                    if (!result.expiration_date.empty()) {
                        g_expiration_date = result.expiration_date;
                    }
                    g_days_left = result.days_left;
                    g_exp_year = result.exp_year;
                    g_exp_month = result.exp_month;
                    g_exp_day = result.exp_day;

                    storage::set_saved_license(s_license_key);

                    if (g_days_left > 0) {
                        snprintf(s_status_msg, sizeof(s_status_msg), "Valid until %s (%d days left)", g_expiration_date.c_str(), g_days_left);
                    } else {
                        snprintf(s_status_msg, sizeof(s_status_msg), "Valid until %s", g_expiration_date.c_str());
                    }
                    s_status_color = ImVec4(0.35f, 0.95f, 0.45f, 1.0f);
                    ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Success, 4000, "License activated successfully!"));
                    logger::log_info("[LICENSE-SUCCESS] Key activated successfully!");
                    auth::start_heartbeat_watchdog(s_license_key);
                } else {
                    snprintf(s_status_msg, sizeof(s_status_msg), "%s", result.message.c_str());
                    s_status_color = ImVec4(0.95f, 0.30f, 0.30f, 1.0f);
                    ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Error, 4000, "%s", result.message.c_str()));
                    logger::log_warn("[LICENSE-FAIL] Key verification rejected: %s", result.message.c_str());
                }
            } else {
                snprintf(s_status_msg, sizeof(s_status_msg), "Error: License key too short!");
                s_status_color = ImVec4(0.95f, 0.30f, 0.30f, 1.0f);
            }
        }
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar();
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    return g_licensed;
}

} // namespace ui::license
