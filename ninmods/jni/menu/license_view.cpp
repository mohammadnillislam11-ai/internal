#include "license_view.h"
#include "style.hpp"
#include "license_client.h"
#include "config_store.h"
#include "logger.h"
#include "Icon.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_notify.h"
#include <cstring>
#include <cstdio>
#include <string>
#include <algorithm>

#ifndef ICON_FA_CROWN
#define ICON_FA_CROWN "\xef\x94\xa1"
#endif

extern void request_exit(int code);

namespace ui::license {

namespace {
    static char s_license_key[128] = "";
    static char s_status_msg[128]  = "Ready. Tap keys below or press Paste to enter key.";
    static ImVec4 s_status_color   = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
    static bool s_key_initialized  = false;

    // Helper to render responsive on-screen virtual keyboard
    void render_keyboard(char* buf, size_t max_len, float avail_w) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.10f, 0.18f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.15f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ui::theme::PrimaryBg);
        ImGui::PushStyleColor(ImGuiCol_Text, ui::theme::TextWhite);

        // Row 1: Numbers and hyphen (11 keys)
        const char* row1[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-" };
        float keyW1 = (avail_w - (10 * 4.0f)) / 11.0f;
        for (int i = 0; i < 11; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(row1[i], ImVec2(keyW1, 30.0f))) {
                size_t len = strlen(buf);
                if (len + 1 < max_len) {
                    buf[len] = row1[i][0];
                    buf[len + 1] = '\0';
                }
            }
        }

        // Row 2: QWERTY top row (10 keys)
        const char* row2[] = { "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P" };
        float keyW2 = (avail_w - (9 * 4.0f)) / 10.0f;
        for (int i = 0; i < 10; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(row2[i], ImVec2(keyW2, 30.0f))) {
                size_t len = strlen(buf);
                if (len + 1 < max_len) {
                    buf[len] = row2[i][0];
                    buf[len + 1] = '\0';
                }
            }
        }

        // Row 3: ASDF home row (9 keys)
        const char* row3[] = { "A", "S", "D", "F", "G", "H", "J", "K", "L" };
        float keyW3 = (avail_w - (8 * 4.0f)) / 9.0f;
        for (int i = 0; i < 9; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(row3[i], ImVec2(keyW3, 30.0f))) {
                size_t len = strlen(buf);
                if (len + 1 < max_len) {
                    buf[len] = row3[i][0];
                    buf[len + 1] = '\0';
                }
            }
        }

        // Row 4: ZXCV bottom row (7 keys) + Underscore + DEL
        const char* row4[] = { "Z", "X", "C", "V", "B", "N", "M", "_" };
        float delW = 56.0f;
        float keyW4 = (avail_w - (8 * 4.0f) - delW) / 8.0f;
        for (int i = 0; i < 8; i++) {
            if (i > 0) ImGui::SameLine();
            if (ImGui::Button(row4[i], ImVec2(keyW4, 30.0f))) {
                size_t len = strlen(buf);
                if (len + 1 < max_len) {
                    buf[len] = row4[i][0];
                    buf[len + 1] = '\0';
                }
            }
        }
        // DEL / Backspace button
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.12f, 0.16f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.18f, 0.22f, 1.0f));
        if (ImGui::Button("DEL", ImVec2(delW, 30.0f))) {
            size_t len = strlen(buf);
            if (len > 0) {
                buf[len - 1] = '\0';
            }
        }
        ImGui::PopStyleColor(2);

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();
    }
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
            // Default pre-fill with Firebase VIP Key
            snprintf(s_license_key, sizeof(s_license_key), "%s", "VIP-3DAYS-10DEV");
        }
        s_key_initialized = true;
    }

    const ImVec2 ds = ImGui::GetIO().DisplaySize;
    float winW = std::min(540.0f, ds.x - 20.0f);
    float winH = std::min(460.0f, ds.y - 20.0f);

    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2((ds.x - winW) * 0.5f, (ds.y - winH) * 0.5f), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoCollapse |
                            ImGuiWindowFlags_NoScrollbar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ui::theme::WindowBg);
    ImGui::PushStyleColor(ImGuiCol_Border, ui::theme::Primary);

    if (ImGui::Begin("LICENSE VERIFICATION", nullptr, flags)) {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 pMin = ImGui::GetWindowPos();
        ImVec2 pMax = ImVec2(pMin.x + winW, pMin.y + winH);
        float avail_w = winW - 32.0f;

        // Header Accent Banner
        draw->AddRectFilled(ImVec2(pMin.x, pMin.y), ImVec2(pMax.x, pMin.y + 42), ImColor(ui::theme::SidebarBg), 14.0f, ImDrawFlags_RoundCornersTop);
        draw->AddLine(ImVec2(pMin.x, pMin.y + 42), ImVec2(pMax.x, pMin.y + 42), ImColor(ui::theme::Primary), 1.5f);

        // Header Title with Crown
        ImGui::SetCursorPos(ImVec2(18, 11));
        ImGui::TextColored(ImColor(255, 180, 0), "%s", ICON_FA_CROWN);
        ImGui::SameLine(40);
        ImGui::TextColored(ui::theme::TextWhite, "NINMODS CLIENT - VIP ACCESS");

        ImGui::SetCursorPosY(52);
        ImGui::TextColored(ui::theme::TextGray, "Enter or tap keys below to type your license key:");
        ImGui::Spacing();

        // Key Input Display (Full Width)
        ImGui::PushItemWidth(avail_w);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ui::theme::FrameBg);
        ImGui::PushStyleColor(ImGuiCol_Border, ui::theme::Primary);
        ImGui::InputTextWithHint("##KeyInput", "Tap keyboard below or paste key...", s_license_key, sizeof(s_license_key));
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // Control Buttons: PASTE, DEFAULT KEY, CLEAR
        float ctrlW = (avail_w - (2 * 6.0f)) / 3.0f;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.12f, 0.22f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.18f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ui::theme::PrimaryBg);
        ImGui::PushStyleColor(ImGuiCol_Text, ui::theme::PrimaryLight);

        if (ImGui::Button("PASTE CLIPBOARD", ImVec2(ctrlW, 28.0f))) {
            const char* clip = ImGui::GetClipboardText();
            if (clip && strlen(clip) > 0) {
                snprintf(s_license_key, sizeof(s_license_key), "%s", clip);
                snprintf(s_status_msg, sizeof(s_status_msg), "Key pasted from clipboard!");
                s_status_color = ImVec4(0.4f, 0.9f, 0.5f, 1.0f);
            }
        }
        ImGui::SameLine(0, 6.0f);
        if (ImGui::Button("DEFAULT VIP KEY", ImVec2(ctrlW, 28.0f))) {
            snprintf(s_license_key, sizeof(s_license_key), "VIP-3DAYS-10DEV");
            snprintf(s_status_msg, sizeof(s_status_msg), "Loaded VIP-3DAYS-10DEV key!");
            s_status_color = ImVec4(0.4f, 0.9f, 0.5f, 1.0f);
        }
        ImGui::SameLine(0, 6.0f);
        if (ImGui::Button("CLEAR", ImVec2(ctrlW, 28.0f))) {
            s_license_key[0] = '\0';
            snprintf(s_status_msg, sizeof(s_status_msg), "Key field cleared.");
            s_status_color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        }
        ImGui::PopStyleColor(4);

        ImGui::Spacing();

        // ── Touch Virtual Keyboard ──
        render_keyboard(s_license_key, sizeof(s_license_key), avail_w);

        ImGui::Spacing();
        ImGui::TextColored(s_status_color, "%s", s_status_msg);
        ImGui::Spacing();

        // Activation Button (Large, Glowing Booyah Pink)
        ImGui::PushStyleColor(ImGuiCol_Button, ui::theme::Primary);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ui::theme::PrimaryLight);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.45f, 0.75f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));

        if (ImGui::Button("1-CLICK ACTIVATE VIP ACCESS", ImVec2(avail_w, 38.0f))) {
            if (strlen(s_license_key) >= 4) {
                snprintf(s_status_msg, sizeof(s_status_msg), "Connecting to authentication server...");
                s_status_color = ImVec4(0.9f, 0.9f, 0.2f, 1.0f);

                logger::log_info("[LICENSE-SUBMIT] Submitting key for online verification: %s", s_license_key);
                logger::log_info("[LICENSE-CONNECT] Authenticating with license database (HWID: %s)...",
                                 auth::get_device_hwid().substr(0, 16).c_str());

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

                    logger::log_info("[LICENSE-SUCCESS] Key activated successfully!");
                    logger::log_info("[+] Expiry Date: %s | Days Left: %d (Month: %02d, Year: %04d)",
                                     g_expiration_date.c_str(), g_days_left, g_exp_month, g_exp_year);

                    // Start persistent heartbeat watchdog with Firebase
                    auth::start_heartbeat_watchdog(s_license_key);
                } else {
                    snprintf(s_status_msg, sizeof(s_status_msg), "%s", result.message.c_str());
                    s_status_color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
                    ImGui::InsertNotification(ImGuiToast(ImGuiToastType_Error, 4000, "%s", result.message.c_str()));

                    logger::log_warn("[LICENSE-FAIL] Key verification rejected: %s", result.message.c_str());
                }
            } else {
                snprintf(s_status_msg, sizeof(s_status_msg), "Error: License key too short!");
                s_status_color = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
                logger::log_warn("[LICENSE-FAIL] Input rejected: License key too short (%zu characters).", strlen(s_license_key));
            }
        }
        ImGui::PopStyleColor(4);

        ImGui::Spacing();

        // Exit / Terminate button from Menu
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.12f, 0.12f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.60f, 0.18f, 0.18f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.85f, 0.85f, 1.0f));

        if (ImGui::Button("EXIT CLIENT (CODE 0)", ImVec2(avail_w, 28.0f))) {
            ::request_exit(0);
        }
        ImGui::PopStyleColor(4);
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    return g_licensed;
}

} // namespace ui::license
