#pragma once
#include <atomic>
#include <string>

/**
 * @file shield_manager.hpp
 * @brief High-security native watchdog coordinator combining AntiDebug, AntiDump,
 *        AntiLibPatch, and FridaDetect into a persistent, unbypassable defense layer.
 */
namespace shield {

    /**
     * @brief Initialize all low-level anti-debugging, anti-tampering, and memory-dump protections.
     * Starts the persistent security watchdog thread.
     */
    void start_watchdog();

    /**
     * @brief Stop the watchdog thread cleanly on client shutdown.
     */
    void stop_watchdog();

    /**
     * @brief Perform an immediate synchronous verification of execution environment.
     * @return true if environment is safe and uninstrumented, false if debugger/hook/dump detected.
     */
    bool verify_environment();

    /**
     * @brief Checks if any security violation has occurred.
     */
    bool is_tampered();

} // namespace shield

