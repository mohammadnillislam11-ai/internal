#pragma once
#include <cstdint>

/**
 * @file game_security.hpp
 * @brief libanogs.so Security Module utilizing official Dobby & ShadowHook libraries.
 * Replaces custom patch code with standard library-driven function hooks.
 */

namespace engine::security {

    // libanogs.so Security Offsets
    constexpr uint64_t OFF_ANTI_FRIDA        = 0x3516DC;
    constexpr uint64_t OFF_ANTI_DEBUG        = 0x365954;
    constexpr uint64_t OFF_ANTI_MONO_MAIN    = 0x307EF0;
    constexpr uint64_t OFF_ANTI_MONO_DLOPEN  = 0x308B2C;
    constexpr uint64_t OFF_SCAN_WRAPPER      = 0x419E3C;
    constexpr uint64_t OFF_SCAN_THREAD       = 0x419EE4;
    constexpr uint64_t OFF_SCAN_WRAPPER_ALT  = 0x39FF60;
    constexpr uint64_t OFF_SCAN_THREAD_ALT   = 0x39FF78;
    constexpr uint64_t OFF_DETECT_CHEAT_DLL  = 0x30A108;
    constexpr uint64_t OFF_DETECT_CHEAT_DLL2 = 0x319CE4;
    constexpr uint64_t OFF_DETECT_CHEAT_DLL3 = 0x2D7A78;
    constexpr uint64_t OFF_ANTI_MONO_DUP     = 0x318EF0;

    bool init(int target_pid);
    bool is_active();
    uint64_t get_anogs_base();

} // namespace engine::security

