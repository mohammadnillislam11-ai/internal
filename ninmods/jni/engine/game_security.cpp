#include "game_security.hpp"
#include "dobby.h"
#if defined(__aarch64__)
#include "shadowhook.h"
#endif
#include "oxorany.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "NINMODS_SECURITY"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace engine::security {

    static bool g_security_active = false;
    static uint64_t g_anogs_base = 0;

    // ─────────────────────────────────────────────────────────────
    // Hook Originals & Stubs (Standardized Return Handlers)
    // ─────────────────────────────────────────────────────────────
    static void* orig_anti_frida = nullptr;
    static int hook_anti_frida(void* a1, void* a2) {
        LOGI("[+] security: anti_frida neutralized -> return 0");
        return 0;
    }

    static void* orig_anti_debug = nullptr;
    static int hook_anti_debug(void* a1, void* a2, uint32_t a3, const char* a4,
                               uint64_t* a5, int a6, int a7, int a8, int a9,
                               int a10, int a11, int a12, int a13, int a14,
                               int a15, int a16, int a17, int a18, int a19,
                               int a20, int a21, int64_t a22) {
        LOGI("[+] security: anti_debug neutralized -> return 0");
        return 0;
    }

    static void* orig_anti_mono_main = nullptr;
    static int hook_anti_mono_main(void* a1) {
        return 0;
    }

    static void* orig_anti_mono_dlopen = nullptr;
    static int hook_anti_mono_dlopen(void* a1) {
        return 1; // Mono load permitted
    }

    static void* orig_scan_wrapper = nullptr;
    static int hook_scan_wrapper(void* a1) {
        return 0;
    }

    static void* orig_scan_thread = nullptr;
    static int hook_scan_thread(void* a1) {
        return 0;
    }

    static void* orig_scan_wrapper_alt = nullptr;
    static int hook_scan_wrapper_alt(void* a1) {
        return 0;
    }

    static void* orig_scan_thread_alt = nullptr;
    static int hook_scan_thread_alt(void* a1) {
        return 0;
    }

    static void* orig_detect_cheat_dll = nullptr;
    static int hook_detect_cheat_dll(void* a1, void* a2, char a3) {
        return 0;
    }

    static void* orig_detect_cheat_dll2 = nullptr;
    static int hook_detect_cheat_dll2(void* a1, const char* a2) {
        return 0;
    }

    static void* orig_detect_cheat_dll3 = nullptr;
    static int hook_detect_cheat_dll3(void* a1, void* a2, size_t a3) {
        return 0;
    }

    static void* orig_anti_mono_dup = nullptr;
    static int hook_anti_mono_dup(void* a1) {
        return 0;
    }

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

    bool init(int target_pid) {
        if (g_security_active) return true;
        if (target_pid <= 0) return false;

        g_anogs_base = find_anogs_base(target_pid);
        if (g_anogs_base == 0) {
            LOGI("[-] libanogs.so not yet loaded in target");
            return false;
        }

        // Initialize ShadowHook (ARM64) & Dobby Hook Frameworks
#if defined(__aarch64__)
        shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false);
#endif

        // Install hooks across all 12 protection offsets via official Dobby hook framework
        DobbyHook((void*)(g_anogs_base + OFF_ANTI_FRIDA),
            (void*)hook_anti_frida, (void**)&orig_anti_frida);

        DobbyHook((void*)(g_anogs_base + OFF_ANTI_DEBUG),
            (void*)hook_anti_debug, (void**)&orig_anti_debug);

        DobbyHook((void*)(g_anogs_base + OFF_ANTI_MONO_MAIN),
            (void*)hook_anti_mono_main, (void**)&orig_anti_mono_main);

        DobbyHook((void*)(g_anogs_base + OFF_ANTI_MONO_DLOPEN),
            (void*)hook_anti_mono_dlopen, (void**)&orig_anti_mono_dlopen);

        DobbyHook((void*)(g_anogs_base + OFF_SCAN_WRAPPER),
            (void*)hook_scan_wrapper, (void**)&orig_scan_wrapper);

        DobbyHook((void*)(g_anogs_base + OFF_SCAN_THREAD),
            (void*)hook_scan_thread, (void**)&orig_scan_thread);

        DobbyHook((void*)(g_anogs_base + OFF_SCAN_WRAPPER_ALT),
            (void*)hook_scan_wrapper_alt, (void**)&orig_scan_wrapper_alt);

        DobbyHook((void*)(g_anogs_base + OFF_SCAN_THREAD_ALT),
            (void*)hook_scan_thread_alt, (void**)&orig_scan_thread_alt);

        DobbyHook((void*)(g_anogs_base + OFF_DETECT_CHEAT_DLL),
            (void*)hook_detect_cheat_dll, (void**)&orig_detect_cheat_dll);

        DobbyHook((void*)(g_anogs_base + OFF_DETECT_CHEAT_DLL2),
            (void*)hook_detect_cheat_dll2, (void**)&orig_detect_cheat_dll2);

        DobbyHook((void*)(g_anogs_base + OFF_DETECT_CHEAT_DLL3),
            (void*)hook_detect_cheat_dll3, (void**)&orig_detect_cheat_dll3);

        DobbyHook((void*)(g_anogs_base + OFF_ANTI_MONO_DUP),
            (void*)hook_anti_mono_dup, (void**)&orig_anti_mono_dup);

        g_security_active = true;
        LOGI("[+] All 12 security offsets hooked successfully on libanogs.so base: 0x%lx", (unsigned long)g_anogs_base);
        return true;
    }

    bool is_active() {
        return g_security_active;
    }

    uint64_t get_anogs_base() {
        return g_anogs_base;
    }

} // namespace engine::security

