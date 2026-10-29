#pragma once
#include <string>
#include <atomic>
#include <thread>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <android/log.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "ui/styles/styles.h"

// ============================================================
// EXTERNAL VARIABLES
// ============================================================
extern float global_anim_time;
extern float g_ui_scale;
extern std::atomic<bool> menu_visible;

extern std::atomic<int>  g_license_activated;
extern int               g_license_days_left;
extern std::string       g_license_error;
extern std::string       g_license_expiry_date;
extern std::string       g_license_start_date;
extern std::string       g_license_key_number;
extern std::string       g_license_device_id;
extern std::string       g_license_user_name;
extern bool              g_license_checking;
extern bool              g_license_auto_login;
extern bool              g_license_security_block;
extern std::string       g_license_security_msg;

// ============================================================
// FLAG GLOBAL - didefinisikan di main.cpp
// ============================================================
extern std::atomic<bool> g_menu_started;

// ============================================================
// EXTERNAL FUNCTIONS
// ============================================================
extern bool IsLicenseActivated();

// ============================================================
// LICENSE MENU - STUB (NO LOGIN SCREEN)
// ============================================================
inline void DrawLicenseMenu() {
    g_menu_started.store(true);
}
