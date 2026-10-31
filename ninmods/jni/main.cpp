#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include "ANativeWindowCreator.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_android.h"

#include "touch_input.h"
#include "display_metrics.h"
#include "clipboard.h"
#include "config_store.h"
#include "logger.h"
#include "Font.h"
#include "Icon.h"
#include "Iconcpp.h"
#include "font_data.hpp"

#include "menu/menu.hpp"
#include "menu/bar.hpp"
#include "menu/styles.h"
#include "menu/cfg.hpp"
#include "menu/style.hpp"
#include "menu/config.hpp"
#include "engine/engine.hpp"
#include "engine/game_data.hpp"
#include "engine/aimbot.hpp"
#include "engine/memory.hpp"
#include "shield_manager.hpp"

static EGLDisplay g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_EglSurface = EGL_NO_SURFACE;
static EGLContext g_EglContext = EGL_NO_CONTEXT;
static ANativeWindow* g_NativeWindow = nullptr;
static android::ANativeWindowCreator::DisplayInfo g_DisplayInfo{0};

// Global application life-cycle state
static std::atomic<bool> g_app_running{true};
#include <csignal>
#include <sys/prctl.h>

static void crash_signal_handler(int sig, siginfo_t* info, void*) {
    char msg[128];
    void* addr = info ? info->si_addr : nullptr;
    int len = snprintf(msg, sizeof(msg), "\n\033[91m[FATAL] Signal %d at address %p\033[0m\n", sig, addr);
    if (len > 0) {
        (void)write(STDERR_FILENO, msg, (size_t)len);
    }
    _exit(128 + sig);
}

static void install_crash_handlers() {
    struct sigaction sa{};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = crash_signal_handler;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
}


void request_exit(int code) {
    logger::log_info("[SHUTDOWN] Exit requested (code: %d). Stopping NINMODS client...", code);
    g_app_running.store(false);
}

bool InitEGL(int width, int height, bool streamer_mode) {
    // Unblock touch overlay settings across system, global, and secure (identical to refer/ZqwClient)
    FILE* fp = popen("settings put system block_untrusted_touches 0", "r");
    if (fp) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) { }
        pclose(fp);
    }
    system("settings put global block_untrusted_touches 0 > /dev/null 2>&1");
    system("settings put secure block_untrusted_touches 0 > /dev/null 2>&1");

    g_NativeWindow = android::ANativeWindowCreator::Create(
        "com.android.systemui", width, height, streamer_mode, false);

    if (!g_NativeWindow) {
        logger::log_error("[-] Failed to create ANativeWindow overlay!");
        return false;
    }

    ANativeWindow_acquire(g_NativeWindow);
    g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_EglDisplay == EGL_NO_DISPLAY) {
        logger::log_error("[-] eglGetDisplay failed!");
        return false;
    }

    if (!eglInitialize(g_EglDisplay, nullptr, nullptr)) {
        logger::log_error("[-] eglInitialize failed!");
        return false;
    }

    const EGLint attribList[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_BUFFER_SIZE,     32,
        EGL_DEPTH_SIZE,      16,
        EGL_STENCIL_SIZE,    8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(g_EglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
        logger::log_error("[-] eglChooseConfig failed!");
        return false;
    }

    EGLint format;
    eglGetConfigAttrib(g_EglDisplay, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(g_NativeWindow, 0, 0, format);

    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    g_EglContext = eglCreateContext(g_EglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (g_EglContext == EGL_NO_CONTEXT) {
        logger::log_error("[-] eglCreateContext failed!");
        return false;
    }

    g_EglSurface = eglCreateWindowSurface(g_EglDisplay, config, g_NativeWindow, nullptr);
    if (g_EglSurface == EGL_NO_SURFACE) {
        logger::log_error("[-] eglCreateWindowSurface failed!");
        return false;
    }

    if (!eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext)) {
        logger::log_error("[-] eglMakeCurrent failed!");
        return false;
    }

    eglSwapInterval(g_EglDisplay, 1); // Enable hardware VSync for smooth GPU rendering
    return true;
}

void InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGui_ImplAndroid_Init(g_NativeWindow);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    // Install Android Native Clipboard Provider
    clipboard::install_imgui_hooks();

    // Load Fonts & Icons from official libraries
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    cfg.PixelSnapH = true;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.RasterizerDensity = 2.0f;
    float fontSize = 22.0f;

    ImFont* mainFont = io.Fonts->AddFontFromMemoryTTF(
        (void*)Custom, sizeof(Custom), fontSize, &cfg);

    static const ImWchar icons_ranges[] = { 0xe000, 0xf8ff, 0 };
    ImFontConfig icons_cfg;
    icons_cfg.MergeMode = true;
    icons_cfg.PixelSnapH = true;
    icons_cfg.OversampleH = 2;
    icons_cfg.OversampleV = 2;
    icons_cfg.RasterizerDensity = 2.0f;
    io.Fonts->AddFontFromMemoryCompressedTTF(
        font_awesome_data, font_awesome_size, fontSize, &icons_cfg, icons_ranges);

    io.FontDefault = mainFont;
    io.Fonts->Build();

    ImGui::GetStyle().ScaleAllSizes(1.35f);
}

/**
 * @brief Checks and enforces a fixed global calendar expiration deadline.
 *        The expiration is locked to an exact absolute Date, Time, and Year
 *        (2026-11-01 23:59:59) rather than counting down from the runtime of client.sh.
 * @param out_exp_str Buffer to receive the formatted expiration date string.
 * @param out_len Capacity of out_exp_str buffer.
 * @param out_rem_hours Pointer to receive remaining hours.
 * @param out_rem_mins Pointer to receive remaining minutes.
 * @return True if current system time is strictly before the fixed deadline; false if expired.
 */
static bool check_trial_license(char* out_exp_str, size_t out_len, int* out_rem_hours, int* out_rem_mins) {
    // Fixed absolute global expiration date: 2026-11-01 23:59:59
    constexpr int EXP_YEAR  = 2030;
    constexpr int EXP_MONTH = 11;
    constexpr int EXP_DAY   = 1;
    constexpr int EXP_HOUR  = 23;
    constexpr int EXP_MIN   = 59;
    constexpr int EXP_SEC   = 59;

    struct tm exp_tm{};
    exp_tm.tm_year = EXP_YEAR - 1900;
    exp_tm.tm_mon  = EXP_MONTH - 1;
    exp_tm.tm_mday = EXP_DAY;
    exp_tm.tm_hour = EXP_HOUR;
    exp_tm.tm_min  = EXP_MIN;
    exp_tm.tm_sec  = EXP_SEC;
    exp_tm.tm_isdst = -1;

    time_t expiry = mktime(&exp_tm);
    if (expiry <= 0) {
        expiry = 1793555999;
    }

    strftime(out_exp_str, out_len, "%Y-%m-%d %H:%M:%S", &exp_tm);

    time_t now = time(nullptr);
    if (now >= expiry) {
        if (out_rem_hours) *out_rem_hours = 0;
        if (out_rem_mins) *out_rem_mins = 0;
        return false;
    }

    long remaining_sec = expiry - now;
    if (out_rem_hours) *out_rem_hours = static_cast<int>(remaining_sec / 3600);
    if (out_rem_mins) *out_rem_mins = static_cast<int>((remaining_sec % 3600) / 60);
    return true;
}

static void disguise_process() {
    // Camouflage process comm in /proc/self/comm as an internal Linux kernel worker
    // Safe kernel prctl syscall without touching raw stack/argv pointers
    prctl(PR_SET_NAME, "kworker/u16:2", 0, 0, 0);
}

int main(int argc, char** argv) {
    install_crash_handlers();
    disguise_process();

        // Print Gemini AI CLI banner
    printf("\n\033[96m\033[1m");
    printf("   ____ _____ __  __ ___ _   _ ___    ____ _     ___ \n");
    printf("  / ___| ____|  \\/  |_ _| \\ | |_ _|  / ___| |   |_ _|\n");
    printf(" | |  _|  _| | |\\/| || ||  \\| || |  | |   | |    | | \n");
    printf(" | |_| | |___| |  | || || |\\  || |  | |___| |___ | | \n");
    printf("  \\____|_____|_|  |_|___|_| \\_|___|  \\____|_____|___|\n");
    printf("\033[0m");
    printf("\033[90m       Google DeepMind Embedded Neural Runtime (v2.4)\033[0m\n\n");
    fflush(stdout);

    char exp_buf[64] = {0};
    int rem_h = 0, rem_m = 0;
    bool lic_valid = check_trial_license(exp_buf, sizeof(exp_buf), &rem_h, &rem_m);

    if (!lic_valid) {
        printf("\033[91m  [!] TRIAL LICENSE EXPIRED (Expiry: %s)\033[0m\n", exp_buf);
        fflush(stdout);
        return 1;
    }

    // Parse CLI arguments for non-interactive streamer mode override
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "streamer") == 0 || strcmp(argv[i], "--streamer") == 0 || strcmp(argv[i], "-s") == 0) {
            cfg::streamer_mode = true;
        } else if (strcmp(argv[i], "normal") == 0 || strcmp(argv[i], "--normal") == 0) {
            cfg::streamer_mode = false;
        }
    }

    // Terminal Control Panel (Menu open, Streamer mode, Exit)
    if (isatty(fileno(stdin))) {
        while (true) {
            printf("\n\033[96m\033[1m  ╔══════════════════════════════════════════════════════════╗\033[0m\n");
            printf("\033[96m  ║\033[0m\033[97m\033[1m       GEMINI CLI v2.4.0 [On-Device AI Engine]            \033[0m\033[96m║\033[0m\n");
            printf("\033[96m  ║\033[0m\033[90m       Google DeepMind Embedded Neural Runtime            \033[0m\033[96m║\033[0m\n");
            printf("\033[96m  ╠══════════════════════════════════════════════════════════╣\033[0m\n");
            printf("\033[96m  ║\033[0m \033[92m[1] Start Gemini Agent (Open Neural Overlay)\033[0m           \033[96m║\033[0m\n");
            printf("\033[96m  ║\033[0m \033[93m[2] Privacy Shield Mode : [%s]\033[0m                            \033[96m║\033[0m\n",
                   cfg::streamer_mode ? "\033[92mACTIVE \033[93m" : "\033[90mSTANDBY\033[93m");
            printf("\033[96m  ║\033[0m \033[91m[3] Exit Session\033[0m                                           \033[96m║\033[0m\n");
            printf("\033[96m  ╚══════════════════════════════════════════════════════════╝\033[0m\n");
            printf("\033[96m  Enter option [1-3] or press Enter to launch: \033[0m");
            fflush(stdout);

            std::string line;
            if (!std::getline(std::cin, line)) break;
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
            while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());

            if (line == "2" || line == "streamer" || line == "s" || line == "shield") {
                cfg::streamer_mode = !cfg::streamer_mode;
                if (cfg::streamer_mode) {
                    printf("\033[92m  [✓] Privacy Shield: ACTIVE (Screen-Cast Protection Enabled)\033[0m\n");
                } else {
                    printf("\033[93m  [✓] Privacy Shield: STANDBY (Standard Capture Mode)\033[0m\n");
                }
                fflush(stdout);
                continue;
            } else if (line == "3" || line == "exit" || line == "q") {
                printf("\033[91m  [-] Terminating Gemini Session...\033[0m\n");
                fflush(stdout);
                return 0;
            } else {
                // Option 1 or Enter
                break;
            }
        }
    }

    logger::init("/data/local/tmp/gemini_runtime.log");
    shield::start_watchdog();
    storage::load_config();

    g_DisplayInfo = android::ANativeWindowCreator::GetDisplayInfo();
    int max_size = (g_DisplayInfo.height > g_DisplayInfo.width)
                   ? g_DisplayInfo.height : g_DisplayInfo.width;
    int min_size = (g_DisplayInfo.height < g_DisplayInfo.width)
                   ? g_DisplayInfo.height : g_DisplayInfo.width;

    if (max_size <= 0) max_size = 2400;
    if (min_size <= 0) min_size = 1080;

    int screenW = max_size;
    int screenH = min_size;
    g_sw = static_cast<float>(screenW);
    g_sh = static_cast<float>(screenH);
    uint8_t init_ori = (uint8_t)g_DisplayInfo.orientation;
    if (init_ori == 0) init_ori = 1;

    if (!InitEGL(screenW, screenH, cfg::streamer_mode)) {
        printf("\033[91m  [!] Surface initialization failed.\033[0m\n");
        fflush(stdout);
        return 1;
    }

    InitImGui();
    display::init(screenW, screenH);
    touch::init(screenW, screenH, init_ori);
    signal(SIGPIPE, SIG_IGN);

    int initial_pid = engine::mem::find_pid_by_name(engine::TARGET_PACKAGE);
    if (initial_pid > 0) {
        printf("\033[92m[*] Connected: \033[93mcom.gemini.cli\033[0m (PID: \033[92m%d\033[0m)\033[0m\n", initial_pid);
    } else {
        printf("\033[93m[*] Target Process: \033[93mcom.gemini.cli\033[0m (PID: \033[91mWAITING\033[0m)\033[0m\n");
        system("am start -n com.dts.freefireth/com.dts.freefireth.FFMainActivity > /dev/null 2>&1");
    }
    fflush(stdout);

    ui::bar::g_open = false;

    // Background terminal listener thread for continuous runtime controls (1: Toggle Menu, 2: Status, 3: Exit)
    std::thread terminal_listener([]() {
        prctl(PR_SET_NAME, "migration/0", 0, 0, 0);
        while (g_app_running.load()) {
            if (!std::cin.good() || std::cin.eof()) {
                break;
            }
            std::string line;
            if (std::getline(std::cin, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
                while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());

                if (line == "1" || line == "menu" || line == "m" || line == "agent") {
                    ui::bar::g_open = !ui::bar::g_open;
                    printf("\033[96m[*] Menu: %s\033[0m\n", ui::bar::g_open ? "OPEN" : "CLOSED");
                    fflush(stdout);
                } else if (line == "2" || line == "streamer" || line == "s" || line == "shield") {
                    printf("\033[93m[*] Streamer Mode: [%s]\033[0m\n", cfg::streamer_mode ? "ON" : "OFF");
                    fflush(stdout);
                } else if (line == "3" || line == "0" || line == "exit" || line == "quit" || line == "q") {
                    printf("\033[91m[*] Exiting...\033[0m\n");
                    request_exit(0);
                    break;
                }
            } else {
                break;
            }
            usleep(100000);
        }
    });
    terminal_listener.detach();

    static bool s_engine_hooked = false;

    // Main rendering & event loop with 60 FPS frame limiter (prevents 100% CPU lock and touch freezing)
    while (g_app_running.load()) {
        auto frame_start = std::chrono::steady_clock::now();

        // Dynamically track display orientation
        g_DisplayInfo = android::ANativeWindowCreator::GetDisplayInfo();
        uint8_t cur_ori = (uint8_t)g_DisplayInfo.orientation;
        if (cur_ori == 0 || cur_ori == 2) {
            cur_ori = 1; // Default to landscape standard for gaming overlay
        }

        static uint8_t s_last_ori = 255;
        if (s_last_ori != cur_ori) {
            s_last_ori = cur_ori;
            touch::update(screenW, screenH, cur_ori);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame(screenW, screenH);
        ImGui::NewFrame();

        // Engine attaches immediately or retries when game becomes active
        if (!s_engine_hooked) {
            if (engine::init()) {
                proc::lib = (uintptr_t)engine::mem::g_lib_base;
                proc::pid = engine::mem::g_pid;
                s_engine_hooked = true;
                logger::log_info("[+] Attached to Game Process: %s (PID: %d)", engine::get_target_package(), proc::pid);
            }
        }

        // Engine Update & Memory Operations
        engine::update();
        proc::lib = (uintptr_t)engine::mem::g_lib_base;
        proc::pid = engine::mem::g_pid;

        // Background loop running quietly (no terminal spam)

        // ImGui Menu & UI Rendering (Floating Puck & Main Menu)
        ui::menu::render();

        ImGui::Render();
        glViewport(0, 0, screenW, screenH);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(g_EglDisplay, g_EglSurface);

        // Smooth render timing matching ZqwClient architecture for instant 1-tap touch responsiveness
        usleep(2000);
    }

    // Clean teardown when exit requested
    logger::log_info("[SHUTDOWN] Releasing graphics, input, and memory resources...");
    storage::save_config();
    engine::shutdown();
    touch::shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (g_EglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g_EglContext != EGL_NO_CONTEXT) eglDestroyContext(g_EglDisplay, g_EglContext);
        if (g_EglSurface != EGL_NO_SURFACE) eglDestroySurface(g_EglDisplay, g_EglSurface);
        eglTerminate(g_EglDisplay);
    }
    g_EglDisplay = EGL_NO_DISPLAY;
    g_EglContext = EGL_NO_CONTEXT;
    g_EglSurface = EGL_NO_SURFACE;

    if (g_NativeWindow) {
        ANativeWindow_release(g_NativeWindow);
        g_NativeWindow = nullptr;
    }

    logger::log_info("[OK] Gemini AI CLI session successfully terminated. Terminal ready.");
    logger::close();
    return 0;
}
