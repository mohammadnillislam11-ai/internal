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

void request_exit(int code) {
    logger::log_info("[SHUTDOWN] Exit requested (code: %d). Stopping NINMODS client...", code);
    g_app_running.store(false);
}

bool InitEGL(int width, int height) {
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
        "com.android.systemui", width, height, true, false);

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

int main(int argc, char** argv) {
    std::cout << R"(
 _   _  ___  _   _ __  __  ___   ___  ____  
| \ | ||_ _|| \ | |  \/  |/ _ \ / _ \/ ___| 
|  \| | | | |  \| | |\/| | | | | | | \___ \ 
| |\  | | | | |\  | |  | | |_| | |_| |___) |
|_| \_||___||_| \_|_|  |_|\___/ \___/|____/ 
)" << std::endl;

    // 0. Initialize persistent logging system (terminal + /data/local/tmp/ninmods.log)
    logger::init("/data/local/tmp/ninmods.log");
    logger::log_info("==================================================");
    logger::log_info("NINMODS Client 2.0 (Multi-ABI ARM64 / ARM32)");
    logger::log_info("Log destination: %s", logger::get_log_path().c_str());
    logger::log_info("Mode: OPEN CLIENT (Zero License Restriction)");
    logger::log_info("==================================================");

    // 1. Initialize native security shield (AntiDebug, AntiDump, AntiLibPatch, FridaDetect)
    shield::start_watchdog();
    logger::log_info("[+] Native security watchdog active.");

    // 2. Load stored user settings (ESP, Aim settings)
    storage::load_config();

    // 3. Start background terminal command listener for exit commands ('0' or 'exit')
    std::thread terminal_listener([]() {
        while (g_app_running.load()) {
            if (!std::cin.good() || std::cin.eof()) {
                break;
            }
            std::string line;
            if (std::getline(std::cin, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
                while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());
                if (line == "0" || line == "exit" || line == "quit" || line == "q") {
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

    if (!InitEGL(screenW, screenH)) {
        logger::log_error("[-] EGL initialization failed!");
        return 1;
    }
    logger::log_info("[+] EGL Surface initialized: %d x %d (Orientation: %d)", screenW, screenH, (int)init_ori);

    InitImGui();
    display::init(screenW, screenH);
    touch::init(screenW, screenH, init_ori);

    // Check target game process status at startup (Strictly com.dts.freefireth)
    int initial_pid = engine::mem::find_pid_by_name(engine::TARGET_PACKAGE);
    if (initial_pid > 0) {
        logger::log_info("[+] Target Game Process Detected: %s (PID: %d)", engine::TARGET_PACKAGE, initial_pid);
    } else {
        logger::log_info("[*] Target Game Process: Monitoring for %s (Background Running)...", engine::TARGET_PACKAGE);
    }

    logger::log_info("[+] Target Package: %s (%s)", engine::TARGET_PACKAGE, engine::TARGET_REGION);
    logger::log_info("[+] Multi-ABI Engine: arm64-v8a & armeabi-v7a");
    logger::log_info("[+] NativeShield Engine: ACTIVE (AntiDebug, AntiDump, CRC32 Checksum)");
#if defined(__aarch64__)
    logger::log_info("[+] Protection Framework: DobbyHook (ARM64) & ByteDance ShadowHook ACTIVE");
#else
    logger::log_info("[+] Protection Framework: DobbyHook (ARM32) ACTIVE");
#endif
    logger::log_info("[+] Display Resolution: %d x %d (Orientation: %d)", screenW, screenH, (g_DisplayInfo.orientation != 0) ? g_DisplayInfo.orientation : 1);
    logger::log_info("[+] Terminal & Menu Control: Type '0' or 'exit' in terminal, or click EXIT in menu to close.");

    static bool s_engine_hooked = false;

    // Main rendering & event loop with 60 FPS frame limiter (prevents 100% CPU lock and touch freezing)
    while (g_app_running.load()) {
        auto frame_start = std::chrono::steady_clock::now();

        // Dynamically track display orientation
        g_DisplayInfo = android::ANativeWindowCreator::GetDisplayInfo();
        uint8_t cur_ori = (uint8_t)g_DisplayInfo.orientation;

        static uint8_t s_last_ori = 255;
        if (s_last_ori != cur_ori) {
            s_last_ori = cur_ori;
            touch::update(screenW, screenH, cur_ori);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame(screenW, screenH);
        ImGui::NewFrame();

        // Engine attaches immediately on start
        if (!s_engine_hooked) {
            logger::log_info("[+] Attaching Engine to Game Process (%s)...", engine::TARGET_PACKAGE);
            engine::init();
            proc::lib = (uintptr_t)engine::mem::g_lib_base;
            proc::pid = engine::mem::g_pid;
            s_engine_hooked = true;
        }

        // Engine Update & Memory Operations
        engine::update();
        proc::lib = (uintptr_t)engine::mem::g_lib_base;
        proc::pid = engine::mem::g_pid;

        // ─── DEBUG LOG TIAP 5 DETIK (Exact ZqwClient format) ───
        static int dbgCounter = 0;
        if (++dbgCounter >= 300) {  // ~5 detik
            dbgCounter = 0;

            size_t nPlayers = 0;
            {
                std::lock_guard<std::mutex> lk(engine::data::g_players_mutex);
                nPlayers = engine::data::g_players.size();
            }

            printf("\033[96m[DBG] \033[0m"
                   "inMatch=%d | players=%zu | aim=%d | nobita=%d\n",
                   (int)engine::data::g_in_match.load(std::memory_order_relaxed),
                   nPlayers,
                   (int)cfg::aim::silent,
                   (int)cfg::aim::nobita);
            fflush(stdout);
        }

        // ImGui Menu & UI Rendering (Floating Puck & Main Menu)
        ui::menu::render();

        ImGui::Render();
        glViewport(0, 0, screenW, screenH);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(g_EglDisplay, g_EglSurface);

        // Frame rate limiter: cap at 60 FPS (~16666 us) to ensure smooth input responsiveness and 0% phone freeze
        auto frame_end = std::chrono::steady_clock::now();
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start).count();
        constexpr int64_t TARGET_FRAME_TIME_US = 16666;
        if (elapsed_us < TARGET_FRAME_TIME_US) {
            usleep(static_cast<useconds_t>(TARGET_FRAME_TIME_US - elapsed_us));
        }
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
        android::ANativeWindowCreator::Destroy(g_NativeWindow);
        g_NativeWindow = nullptr;
    }

    logger::log_info("[OK] NINMODS client successfully terminated. Terminal ready.");
    logger::close();
    return 0;
}
