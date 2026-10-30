#include "license_client.h"
#include "firebase_client.hpp"
#include "node_client.hpp"
#include "oxorany.h"
#include <curl/curl.h>
#include <openssl/sha.h>
#include "json.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <chrono>

namespace auth {

namespace {
    static AuthBackend s_current_backend = AuthBackend::FIREBASE;
    static RevokeCallback s_revoke_cb = nullptr;
    static std::atomic<bool> s_heartbeat_running{false};
    static std::string s_active_key = "";

    // Callback for cURL response body
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t totalSize = size * nmemb;
        std::string* mem = static_cast<std::string*>(userp);
        mem->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }

    // SHA256 helper for HWID calculation
    std::string Sha256Hex(const std::string& input) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, input.c_str(), input.length());
        SHA256_Final(hash, &sha256);

        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        return ss.str();
    }

    static void append_system_prop(std::string& out, const char* prop) {
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "getprop %s", prop);
        FILE* fp = popen(cmd, "r");
        if (fp) {
            char buf[256];
            if (fgets(buf, sizeof(buf), fp)) {
                buf[strcspn(buf, "\r\n")] = 0;
                if (buf[0] != '\0') {
                    if (!out.empty()) out.push_back(':');
                    out += buf;
                }
            }
            pclose(fp);
        }
    }

} // namespace

std::string get_device_hwid() {
    static std::string s_cached_hwid = "";
    if (!s_cached_hwid.empty()) {
        return s_cached_hwid;
    }

    std::string raw_id;
    append_system_prop(raw_id, "ro.serialno");
    append_system_prop(raw_id, "ro.boot.serialno");
    append_system_prop(raw_id, "ro.hardware");
    append_system_prop(raw_id, "ro.product.board");
    append_system_prop(raw_id, "ro.board.platform");
    append_system_prop(raw_id, "ro.product.manufacturer");
    append_system_prop(raw_id, "ro.product.brand");
    append_system_prop(raw_id, "ro.build.fingerprint");

    if (raw_id.empty()) {
        raw_id = "NINMODS_SECURE_PERMANENT_HWID";
    }

    s_cached_hwid = Sha256Hex(raw_id);
    return s_cached_hwid;
}

void set_backend(AuthBackend backend) {
    s_current_backend = backend;
}

AuthBackend get_backend() {
    return s_current_backend;
}

LicenseResult verify_license_online(const std::string& license_key) {
    LicenseResult res;
    if (license_key.empty()) {
        res.success = false;
        res.message = "License key cannot be empty.";
        return res;
    }

    std::string hwid = get_device_hwid();

    // 1. Try Primary Configured Backend (Default: Firebase Realtime Database)
    if (s_current_backend == AuthBackend::FIREBASE) {
        res = firebase::verify_license(license_key, hwid);
    } else {
        res = node::verify_license(license_key, hwid);
    }

    return res;
}

void set_revoke_callback(RevokeCallback cb) {
    s_revoke_cb = cb;
}

bool is_heartbeat_active() {
    return s_heartbeat_running.load(std::memory_order_relaxed);
}

void start_heartbeat_watchdog(const std::string& key) {
    if (s_heartbeat_running.load()) return;
    s_active_key = key;
    s_heartbeat_running.store(true);

    std::thread t([]() {
        printf("[HEARTBEAT-WATCHDOG] Continuous connection watchdog started for live license monitoring.\n");
        fflush(stdout);

        while (s_heartbeat_running.load()) {
            // Check every 120 seconds, checking stop flag every second
            for (int i = 0; i < 120 && s_heartbeat_running.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!s_heartbeat_running.load()) break;

            auto res = verify_license_online(s_active_key);
            if (!res.success) {
                printf("[HEARTBEAT-ALERT] License session invalidated by server: %s\n", res.message.c_str());
                fflush(stdout);
                if (s_revoke_cb) {
                    s_revoke_cb();
                }
                s_heartbeat_running.store(false);
                break;
            } else {
                printf("[HEARTBEAT-PING] Live server sync OK | Valid: %s | Days Left: %d\n",
                       res.expiration_date.c_str(), res.days_left);
                fflush(stdout);
            }
        }
    });
    t.detach();
}

void stop_heartbeat_watchdog() {
    s_heartbeat_running.store(false);
}

} // namespace auth

