// src/Login/Jni_Stuff.cpp
#include "Jni_Stuff.h"

// ============================================================
//  GLOBAL VARIABLES - DEFINISI (ALWAYS ACTIVATED / NO NETWORK)
// ============================================================
std::string g_license_expiry_date = "PERMANENT";
std::string g_license_user_name = "USER";
std::string g_license_key_number = "FREE";
std::string g_license_device_id = "LOCAL";
std::string g_license_start_date = "2026-01-01";
std::string g_license_error = "";
int g_license_days_left = 9999;
std::atomic<int> g_license_activated{0x4E580021};
bool g_license_checking = false;
bool g_license_auto_login = true;
bool g_license_security_block = false;
std::string g_license_security_msg = "";
char g_license_key_input[256] = {0};

std::string g_Token = "TOKEN";
std::string g_Auth = "AUTH";
bool bValid = true;
std::string EXP = "9999-12-31";
std::string mod_status = " SAFE";
std::string DeviceID = "LOCAL";
std::string DeviceModel = "ANDROID";
std::string DeviceBrand = "DEVICE";
std::string g_ModName = "NINMODS";
std::string g_FloatingText = "NINMODS | ACTIVE";
std::string g_server_mod_name = "";
std::string g_server_floating_text = "";
std::string g_server_popup_message = "";
std::string g_server_button_text = "";
std::string g_server_button_url = "";
std::vector<std::string> g_server_disabled_features;
bool g_server_block_menu = false;
bool g_server_force_logout = false;

std::string base64_decode(const std::string& input) { return ""; }
std::string AESDecrypt(const std::string& ciphertext_b64, const std::string& iv_b64, const std::string& key) { return ""; }
std::string GetDeviceID() { return "LOCAL"; }
std::string GetDeviceModel() { return "ANDROID"; }
std::string GetDeviceBrand() { return "DEVICE"; }
void SaveLicenseKey(const char* key) {}
std::string LoadLicenseKey() { return "FREE"; }
void ClearLicenseKey() {}
void RecalculateDaysLeft() {}
std::string Login(const char* user_key) { return "OK"; }
void TryAutoLogin() {}
bool IsLicenseActivated() { return true; }
bool IsMenuBlockedByServer() { return false; }
std::string GetServerModName() { return ""; }
std::string GetServerFloatingText() { return ""; }
std::string GetServerPopupMessage() { return ""; }
std::string GetServerButtonText() { return ""; }
std::string GetServerButtonUrl() { return ""; }
std::vector<std::string> GetServerDisabledFeatures() { return {}; }
std::string GetModName() { return "NINMODS"; }
std::string GetFloatingText() { return "NINMODS"; }
std::string GetUserKey() { return "FREE"; }
int GetDaysRemaining() { return 9999; }
std::string GetLicenseUserName() { return "USER"; }
std::string GetLicenseExpiryDate() { return "PERMANENT"; }
std::string GetLicenseStartDate() { return "2026-01-01"; }
