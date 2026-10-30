#include "node_client.hpp"
#include "oxorany.h"
#include <curl/curl.h>
#include "json.hpp"

namespace auth::node {

namespace {
    static NodeConfig s_config;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total = size * nmemb;
        std::string* s = static_cast<std::string*>(userp);
        s->append(static_cast<char*>(contents), total);
        return total;
    }
} // namespace

void configure(const NodeConfig& config) {
    s_config = config;
}

NodeConfig get_config() {
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

    std::string endpoint = s_config.server_url + s_config.api_prefix + "/verify";
    std::string req_body = nlohmann::json({
        {"key", key},
        {"hwid", hwid},
        {"client_version", "2.0"},
        {"os", "Android"}
    }).dump();

    std::string response_data;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "User-Agent: NINMODS-NodeClient/2.0");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    // TLS Certificate Verification
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
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

    try {
        auto json_res = nlohmann::json::parse(response_data);
        res.success = json_res.value("success", false);
        res.message = json_res.value("message", "Unknown server response");
        res.expiration_date = json_res.value("expiration", "PERMANENT");
        return res;
    } catch (const std::exception& e) {
        res.message = "Server response parse error.";
        return res;
    }
}

bool send_heartbeat(const std::string& session_token) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string endpoint = s_config.server_url + s_config.api_prefix + "/heartbeat";
    std::string req_body = nlohmann::json({{"token", session_token}}).dump();
    std::string response;

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode code = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (code == CURLE_OK && http_code == 200);
}

} // namespace auth::node

