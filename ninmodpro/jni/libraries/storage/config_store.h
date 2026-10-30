#pragma once
#include <string>

/**
 * @file config_store.h
 * @brief Persistent configuration and license state storage using JSON.
 */
namespace storage {

    bool load_config();
    bool save_config();

    std::string get_saved_license();
    void set_saved_license(const std::string& key);

    std::string get_firebase_url();
    void set_firebase_url(const std::string& url);

    std::string get_firebase_auth();
    void set_firebase_auth(const std::string& auth);

    std::string get_auth_backend();
    void set_auth_backend(const std::string& backend);

    std::string get_node_url();
    void set_node_url(const std::string& url);

    std::string get_active_server_display();

} // namespace storage
