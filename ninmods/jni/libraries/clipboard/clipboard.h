#pragma once
#include <string>

/**
 * @file clipboard.h
 * @brief Android native clipboard integration for Dear ImGui.
 */
namespace clipboard {

    std::string get_text();
    void set_text(const std::string& text);

    void install_imgui_hooks();

} // namespace clipboard
