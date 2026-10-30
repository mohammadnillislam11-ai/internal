#pragma once
#include <string>

/**
 * @file license_client.h
 * @brief Secure Online License Verification and Anti-Tamper Authentication Client.
 */
namespace auth {

    struct LicenseResult {
        bool success{false};
        std::string message;
        std::string expiration_date;
        int days_left{0};
        int exp_year{0};
        int exp_month{0};
        int exp_day{0};
        std::string token;
    };

    /**
     * @brief Computes device hardware ID (HWID) based on unique hardware attributes.
     * @return 64-character SHA256 hex string.
     */
    std::string get_device_hwid();

    enum class AuthBackend {
        FIREBASE,
        NODE_JS
    };

    void set_backend(AuthBackend backend);
    AuthBackend get_backend();

    /**
     * @brief Performs secure HTTPS online license verification.
     * @param license_key User entered product key.
     * @return LicenseResult with status and expiry.
     */
    LicenseResult verify_license_online(const std::string& license_key);

    using RevokeCallback = void(*)();
    void set_revoke_callback(RevokeCallback cb);

    /**
     * @brief Starts periodic server heartbeat watchdog to ensure live database connectivity.
     */
    void start_heartbeat_watchdog(const std::string& key);

    /**
     * @brief Stops heartbeat thread cleanly on client exit.
     */
    void stop_heartbeat_watchdog();

    bool is_heartbeat_active();

    // Global license activation state
    inline bool g_licensed = false;
    inline std::string g_expiration_date = "";
    inline int g_days_left = 0;
    inline int g_exp_year = 0;
    inline int g_exp_month = 0;
    inline int g_exp_day = 0;

} // namespace auth

