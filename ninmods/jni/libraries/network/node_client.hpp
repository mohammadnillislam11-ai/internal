#pragma once
#include <string>
#include "license_client.h"

/**
 * @file node_client.hpp
 * @brief High-performance REST & HMAC authentication client for custom Node.js licensing server.
 */
namespace auth::node {

    struct NodeConfig {
        std::string server_url{"https://api.ninmods.com"};
        std::string api_prefix{"/api/v1/license"};
    };

    /**
     * @brief Configure Node.js server base URL.
     */
    void configure(const NodeConfig& config);

    /**
     * @brief Get active Node.js configuration.
     */
    NodeConfig get_config();

    /**
     * @brief Verify license with Node.js backend.
     * Endpoint: POST /api/v1/license/verify
     */
    LicenseResult verify_license(const std::string& key, const std::string& hwid);

    /**
     * @brief Periodic heartbeat to maintain active session with Node.js server.
     * Endpoint: POST /api/v1/license/heartbeat
     */
    bool send_heartbeat(const std::string& session_token);

} // namespace auth::node

