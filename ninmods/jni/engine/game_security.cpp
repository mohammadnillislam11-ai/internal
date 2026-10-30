#include "game_security.hpp"
#include "oxorany.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "NINMODS_SECURITY"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace engine::security {

    static bool g_security_active = false;
    static uint64_t g_anogs_base = 0;

    /**
     * @brief Locates the base address of libanogs.so from the remote process memory map.
     * @param target_pid The process ID of the running game.
     * @return 64-bit virtual memory base address, or 0 if not found.
     */
    static uint64_t find_anogs_base(int target_pid) {
        if (target_pid <= 0) return 0;

        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/maps", target_pid);

        FILE* f = fopen(path, "r");
        if (!f) return 0;

        char line[512];
        uint64_t base = 0;
        const char* target_lib = oxorany("libanogs.so");

        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, target_lib) && strstr(line, "r-xp")) {
                #if defined(__aarch64__)
                sscanf(line, "%lx-", (unsigned long*)&base);
                #else
                sscanf(line, "%x-", (unsigned int*)&base);
                #endif
                break;
            }
        }
        fclose(f);
        return base;
    }

    /**
     * @brief Passively inspects remote process security state without invoking in-process hooks.
     *        Prevents SIGSEGV crashes caused by attempting local hook installation on remote memory.
     * @param target_pid Target process ID.
     * @return true when security module state is verified, false otherwise.
     */
    bool init(int target_pid) {
        if (g_security_active && g_anogs_base != 0) return true;
        if (target_pid <= 0) return false;

        g_anogs_base = find_anogs_base(target_pid);
        if (g_anogs_base == 0) {
            LOGI("[-] libanogs.so not yet loaded in target PID %d", target_pid);
            return false;
        }

        g_security_active = true;
        LOGI("[+] Security module verified for libanogs.so base: 0x%lx (PID: %d)",
             (unsigned long)g_anogs_base, target_pid);
        return true;
    }

    /** @return true if the security inspection layer is active. */
    bool is_active() {
        return g_security_active;
    }

    /** @return 64-bit base address of remote libanogs.so. */
    uint64_t get_anogs_base() {
        return g_anogs_base;
    }

} // namespace engine::security
