#include "shield_manager.hpp"
#include "Modules/AntiDebug/AntiDebug.h"
#include "Modules/AntiDump/AntiDump.h"
#include "Modules/AntiLibPatch/AntiLibPatch.h"
#include "Modules/FridaDetect/FridaDetect.h"
#include "oxorany.h"

#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/prctl.h>
#include <android/log.h>

#define SHIELD_TAG "NINMODS_SHIELD"
#define LOGS(...) __android_log_print(ANDROID_LOG_ERROR, SHIELD_TAG, __VA_ARGS__)

namespace shield {

namespace {
    static std::atomic<bool> s_running{false};
    static std::atomic<bool> s_tampered{false};
    static std::thread s_watchdog_thread;

    static AntiDebug*    s_anti_debug    = nullptr;
    static AntiDump*     s_anti_dump     = nullptr;
    static AntiLibPatch* s_anti_libpatch = nullptr;
    static FridaDetect*  s_frida_detect  = nullptr;

    void terminate_on_breach(const char* reason) {
        s_tampered.store(true);
        LOGS("[CRITICAL] Security violation detected: %s! Terminating process immediately.", reason);
        // Instant hard-kill bypassing any user-mode atexit handlers
        kill(getpid(), SIGKILL);
        _exit(1);
    }

    void on_debugger_detected() {
        terminate_on_breach("Debugger/PTrace attached");
    }

    void on_dump_detected() {
        terminate_on_breach("Memory dump attempt (/proc/self/mem)");
    }

    void on_lib_tampered(const char* libPath, uint32_t old_crc, uint32_t new_crc) {
        terminate_on_breach("Binary code section patched in memory");
    }

    void on_frida_detected() {
        terminate_on_breach("Frida/Dynamic instrumentation framework detected");
    }

    void watchdog_loop() {
        while (s_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            if (!s_running.load()) break;

            if (s_anti_debug && s_anti_debug->execute()) {
                terminate_on_breach("AntiDebug scan positive");
                break;
            }

            if (s_frida_detect && s_frida_detect->execute()) {
                terminate_on_breach("FridaDetect scan positive");
                break;
            }

            if (s_anti_dump && s_anti_dump->execute()) {
                terminate_on_breach("AntiDump scan positive");
                break;
            }

            if (s_anti_libpatch && s_anti_libpatch->execute()) {
                terminate_on_breach("AntiLibPatch code checksum mismatch");
                break;
            }
        }
    }
} // namespace

void start_watchdog() {
    if (s_running.load()) return;

    // 1. Anti-dump: prevent memory dump through /proc/<pid>/mem
    prctl(PR_SET_DUMPABLE, 0);

    // 2. Anti-tracer: attach to self via ptrace so no other debugger can attach
    ptrace(PTRACE_TRACEME, 0, 1, 0);

    // 3. Instantiate modules
    s_anti_debug    = new AntiDebug(on_debugger_detected);
    s_anti_dump     = new AntiDump(on_dump_detected);
    s_anti_libpatch = new AntiLibPatch(on_lib_tampered);
    s_frida_detect  = new FridaDetect(on_frida_detected);

    s_running.store(true);
    s_watchdog_thread = std::thread(watchdog_loop);
}

void stop_watchdog() {
    if (!s_running.load()) return;
    s_running.store(false);

    if (s_watchdog_thread.joinable()) {
        s_watchdog_thread.join();
    }

    delete s_anti_debug;    s_anti_debug = nullptr;
    delete s_anti_dump;     s_anti_dump = nullptr;
    delete s_anti_libpatch; s_anti_libpatch = nullptr;
    delete s_frida_detect;  s_frida_detect = nullptr;
}

bool verify_environment() {
    if (s_tampered.load()) return false;

    if (s_anti_debug && s_anti_debug->execute()) return false;
    if (s_frida_detect && s_frida_detect->execute()) return false;
    if (s_anti_dump && s_anti_dump->execute()) return false;
    if (s_anti_libpatch && s_anti_libpatch->execute()) return false;

    return true;
}

bool is_tampered() {
    return s_tampered.load();
}

} // namespace shield

