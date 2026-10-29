// bypass.cpp — ShadowHook bypass libanogs.so
// NINMODS Bypass System
#include "shadowhook/shadowhook.h"
#include "memory.hpp"
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <android/log.h>

#define LOG_TAG "NinmodsBypass"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/** Security library hook offsets */
static constexpr uint64_t OFF_ANTI_FRIDA       = 0x3516DC;
static constexpr uint64_t OFF_ANTI_DEBUG       = 0x365954;
static constexpr uint64_t OFF_ANTI_MONO_MAIN   = 0x307EF0;
static constexpr uint64_t OFF_ANTI_MONO_DLOPEN = 0x308B2C;
static constexpr uint64_t OFF_SCAN_WRAPPER     = 0x419E3C;
static constexpr uint64_t OFF_SCAN_THREAD      = 0x419EE4;
static constexpr uint64_t OFF_SCAN_WRAPPER_ALT = 0x39FF60;
static constexpr uint64_t OFF_SCAN_THREAD_ALT  = 0x39FF78;
static constexpr uint64_t OFF_DETECT_CHEAT_DLL = 0x30A108;
static constexpr uint64_t OFF_DETECT_CHEAT_DLL2= 0x319CE4;
static constexpr uint64_t OFF_DETECT_CHEAT_DLL3= 0x2D7A78;
static constexpr uint64_t OFF_ANTI_MONO_DUP    = 0x318EF0;

/**
 * @brief Locates virtual base address of libanogs.so via procfs maps.
 * @return Virtual base address, or 0 if unmapped.
 */
static uint64_t find_libanogs_base() {
    if (proc::pid <= 0) return 0;

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", proc::pid);

    FILE* f = fopen(path, "r");
    if (!f) return 0;

    char line[512];
    uint64_t base = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "libanogs.so")) {
            unsigned long start = 0;
            if (sscanf(line, "%lx-", &start) == 1) {
                base = (uint64_t)start;
                break;
            }
        }
    }
    fclose(f);
    return base;
}

/** Hook intercept handlers returning neutral zero status */
static void* orig_anti_frida = nullptr;
static int hook_anti_frida(void* a1, void* a2) {
    LOGI("[hook] anti_frida called → return 0");
    return 0;
}

static void* orig_anti_debug = nullptr;
static int hook_anti_debug(void* a1, void* a2, uint32_t a3, const char* a4,
                            uint64_t* a5, int a6, int a7, int a8, int a9,
                            int a10, int a11, int a12, int a13, int a14,
                            int a15, int a16, int a17, int a18, int a19,
                            int a20, int a21, int64_t a22) {
    LOGI("[hook] anti_debug_root called → return 0");
    return 0;
}

static void* orig_anti_mono_main = nullptr;
static int hook_anti_mono_main(void* a1) {
    LOGI("[hook] anti_mono_main called → return 0");
    return 0;
}

static void* orig_anti_mono_dlopen = nullptr;
static int hook_anti_mono_dlopen(void* a1) {
    LOGI("[hook] anti_mono_dlopen called → return 1");
    return 1;  // mono OK
}

static void* orig_scan_wrapper = nullptr;
static int hook_scan_wrapper(void* a1) {
    LOGI("[hook] scan_wrapper called → return 0");
    return 0;
}

static void* orig_scan_thread = nullptr;
static int hook_scan_thread(void* a1) {
    LOGI("[hook] scan_thread called → return 0");
    return 0;
}

static void* orig_scan_wrapper_alt = nullptr;
static int hook_scan_wrapper_alt(void* a1) {
    LOGI("[hook] scan_wrapper_alt called → return 0");
    return 0;
}

static void* orig_scan_thread_alt = nullptr;
static int hook_scan_thread_alt(void* a1) {
    LOGI("[hook] scan_thread_alt called → return 0");
    return 0;
}

static void* orig_detect_cheat_dll = nullptr;
static int hook_detect_cheat_dll(void* a1, void* a2, char a3) {
    LOGI("[hook] detect_cheat_dll called → return 0");
    return 0;
}

static void* orig_detect_cheat_dll2 = nullptr;
static int hook_detect_cheat_dll2(void* a1, const char* a2) {
    LOGI("[hook] detect_cheat_dll2 called → return 0");
    return 0;
}

static void* orig_detect_cheat_dll3 = nullptr;
static int hook_detect_cheat_dll3(void* a1, void* a2, size_t a3) {
    LOGI("[hook] detect_cheat_dll3 called → return 0");
    return 0;
}

static void* orig_anti_mono_dup = nullptr;
static int hook_anti_mono_dup(void* a1) {
    LOGI("[hook] anti_mono_dup called → return 0");
    return 0;
}

/**
 * @brief Initializes ShadowHook engine and installs security handler stubs.
 */
extern "C" void init_bypass() {
    LOGI("=== NINMODS Bypass (ShadowHook) init ===");

    // Init ShadowHook
    int err = shadowhook_init(SHADOWHOOK_MODE_UNIQUE, true);
    if (err != 0) {
        LOGE("[!] shadowhook_init failed: %d", err);
        return;
    }
    LOGI("[+] ShadowHook initialized");

    // Attach ke FF
    if (proc::pid <= 0) {
        proc::pid = get_pid();
        if (proc::pid <= 0) {
            LOGE("[!] FreeFire not running");
            return;
        }
    }
    LOGI("[+] FF pid: %d", proc::pid);

    // Tunggu libanogs.so load
    uint64_t base = 0;
    for (int i = 0; i < 60; i++) {
        base = find_libanogs_base();
        if (base != 0) break;
        usleep(500000);
    }

    if (base == 0) {
        LOGE("[!] libanogs.so not found (timeout)");
        return;
    }
    LOGI("[+] libanogs.so base: 0x%lx", base);

    // ═══ HOOK pakai ShadowHook ═══
    void* h;

    h = shadowhook_hook_func_addr((void*)(base + OFF_ANTI_FRIDA),
        (void*)hook_anti_frida, (void**)&orig_anti_frida);
    LOGI("[+] anti_frida: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_ANTI_DEBUG),
        (void*)hook_anti_debug, (void**)&orig_anti_debug);
    LOGI("[+] anti_debug_root: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_ANTI_MONO_MAIN),
        (void*)hook_anti_mono_main, (void**)&orig_anti_mono_main);
    LOGI("[+] anti_mono_main: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_ANTI_MONO_DLOPEN),
        (void*)hook_anti_mono_dlopen, (void**)&orig_anti_mono_dlopen);
    LOGI("[+] anti_mono_dlopen: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_SCAN_WRAPPER),
        (void*)hook_scan_wrapper, (void**)&orig_scan_wrapper);
    LOGI("[+] scan_wrapper: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_SCAN_THREAD),
        (void*)hook_scan_thread, (void**)&orig_scan_thread);
    LOGI("[+] scan_thread: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_SCAN_WRAPPER_ALT),
        (void*)hook_scan_wrapper_alt, (void**)&orig_scan_wrapper_alt);
    LOGI("[+] scan_wrapper_alt: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_SCAN_THREAD_ALT),
        (void*)hook_scan_thread_alt, (void**)&orig_scan_thread_alt);
    LOGI("[+] scan_thread_alt: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_DETECT_CHEAT_DLL),
        (void*)hook_detect_cheat_dll, (void**)&orig_detect_cheat_dll);
    LOGI("[+] detect_cheat_dll: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_DETECT_CHEAT_DLL2),
        (void*)hook_detect_cheat_dll2, (void**)&orig_detect_cheat_dll2);
    LOGI("[+] detect_cheat_dll2: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_DETECT_CHEAT_DLL3),
        (void*)hook_detect_cheat_dll3, (void**)&orig_detect_cheat_dll3);
    LOGI("[+] detect_cheat_dll3: %s", h ? "OK" : "FAIL");

    h = shadowhook_hook_func_addr((void*)(base + OFF_ANTI_MONO_DUP),
        (void*)hook_anti_mono_dup, (void**)&orig_anti_mono_dup);
    LOGI("[+] anti_mono_dup: %s", h ? "OK" : "FAIL");

    LOGI("=== NINMODS Bypass done ===");
}