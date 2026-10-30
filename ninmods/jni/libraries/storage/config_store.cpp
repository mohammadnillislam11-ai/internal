#include "config_store.h"
#include "../../menu/config.hpp"
#include "../../menu/license_view.h"
#include "../network/license_client.h"
#include "../network/firebase_client.hpp"
#include "../network/node_client.hpp"
#include "json.hpp"
#include <fstream>
#include <cstdio>

namespace storage {

namespace {
    static const char* CONFIG_FILE_PATH = "/data/local/tmp/ninmods_config.json";
    static std::string s_cached_license = "";
    static std::string s_firebase_url = "https://ninmods-auth-default-rtdb.asia-southeast1.firebasedatabase.app";
    static std::string s_firebase_auth = "";
    static std::string s_node_url = "https://api.ninmods.com";
    static std::string s_auth_backend = "firebase";
}

std::string get_saved_license() {
    return s_cached_license;
}

void set_saved_license(const std::string& key) {
    s_cached_license = key;
    save_config();
}

std::string get_firebase_url() {
    return s_firebase_url;
}

void set_firebase_url(const std::string& url) {
    s_firebase_url = url;
    auth::firebase::configure({"", s_firebase_url, s_firebase_auth});
    save_config();
}

std::string get_firebase_auth() {
    return s_firebase_auth;
}

void set_firebase_auth(const std::string& auth) {
    s_firebase_auth = auth;
    auth::firebase::configure({"", s_firebase_url, s_firebase_auth});
    save_config();
}

std::string get_auth_backend() {
    return s_auth_backend;
}

void set_auth_backend(const std::string& backend) {
    s_auth_backend = backend;
    if (s_auth_backend == "nodejs" || s_auth_backend == "node") {
        auth::set_backend(auth::AuthBackend::NODE_JS);
    } else {
        auth::set_backend(auth::AuthBackend::FIREBASE);
    }
    save_config();
}

std::string get_node_url() {
    return s_node_url;
}

void set_node_url(const std::string& url) {
    s_node_url = url;
    auth::node::configure({s_node_url, "/api/v1/license"});
    save_config();
}

std::string get_active_server_display() {
    if (s_auth_backend == "nodejs" || s_auth_backend == "node") {
        return s_node_url;
    }
    return s_firebase_url;
}

bool load_config() {
    std::ifstream file(CONFIG_FILE_PATH);
    if (!file.is_open()) return false;

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("license_key")) {
            s_cached_license = j["license_key"].get<std::string>();
        }

        if (j.contains("firebase_url") && !j["firebase_url"].get<std::string>().empty()) {
            s_firebase_url = j["firebase_url"].get<std::string>();
        }

        if (j.contains("firebase_auth")) {
            s_firebase_auth = j["firebase_auth"].get<std::string>();
        }

        if (j.contains("node_url") && !j["node_url"].get<std::string>().empty()) {
            s_node_url = j["node_url"].get<std::string>();
        }

        if (j.contains("auth_backend") && !j["auth_backend"].get<std::string>().empty()) {
            s_auth_backend = j["auth_backend"].get<std::string>();
        }

        // Apply loaded configuration to auth clients
        auth::firebase::configure({"", s_firebase_url, s_firebase_auth});
        auth::node::configure({s_node_url, "/api/v1/license"});
        if (s_auth_backend == "nodejs" || s_auth_backend == "node") {
            auth::set_backend(auth::AuthBackend::NODE_JS);
        } else {
            auth::set_backend(auth::AuthBackend::FIREBASE);
        }

        if (j.contains("esp")) {
            auto& esp = j["esp"];
            cfg::esp::enabled    = esp.value("enabled", false);
            cfg::esp::box        = esp.value("box", false);
            cfg::esp::tracer     = esp.value("tracer", false);
            cfg::esp::skeleton   = esp.value("skeleton", false);
            cfg::esp::health     = esp.value("health", false);
            cfg::esp::name       = esp.value("name", false);
            cfg::esp::distance   = esp.value("distance", false);
            cfg::esp::max_range  = esp.value("max_range", 250.0f);
        }

        if (j.contains("aim")) {
            auto& aim = j["aim"];
            cfg::aim::enabled    = aim.value("enabled", false);
            cfg::aim::silent     = aim.value("silent", false);
            cfg::aim::target_head= aim.value("target_head", true);
            cfg::aim::fov        = aim.value("fov", 60.0f);
            cfg::aim::distance   = aim.value("distance", 250.0f);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool save_config() {
    try {
        nlohmann::json j;
        j["license_key"]   = s_cached_license;
        j["firebase_url"]  = s_firebase_url;
        j["firebase_auth"] = s_firebase_auth;
        j["node_url"]      = s_node_url;
        j["auth_backend"]  = s_auth_backend;

        nlohmann::json esp;
        esp["enabled"]    = cfg::esp::enabled;
        esp["box"]        = cfg::esp::box;
        esp["tracer"]     = cfg::esp::tracer;
        esp["skeleton"]   = cfg::esp::skeleton;
        esp["health"]     = cfg::esp::health;
        esp["name"]       = cfg::esp::name;
        esp["distance"]   = cfg::esp::distance;
        esp["max_range"]  = cfg::esp::max_range;
        j["esp"] = esp;

        nlohmann::json aim;
        aim["enabled"]    = cfg::aim::enabled;
        aim["silent"]     = cfg::aim::silent;
        aim["target_head"]= cfg::aim::target_head;
        aim["fov"]        = cfg::aim::fov;
        aim["distance"]   = cfg::aim::distance;
        j["aim"] = aim;

        std::ofstream file(CONFIG_FILE_PATH);
        if (!file.is_open()) return false;

        file << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace storage
