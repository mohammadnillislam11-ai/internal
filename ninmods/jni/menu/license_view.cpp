#include "license_view.h"
#include "style.hpp"
#include "license_client.h"
#include "config_store.h"
#include "imgui.h"
#include "imgui_notify.h"
#include <cstring>
#include <cstdio>

namespace ui::license {

namespace {
    static char s_license_key[128] = "";
    static char s_status_msg[128]  = "Please enter your license key to continue.";
    static ImVec4 s_status_color   = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    static bool s_key_initialized  = false;
}

bool render() {
    if (g_licensed) return true;

    if (!s_key_initialized) {
        auth::set_revoke_callback([]() {
            g_licensed = false;
            printf("[LICENSE-REVOKED] Session terminated by server! Client locked.\n");
            fflush(stdout);
        });

        std::string saved = storage::get_saved_license();
        if (!saved.empty()) {
            snprintf(s_license_key, sizeof(s_license_key), "%s", saved.c_str());
        }
        s_key_initialized = true;
    }

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    float winW = 440.0f;
    float winH = 260.0f;

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((ds.x - winW) * 0.5f, (ds.y - winH) * 0.5f), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ui::theme::WindowBg);
    ImGui::PushStyleColor(ImGuiCol_Border, ui::theme::Primary);

    if (ImGui::Begin("LICENSE VERIFICATION", nullptr, flags)) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pMin = ImGui::GetWindowPos();
        ImVec2 pMax = ImVec2(pMin.x + winW, pMin.y + winH);

        // Header accent line
        draw->AddLine(ImVec2(pMin.x + 15, pMin.y + 40), ImVec2(pMax.x - 15, pMin.y + 40),
                      ImColor(ui::theme::Primary), 1.5f);

        ImGui::SetCursorPosY(50);
        ImGui::TextColored(ui::theme::TextWhite, "Enter License Key:");
        ImGui::Spacing();

        // Key Input with Paste Button
        ImGui::PushItemWidth(winW - 130.0f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ui::theme::FrameBg);
        ImGui::PushStyleColor(ImGuiCol_Border, ui::theme::BorderColor);
        ImGui::InputTextWithHint("##KeyInput", "Paste or enter key...", s_license_key, sizeof(s_license_key));
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("PASTE", ImVec2(75, 26))) {
            const char* clip = ImGui::GetClipboardText();
            if (clip && strlen(clip) > 0) {
                snprintf(s_license_key, sizeof(s_license_key), "%s", clip);
            }
        }

        ImGui::Spacing();
        ImGui::TextColored(s_status_color, "%s", s_status_msg);
        ImGui::Spacing();
        ImGui::Spacing();

        // Activation Button
        ImGui::PushStyleColor(ImGuiCol_Button, ui::theme::PrimaryBg);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ui::theme::Primary);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ui::theme::PrimaryLight);
        ImGui::PushStyleColor(ImGuiCol_Text, ui::theme::TextWhite);

        if (ImGui::Button("ACTIVATE NOW", ImVec2(winW - 40.0f, 40.0f))) {
            if (strlen(s_license_key) >= 4) {
                snprintf(s_status_msg, sizeof(s_status_msg), "Connecting to authentication server...");
                s_status_color = ImVec4(0.9f, 0.9f, 0.2f, 1.0f);

                printf("[LICENSE-SUBMIT] Submitting key for online verification: %s\n", s_license_key);
                printf("[LICENSE-CONNECT] Authenticating with license database (HWID: %s)...\n",
                       auth::get_device_hwid().substr(0, 16).c_str());
                fflush(stdout);

                auto result = auth::verify_license_online(s_license_key);
                if (result.success) {
                    g_licensed = true;
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
                    s_status_color = ImVec4(0.2f, 0.9f, 0.3f, 1.0f);
                    ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Success, 4000, "License activated successfully!"));

                    printf("[LICENSE-SUCCESS] Key activated successfully!\n");
                    printf("[+] Expiry Date: %s | Days Left: %d (Month: %02d, Year: %04d)\n",
                           g_expiration_date.c_str(), g_days_left, g_exp_month, g_exp_year);
                    fflush(stdout);

                    // Start persistent heartbeat watchdog with Firebase
                    auth::start_heartbeat_watchdog(s_license_key);
                } else {
                    snprintf(s_status_msg, sizeof(s_status_msg), "%s", result.message.c_str());
                    s_status_color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
                    ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Error, 4000, "%s", result.message.c_str()));

                    printf("[LICENSE-FAIL] Key verification rejected: %s\n", result.message.c_str());
                    fflush(stdout);
                }
            } else {
                snprintf(s_status_msg, sizeof(s_status_msg), "Error: License key too short!");
                s_status_color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
                printf("[LICENSE-FAIL] Input rejected: License key too short (%zu characters).\n", strlen(s_license_key));
                fflush(stdout);
            }
        }

        ImGui::PopStyleColor(4);
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    return g_licensed;
}

} // namespace ui::license
