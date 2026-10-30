#include "shield_manager.hpp"
#include <atomic>
#include <cstdio>
#include <android/log.h>

#define SHIELD_TAG "NINMODS_SHIELD"

namespace shield {

namespace {
    static std::atomic<bool> s_running{false};
    static std::atomic<bool> s_tampered{false};
} // namespace

void start_watchdog() {
    if (s_running.load()) return;
    s_running.store(true);
    __android_log_print(ANDROID_LOG_INFO, SHIELD_TAG, "[+] Native shield initialized in passive safe mode.");
}

void stop_watchdog() {
    s_running.store(false);
}

bool verify_environment() {
    return true;
}

bool is_tampered() {
    return s_tampered.load();
}

} // namespace shield
