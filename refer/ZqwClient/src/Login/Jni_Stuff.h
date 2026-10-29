// src/Login/Jni_Stuff.h
#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <cstring>
#include <cstdio>

// ============================================================
//  GLOBAL VARIABLES - DECLARATIONS
// ============================================================
extern std::string g_license_expiry_date;
extern std::string g_license_user_name;
extern std::string g_license_key_number;
extern std::string g_license_device_id;
extern std::string g_license_start_date;
extern std::string g_license_error;
extern int g_license_days_left;
extern std::atomic<int> g_license_activated;
extern bool g_license_checking;
extern bool g_license_auto_login;
extern bool g_license_security_block;
extern std::string g_license_security_msg;
extern char g_license_key_input[256];

extern std::string g_Token;
extern std::string g_Auth;
extern bool bValid;
extern std::string EXP;
extern std::string mod_status;
extern std::string DeviceID;
extern std::string DeviceModel;
extern std::string DeviceBrand;
extern std::string g_ModName;
extern std::string g_FloatingText;
extern std::string g_server_mod_name;
extern std::string g_server_floating_text;
extern std::string g_server_popup_message;
extern std::string g_server_button_text;
extern std::string g_server_button_url;
extern std::vector<std::string> g_server_disabled_features;
extern bool g_server_block_menu;
extern bool g_server_force_logout;

// ============================================================
//  STRUCT
// ============================================================
struct MemoryStruct {
    char* memory;
    size_t size;
};

// ============================================================
//  FUNCTIONS - DECLARATIONS
// ============================================================
std::string base64_decode(const std::string& input);
std::string AESDecrypt(const std::string& ciphertext_b64,
                       const std::string& iv_b64,
                       const std::string& key);
std::string GetDeviceID();
std::string GetDeviceModel();
std::string GetDeviceBrand();
void SaveLicenseKey(const char* key);
std::string LoadLicenseKey();
void ClearLicenseKey();
void RecalculateDaysLeft();
std::string Login(const char* user_key);
void TryAutoLogin();
bool IsLicenseActivated();
bool IsMenuBlockedByServer();
std::string GetServerModName();
std::string GetServerFloatingText();
std::string GetServerPopupMessage();
std::string GetServerButtonText();
std::string GetServerButtonUrl();
std::vector<std::string> GetServerDisabledFeatures();
std::string GetModName();
std::string GetFloatingText();
std::string GetUserKey();
int GetDaysRemaining();
std::string GetLicenseUserName();
std::string GetLicenseExpiryDate();
std::string GetLicenseStartDate();
