#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
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
#include "libraries/network/license_client.h"
#include "Font.h"
#include "Icon.h"
#include "Iconcpp.h"
#include "font_data.hpp"

#include "menu/menu.hpp"
#include "menu/license_view.h"
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
    // Unblock touch overlay settings across system, global, and secure
    system("settings put global block_untrusted_touches 0 > /dev/null 2>&1");
    system("settings put system block_untrusted_touches 0 > /dev/null 2>&1");
    system("settings put secure block_untrusted_touches 0 > /dev/null 2>&1");

    g_NativeWindow = android::ANativeWindowCreator::Create(
        "com.android.systemui", width, height, false, false);

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

    ui::theme::ApplyTheme();

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
    logger::log_info("==================================================");

    // 1. Start background terminal command listener (Real-time live typing from Gboard/terminal into overlay)
    std::thread terminal_listener([]() {
        struct termios orig_termios, raw_termios;
        bool is_tty = (isatty(STDIN_FILENO) != 0);
        if (is_tty) {
            tcgetattr(STDIN_FILENO, &orig_termios);
            raw_termios = orig_termios;
            raw_termios.c_lflag &= ~(ICANON | ECHO);
            raw_termios.c_cc[VMIN] = 0;
            raw_termios.c_cc[VTIME] = 1; // 100ms timeout
            tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
        }

        std::string live_buffer = "";
        char ch = 0;

        while (g_app_running.load()) {
            if (!ui::license::g_licensed) {
                // Real-time character-by-character live typing mode
                ssize_t n = read(STDIN_FILENO, &ch, 1);
                if (n > 0) {
                    if (ch == '\r' || ch == '\n') {
                        // Enter pressed on keyboard
                        std::string key = live_buffer.empty() ? ui::license::get_key() : live_buffer;
                        while (!key.empty() && (key.front() == ' ' || key.front() == '\t')) key.erase(key.begin());
                        while (!key.empty() && (key.back() == ' ' || key.back() == '\r' || key.back() == '\n')) key.pop_back();

                        if (key == "0" || key == "exit" || key == "quit" || key == "q") {
                            request_exit(0);
                            break;
                        }

                        if (!key.empty()) {
                            ui::license::set_key(key);
                            logger::log_info("[TERMINAL-KEY] Submitting: '%s'", key.c_str());
                            auto result = auth::verify_license_online(key);
                            if (result.success) {
                                ui::license::g_licensed = true;
                                ui::menu::g_open = false; // Floating logo opens FIRST
                                ui::license::g_expiration_date = result.expiration_date;
                                ui::license::g_days_left = result.days_left;
                                ui::license::g_exp_year = result.exp_year;
                                ui::license::g_exp_month = result.exp_month;
                                ui::license::g_exp_day = result.exp_day;
                                storage::set_saved_license(key);
                                logger::log_info("[TERMINAL-KEY] [OK] LICENSE ACTIVATED SUCCESSFULLY!");
                                auth::start_heartbeat_watchdog(key);
                                live_buffer.clear();
                            } else {
                                logger::log_warn("[TERMINAL-KEY] [FAIL] Rejected: %s", result.message.c_str());
                            }
                        }
                    } else if (ch == 127 || ch == 8) {
                        // Backspace pressed on keyboard
                        if (!live_buffer.empty()) {
                            live_buffer.pop_back();
                            ui::license::set_key(live_buffer);
                        }
                    } else if (ch >= 32 && ch <= 126) {
                        // Printable ASCII character typed on keyboard
                        if (live_buffer.length() < 120) {
                            live_buffer.push_back(ch);
                            ui::license::set_key(live_buffer); // INSTANT ON-SCREEN VISUAL UPDATE!
                        }
                    }
                } else {
                    usleep(15000);
                }
            } else {
                // Once licensed, restore canonical terminal or read normal commands (0 / exit)
                if (is_tty) {
                    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
                    is_tty = false;
                }
                std::string line;
                if (std::getline(std::cin, line)) {
                    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) line.pop_back();
                    while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.erase(line.begin());
                    if (line == "0" || line == "exit" || line == "quit" || line == "q") {
                        request_exit(0);
                        break;
                    }
                }
                usleep(50000);
            }
        }

        if (is_tty) {
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        }
    });
    terminal_listener.detach();

    // 2. Initialize native security shield (AntiDebug, AntiDump, AntiLibPatch, FridaDetect)
    shield::start_watchdog();
    logger::log_info("[+] Native security watchdog active.");

    // 3. Early VIP License Auto-Verification from saved config or CLI arguments
    storage::load_config();
    std::string active_key = storage::get_saved_license();

    // Check CLI argument first: ./client.sh <key>
    if (active_key.empty() && argc > 1 && argv[1] != nullptr && strlen(argv[1]) >= 2) {
        std::string cli_arg = argv[1];
        while (!cli_arg.empty() && (cli_arg.back() == '\r' || cli_arg.back() == '\n' || cli_arg.back() == ' ')) cli_arg.pop_back();
        while (!cli_arg.empty() && cli_arg.front() == ' ') cli_arg.erase(cli_arg.begin());
        if (cli_arg != "0" && cli_arg != "exit") {
            active_key = cli_arg;
            logger::log_info("[LICENSE] Detected key from CLI argument: %s", active_key.c_str());
        }
    }

    if (!active_key.empty()) {
        logger::log_info("[LICENSE] Checking license: %s ...", active_key.c_str());
        auto auth_res = auth::verify_license_online(active_key);
        if (auth_res.success) {
            ui::license::g_licensed = true;
            ui::menu::g_open = false; // Floating logo opens FIRST
            ui::license::g_expiration_date = auth_res.expiration_date;
            ui::license::g_days_left = auth_res.days_left;
            ui::license::g_exp_year = auth_res.exp_year;
            ui::license::g_exp_month = auth_res.exp_month;
            ui::license::g_exp_day = auth_res.exp_day;
            storage::set_saved_license(active_key);
            logger::log_info("[LICENSE] [OK] License VALID! Expiry: %s (%d days left)",
                             auth_res.expiration_date.c_str(), auth_res.days_left);
            auth::start_heartbeat_watchdog(active_key);
        } else {
            logger::log_warn("[LICENSE] Key is invalid or expired: %s", auth_res.message.c_str());
        }
    } else {
        // Pre-fill key from /sdcard/key.txt or clipboard if available
        std::string auto_clip = clipboard::get_text();
        if (!auto_clip.empty() && auto_clip.length() >= 2 && auto_clip.length() <= 64) {
            ui::license::set_key(auto_clip);
            logger::log_info("[LICENSE] Pre-filled license from clipboard/file: %s", auto_clip.c_str());
        }
    }

    g_DisplayInfo = android::ANativeWindowCreator::GetDisplayInfo();
    int screenW = (g_DisplayInfo.width > 0) ? g_DisplayInfo.width : 2400;
    int screenH = (g_DisplayInfo.height > 0) ? g_DisplayInfo.height : 1080;
    uint8_t init_ori = (uint8_t)g_DisplayInfo.orientation;

    if (!InitEGL(screenW, screenH)) {
        logger::log_error("[-] EGL initialization failed!");
        return 1;
    }
    logger::log_info("[+] EGL Surface initialized: %d x %d (Orientation: %d)", screenW, screenH, (int)init_ori);

    InitImGui();
    display::init(screenW, screenH);
    touch::init(screenW, screenH, init_ori);

    // Check target game process status at startup
    int initial_pid = engine::mem::find_pid_by_name(engine::TARGET_PACKAGE);
    if (initial_pid > 0) {
        logger::log_info("[+] Target Game Process Detected: %s (PID: %d)", engine::TARGET_PACKAGE, initial_pid);
    } else {
        logger::log_info("[*] Target Game Process: Monitoring for %s (Background Running)...", engine::TARGET_PACKAGE);
    }

    logger::log_info("[+] Target Package: %s (%s)", engine::get_target_package(), engine::get_target_region());
    logger::log_info("[+] Multi-ABI Engine: arm64-v8a & armeabi-v7a");
    logger::log_info("[+] NativeShield Engine: ACTIVE (AntiDebug, AntiDump, CRC32 Checksum)");
#if defined(__aarch64__)
    logger::log_info("[+] Protection Framework: DobbyHook (ARM64) & ByteDance ShadowHook ACTIVE");
#else
    logger::log_info("[+] Protection Framework: DobbyHook (ARM32) ACTIVE");
#endif
    logger::log_info("[+] Display Resolution: %d x %d (Orientation: %d)", screenW, screenH, (g_DisplayInfo.orientation != 0) ? g_DisplayInfo.orientation : 1);
    logger::log_info("[+] Terminal & Menu Control: Type '0' or 'exit' in terminal, or click EXIT in menu to close.");
    logger::log_info("[*] Awaiting VIP License: Enter key in terminal or on-screen prompt...");

    static bool s_engine_hooked = false;

    // Main rendering & event loop with 60 FPS frame limiter (prevents 100% CPU lock and touch freezing)
    while (g_app_running.load()) {
        auto frame_start = std::chrono::steady_clock::now();

        // Dynamically track display orientation and dimensions every frame
        g_DisplayInfo = android::ANativeWindowCreator::GetDisplayInfo();
        int cur_w = (g_DisplayInfo.width > 0) ? g_DisplayInfo.width : screenW;
        int cur_h = (g_DisplayInfo.height > 0) ? g_DisplayInfo.height : screenH;
        uint8_t cur_ori = (uint8_t)g_DisplayInfo.orientation;

        static uint8_t s_last_ori = 255;
        if (s_last_ori != cur_ori) {
            s_last_ori = cur_ori;
            touch::update(cur_w, cur_h, cur_ori);
        }

        touch::poll();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame(cur_w, cur_h);
        ImGui::NewFrame();

        // Stepwise License Verification Gate
        // Engine only attaches and runs after license key is verified
        if (ui::license::g_licensed) {
            if (!s_engine_hooked) {
                logger::log_info("[OK] VIP License Validated! Attaching Engine to Game Process...");
                if (!ui::license::g_expiration_date.empty()) {
                    if (ui::license::g_exp_year > 0) {
                        logger::log_info("[+] License Expiry: %s (Month: %02d | Year: %04d | Days Left: %d)",
                                         ui::license::g_expiration_date.c_str(),
                                         ui::license::g_exp_month,
                                         ui::license::g_exp_year,
                                         ui::license::g_days_left);
                    } else {
                        logger::log_info("[+] License Expiry: %s", ui::license::g_expiration_date.c_str());
                    }
                }
                engine::init();
                s_engine_hooked = true;
            }

            // Engine Update & Feature Slots
            engine::update();
            engine::render_esp();

            // Periodic Live Terminal Game Status (Every ~3 seconds at 60 FPS = 180 frames)
            static int s_info_tick = 0;
            if (++s_info_tick >= 180) {
                s_info_tick = 0;
                size_t entity_count = 0;
                {
                    std::lock_guard<std::mutex> lk(engine::data::g_players_mutex);
                    entity_count = engine::data::g_players.size();
                }
                bool in_match = engine::data::g_in_match.load(std::memory_order_relaxed);
                uint64_t target_addr = engine::aim::get_current_target();
                logger::log_info("[GAME-INFO] PID: %d | Status: %s | Entities: %zu | Aim Circle: %.0fpx | Target: %s | FPS: %.1f",
                                 engine::mem::g_pid,
                                 (engine::mem::g_pid <= 0) ? "DORMANT" : (in_match ? "IN_MATCH" : "LOBBY"),
                                 entity_count,
                                 cfg::aim::fov,
                                 (target_addr != 0) ? "LOCKED" : "IDLE",
                                 ImGui::GetIO().Framerate);
            }
        }

        // ImGui Menu & UI Rendering
        // Displays License Modal when unlicensed; Draggable Floating Puck & Main Menu when licensed
        ui::menu::render();

        ImGui::Render();
        glViewport(0, 0, cur_w, cur_h);
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
    auth::stop_heartbeat_watchdog();
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
