#pragma once
#include <string>

namespace logger {
    void init(const char* custom_log_path = nullptr);
    void log_info(const char* fmt, ...);
    void log_warn(const char* fmt, ...);
    void log_error(const char* fmt, ...);
    void log_terminal(const char* fmt, ...);
    void close();
    const std::string& get_log_path();
}
