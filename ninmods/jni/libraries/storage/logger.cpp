#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>
#include <mutex>
#include <deque>
#include <android/log.h>

#define LOG_TAG "NINMODS"

namespace logger {

namespace {
    static FILE* s_file = nullptr;
    static std::string s_path = "/data/local/tmp/ninmods.log";
    static std::mutex s_mutex;
    static std::deque<LogEntry> s_entries;
    static constexpr size_t MAX_LOG_ENTRIES = 200;

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
        uint32_t col = 0xFFCCCCCC; // Default light gray (ARGB / ABGR for ImGui)

        if (strcmp(level, "ERROR") == 0 || (msg_buf[0] == '[' && msg_buf[1] == '-')) {
            color_prefix = "\033[91m"; // Red
            col = 0xFF3845FF; // Red
        } else if (strcmp(level, "SUCCESS") == 0 || (msg_buf[0] == '[' && msg_buf[1] == '+')) {
            color_prefix = "\033[92m"; // Bright Green
            col = 0xFF5AD334; // Green
        } else if (strcmp(level, "WARN") == 0 || (msg_buf[0] == '[' && msg_buf[1] == '*')) {
            color_prefix = "\033[93m"; // Bright Yellow
            col = 0xFF30D0FF; // Yellow / Orange
        } else if (strcmp(level, "TERMINAL") == 0 || (msg_buf[0] == '[' && msg_buf[1] == 'C')) {
            color_prefix = "\033[96m"; // Cyan
            col = 0xFFFFB030; // Cyan
        } else {
            color_prefix = "\033[0m";
            color_suffix = "";
            col = 0xFFE0E0E0; // Soft White
        }

        printf("%s[%s] [%s] %s%s\n", color_prefix, time_str, level, msg_buf, color_suffix);
        fflush(stdout);

        // 2. Output to In-Memory Live Terminal Buffer (Zero disk footprint)
        LogEntry entry;
        entry.time_str = time_str;
        entry.level = level;
        entry.msg = msg_buf;
        entry.color = col;

        s_entries.push_back(entry);
        if (s_entries.size() > MAX_LOG_ENTRIES) {
            s_entries.pop_front();
        }

        // 3. Output to Disk file if opened
        if (s_file) {
            fprintf(s_file, "[%s] [%s] %s\n", time_str, level, msg_buf);
            fflush(s_file);
        }
    }
}

void init(const char* custom_log_path) {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!custom_log_path || custom_log_path[0] == '\0') {
        s_file = nullptr;
        return;
    }
    s_path = custom_log_path;
    s_file = fopen(s_path.c_str(), "a");
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

void get_terminal_entries(std::vector<LogEntry>& out) {
    std::lock_guard<std::mutex> lock(s_mutex);
    out.assign(s_entries.begin(), s_entries.end());
}

std::string get_all_terminal_text() {
    std::lock_guard<std::mutex> lock(s_mutex);
    std::string result;
    result.reserve(s_entries.size() * 80);
    for (const auto& e : s_entries) {
        result += "[" + e.time_str + "] [" + e.level + "] " + e.msg + "\n";
    }
    return result;
}

void clear_terminal() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_entries.clear();
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
