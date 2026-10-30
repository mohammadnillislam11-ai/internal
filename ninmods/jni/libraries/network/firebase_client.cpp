#include "firebase_client.hpp"
#include "oxorany.h"
#include <curl/curl.h>
#include "json.hpp"
#include <sstream>

namespace auth::firebase {

namespace {
    static FirebaseConfig s_config;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        std::string* s = static_cast<std::string*>(userp);
        s->append(static_cast<char*>(contents), total);
        return total;
    }

    std::string build_url(const std::string& key) {
        std::string url = s_config.database_url;
        if (!url.empty() && url.back() == '/') {
            url.pop_back();
        }
        url += "/licenses/" + key + ".json";
        if (!s_config.auth_token.empty()) {
            url += "?auth=" + s_config.auth_token;
        }
        return url;
    }

    std::string build_device_url(const std::string& key, const std::string& hwid) {
        std::string url = s_config.database_url;
        if (!url.empty() && url.back() == '/') {
            url.pop_back();
        }
        url += "/licenses/" + key + "/devices/" + hwid + ".json";
        if (!s_config.auth_token.empty()) {
            url += "?auth=" + s_config.auth_token;
        }
        return url;
    }
} // namespace

void configure(const FirebaseConfig& config) {
    s_config = config;
}

FirebaseConfig get_config() {
    return s_config;
}

LicenseResult verify_license(const std::string& key, const std::string& hwid) {
    LicenseResult res;
    res.success = false;

    CURL* curl = curl_easy_init();
    if (!curl) {
        res.message = "Failed to initialize SSL connection engine.";
        return res;
    }

    std::string url = build_url(key);
    std::string response_data;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "User-Agent: NINMODS-FirebaseClient/2.0");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // TLS Certificate Enforcement
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode code = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        res.message = std::string("Network error: ") + curl_easy_strerror(code);
        return res;
    }

    if (http_code == 404 || response_data.empty() || response_data == "null") {
        res.message = "License key not found or invalid.";
        return res;
    }

    try {
        auto doc = nlohmann::json::parse(response_data);

        bool active = doc.value("active", false);
        int max_devices = doc.value("max_devices", 1);
        if (max_devices < 1) max_devices = 1;

        if (!active) {
            res.message = "License key is deactivated or suspended.";
            return res;
        }

        // 1. Activation-based expiry calculation (duration_days) or explicit expiry date
        std::string expiry = doc.value("expiry", "");
        if (expiry.empty() && (doc.contains("duration_days") || doc.contains("days"))) {
            int duration_days = doc.value("duration_days", doc.value("days", 30));
            time_t now = time(nullptr);
            time_t exp_t = now + (static_cast<time_t>(duration_days) * 86400);

            char start_buf[64], exp_buf[64];
            struct tm tm_now, tm_exp;
            localtime_r(&now, &tm_now);
            localtime_r(&exp_t, &tm_exp);
            strftime(start_buf, sizeof(start_buf), "%Y-%m-%d %H:%M:%S", &tm_now);
            strftime(exp_buf, sizeof(exp_buf), "%Y-%m-%d %H:%M:%S", &tm_exp);

            std::string start_date = start_buf;
            expiry = exp_buf;

            // Commit activation timestamp & expiry date to Firebase permanently
            activate_license_dates(key, start_date, expiry);
        }

        if (expiry.empty()) {
            expiry = "UNSET";
        }

        // 2. Parse Expiration Date and Calculate Remaining Days
        int yr = 0, mo = 0, dy = 0, hr = 23, mn = 59, sc = 59;
        int matched = sscanf(expiry.c_str(), "%d-%d-%d %d:%d:%d", &yr, &mo, &dy, &hr, &mn, &sc);
        if (matched < 3) {
            matched = sscanf(expiry.c_str(), "%d-%d-%d", &yr, &mo, &dy);
        }

        if (matched >= 3) {
            struct tm tm_exp = {};
            tm_exp.tm_year = yr - 1900;
            tm_exp.tm_mon  = mo - 1;
            tm_exp.tm_mday = dy;
            tm_exp.tm_hour = hr;
            tm_exp.tm_min  = mn;
            tm_exp.tm_sec  = sc;
            time_t exp_time = mktime(&tm_exp);
            time_t now = time(nullptr);

            if (now > exp_time) {
                res.success = false;
                res.message = "License has expired on " + expiry;
                res.expiration_date = expiry;
                res.days_left = 0;
                res.exp_year = yr;
                res.exp_month = mo;
                res.exp_day = dy;
                return res;
            }

            int days_left = static_cast<int>((exp_time - now) / 86400);
            res.days_left = (days_left < 0) ? 0 : days_left;
            res.exp_year = yr;
            res.exp_month = mo;
            res.exp_day = dy;
            res.expiration_date = expiry;
        } else {
            res.expiration_date = expiry;
        }

        bool device_authorized = false;
        size_t registered_devices_count = 0;

        // 3. Check in multi-device map "devices"
        if (doc.contains("devices") && doc["devices"].is_object()) {
            auto& devices = doc["devices"];
            registered_devices_count = devices.size();

            if (devices.contains(hwid)) {
                device_authorized = true;
            }
        } else if (doc.contains("hwid") && doc["hwid"].is_string()) {
            // Legacy single device field compatibility
            std::string legacy_hwid = doc["hwid"].get<std::string>();
            if (!legacy_hwid.empty() && legacy_hwid != "NONE" && legacy_hwid != "NEW") {
                registered_devices_count = 1;
                if (legacy_hwid == hwid) {
                    device_authorized = true;
                }
            }
        }

        // 4. If device is already registered and authorized, succeed!
        if (device_authorized) {
            res.success = true;
            res.message = "License verified successfully.";
            return res;
        }

        // 5. If device not registered yet, check device limit
        if (registered_devices_count < static_cast<size_t>(max_devices)) {
            // First time activation for this device slot: register HWID
            if (register_device(key, hwid)) {
                res.success = true;
                res.message = "Device registered and license activated!";
                return res;
            } else {
                res.message = "Failed to register device hardware on server.";
                return res;
            }
        } else {
            // Device limit exceeded
            if (max_devices == 1) {
                res.message = "Key is locked to another device! Universal sharing is strictly prohibited.";
            } else {
                res.message = "Device limit reached! Maximum " + std::to_string(max_devices) + " devices allowed.";
            }
            return res;
        }

    } catch (const std::exception& e) {
        res.message = std::string("License parse error: ") + e.what();
        return res;
    }
}

bool activate_license_dates(const std::string& key, const std::string& start_date, const std::string& expiry) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = build_url(key);
    std::string patch_body = nlohmann::json({
        {"start_date", start_date},
        {"expiry", expiry}
    }).dump();
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, patch_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode code = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (code == CURLE_OK && http_code == 200);
}

bool register_device(const std::string& key, const std::string& hwid) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = build_device_url(key, hwid);
    std::string put_body = "true";
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, put_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode code = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    // Also update legacy single hwid field for backward compatibility
    if (code == CURLE_OK && http_code == 200) {
        bind_device(key, hwid);
        return true;
    }
    return false;
}

bool bind_device(const std::string& key, const std::string& hwid) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = build_url(key);
    std::string patch_body = nlohmann::json({{"hwid", hwid}}).dump();
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, patch_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

    CURLcode code = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (code == CURLE_OK && http_code == 200);
}

} // namespace auth::firebase

