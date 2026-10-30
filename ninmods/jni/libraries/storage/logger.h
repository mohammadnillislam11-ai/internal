#pragma once
#include <string>

/**
 * @file logger.h
 * @brief Thread-safe logging subsystem supporting terminal ANSI colors, disk file, and Android Logcat.
 */
namespace logger {
    void init(const char* custom_log_path = nullptr);
    void log_info(const char* fmt, ...);
    void log_success(const char* fmt, ...);
    void log_warn(const char* fmt, ...);
    void log_error(const char* fmt, ...);
    void log_terminal(const char* fmt, ...);
    void close();
    const std::string& get_log_path();
}
