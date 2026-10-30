#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>
#include <mutex>
#include <android/log.h>

#define LOG_TAG "NINMODS"

namespace logger {

namespace {
    static FILE* s_file = nullptr;
    static std::string s_path = "/data/local/tmp/ninmods.log";
    static std::mutex s_mutex;

    static void write_entry(const char* level, const char* fmt, va_list args) {
        std::lock_guard<std::mutex> lock(s_mutex);

        time_t now = time(nullptr);
        struct tm tm_buf{};
        localtime_r(&now, &tm_buf);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_buf);

        char msg_buf[1024];
        va_list args_copy;
        va_copy(args_copy, args);
        vsnprintf(msg_buf, sizeof(msg_buf), fmt, args_copy);
        va_end(args_copy);

        // 1. Output to Terminal with ANSI colors (Red for error, Green for success)
        const char* color_prefix = "";
        const char* color_suffix = "\033[0m";

        if (strcmp(level, "ERROR") == 0 || (msg_buf[0] == '[' && msg_buf[1] == '-')) {
            color_prefix = "\033[91m"; // Red
        } else if (strcmp(level, "SUCCESS") == 0 || (msg_buf[0] == '[' && msg_buf[1] == '+')) {
            color_prefix = "\033[92m"; // Bright Green
        } else if (strcmp(level, "WARN") == 0 || (msg_buf[0] == '[' && msg_buf[1] == '*')) {
            color_prefix = "\033[93m"; // Bright Yellow
        } else {
            color_prefix = "\033[0m";
            color_suffix = "";
        }

        printf("%s[%s] [%s] %s%s\n", color_prefix, time_str, level, msg_buf, color_suffix);
        fflush(stdout);

        // 2. Output to Disk file without ANSI colors
        if (s_file) {
            fprintf(s_file, "[%s] [%s] %s\n", time_str, level, msg_buf);
            fflush(s_file);
        }

        // 3. Android Logcat
        int priority = ANDROID_LOG_INFO;
        if (strcmp(level, "WARN") == 0) priority = ANDROID_LOG_WARN;
        else if (strcmp(level, "ERROR") == 0) priority = ANDROID_LOG_ERROR;
        __android_log_print(priority, LOG_TAG, "[%s] %s", level, msg_buf);
    }
}

void init(const char* custom_log_path) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (custom_log_path && custom_log_path[0] != '\0') {
        s_path = custom_log_path;
    }
    s_file = fopen(s_path.c_str(), "a");
    if (!s_file) {
        s_path = "/sdcard/ninmods.log";
        s_file = fopen(s_path.c_str(), "a");
    }
    if (s_file) {
        fprintf(s_file, "\n--- NINMODS SESSION START [%ld] ---\n", (long)time(nullptr));
        fflush(s_file);
    }
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_entry("INFO", fmt, args);
    va_end(args);
}

void log_success(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_entry("SUCCESS", fmt, args);
    va_end(args);
}

void log_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_entry("WARN", fmt, args);
    va_end(args);
}

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_entry("ERROR", fmt, args);
    va_end(args);
}

void log_terminal(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    write_entry("TERMINAL", fmt, args);
    va_end(args);
}

void close() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (s_file) {
        fclose(s_file);
        s_file = nullptr;
    }
}

const std::string& get_log_path() {
    return s_path;
}

} // namespace logger
