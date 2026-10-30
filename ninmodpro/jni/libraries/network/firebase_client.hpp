#pragma once
#include <string>
#include "license_client.h"

/**
 * @file firebase_client.hpp
 * @brief Official Firebase REST API client for license validation and device HWID binding.
 * Powered by libcurl (TLS 1.2/1.3) and nlohmann/json.
 */
namespace auth::firebase {

    struct FirebaseConfig {
        std::string project_id{"ninmods-auth"};
        std::string database_url{"https://ninmods-auth-default-rtdb.asia-southeast1.firebasedatabase.app"};
        std::string auth_token{""}; // Optional Firebase database secret or token
    };

    /**
     * @brief Configure Firebase project credentials.
     */
    void configure(const FirebaseConfig& config);

    /**
     * @brief Get active Firebase configuration.
     */
    FirebaseConfig get_config();

    /**
     * @brief Verify license against Firebase Realtime Database.
     * Endpoint: GET /licenses/{key}.json
     */
    LicenseResult verify_license(const std::string& key, const std::string& hwid);

    /**
     * @brief Register a device HWID under a multi-device license.
     * Endpoint: PUT /licenses/{key}/devices/{hwid}.json
     */
    bool register_device(const std::string& key, const std::string& hwid);

    /**
     * @brief Set activation start and expiration dates on first use.
     * Endpoint: PATCH /licenses/{key}.json
     */
    bool activate_license_dates(const std::string& key, const std::string& start_date, const std::string& expiry);

    /**
     * @brief Bind an unassigned license to this device's HWID in Firebase.
     * Endpoint: PATCH /licenses/{key}.json
     */
    bool bind_device(const std::string& key, const std::string& hwid);

} // namespace auth::firebase

