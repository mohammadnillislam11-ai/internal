#pragma once
#include <string>
#include <vector>
#include <cstdint>

/**
 * @file logger.h
 * @brief Thread-safe logging subsystem supporting in-memory terminal console, ANSI colors, and clipboard export.
 */
namespace logger {
    struct LogEntry {
        std::string time_str;
        std::string level;
        std::string msg;
        uint32_t color;
    };

    void init(const char* custom_log_path = nullptr);
    void log_info(const char* fmt, ...);
    void log_success(const char* fmt, ...);
    void log_warn(const char* fmt, ...);
    void log_error(const char* fmt, ...);
    void log_terminal(const char* fmt, ...);
    void close();
    const std::string& get_log_path();

    // In-memory live terminal methods (zero disk footprint)
    void get_terminal_entries(std::vector<LogEntry>& out);
    std::string get_all_terminal_text();
    void clear_terminal();

    typedef void (*LogCallback)(const char* time_str, const char* level, const char* msg, uint32_t color);
    void set_callback(LogCallback cb);
}
