#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
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

#define LOG_TAG "NINMODS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static EGLDisplay g_EglDisplay = EGL_NO_DISPLAY;
static EGLSurface g_EglSurface = EGL_NO_SURFACE;
static EGLContext g_EglContext = EGL_NO_CONTEXT;
static ANativeWindow* g_NativeWindow = nullptr;
static android::ANativeWindowCreator::DisplayInfo g_DisplayInfo{0};

bool InitEGL(int width, int height) {
    // Unblock touch overlay settings
    system("settings put global block_untrusted_touches 0 > /dev/null 2>&1");

    g_NativeWindow = android::ANativeWindowCreator::Create(
        "NINMODS_Overlay", width, height, false, false);

    if (!g_NativeWindow) {
        LOGI("[-] Failed to create ANativeWindow");
        return false;
    }

    ANativeWindow_acquire(g_NativeWindow);
    g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_EglDisplay == EGL_NO_DISPLAY) return false;

    if (!eglInitialize(g_EglDisplay, nullptr, nullptr)) return false;

    const EGLint attribList[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_NONE
    };

    EGLConfig config;
    EGLint numConfigs;
    if (!eglChooseConfig(g_EglDisplay, attribList, &config, 1, &numConfigs) || numConfigs <= 0)
        return false;

    EGLint format;
    eglGetConfigAttrib(g_EglDisplay, config, EGL_NATIVE_VISUAL_ID, &format);
    ANativeWindow_setBuffersGeometry(g_NativeWindow, 0, 0, format);

    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    g_EglContext = eglCreateContext(g_EglDisplay, config, EGL_NO_CONTEXT, contextAttribs);
    if (g_EglContext == EGL_NO_CONTEXT) return false;

    g_EglSurface = eglCreateWindowSurface(g_EglDisplay, config, g_NativeWindow, nullptr);
    if (g_EglSurface == EGL_NO_SURFACE) return false;

    if (!eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext))
        return false;

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
    float fontSize = 20.0f;

    ImFont* mainFont = io.Fonts->AddFontFromMemoryTTF(
        (void*)Custom, sizeof(Custom), fontSize, &cfg);

    static const ImWchar icons_ranges[] = { 0xe000, 0xf8ff, 0 };
    ImFontConfig icons_cfg;
    icons_cfg.MergeMode = true;
    icons_cfg.PixelSnapH = true;
    icons_cfg.OversampleH = 2;
    icons_cfg.OversampleV = 2;
    io.Fonts->AddFontFromMemoryCompressedTTF(
        font_awesome_data, font_awesome_size, fontSize, &icons_cfg, icons_ranges);

    io.FontDefault = mainFont;
    io.Fonts->Build();

    ImGui::GetStyle().ScaleAllSizes(1.25f);
}

int main(int argc, char** argv) {
    std::cout << R"(
███╗   ██╗██╗███╗   ██╗███╗   ███╗ ██████╗ ██████╗ ███████╗
████╗  ██║██║████╗  ██║████╗ ████║██╔═══██╗██╔══██╗██╔════╝
██╔██╗ ██║██║██╔██╗ ██║██╔████╔██║██║   ██║██║  ██║███████╗
██║╚██╗██║██║██║╚██╗██║██║╚██╔╝██║██║   ██║██║  ██║╚════██║
██║ ╚████║██║██║ ╚████║██║ ╚═╝ ██║╚██████╔╝██████╔╝███████║
╚═╝  ╚═══╝╚═╝╚═╝  ╚═══╝╚═╝     ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝
)" << '\n';

    printf("[+] Starting ninmods standalone client...\n");

    // Initialize native security shield (AntiDebug, AntiDump, AntiLibPatch, FridaDetect)
    shield::start_watchdog();
    printf("[+] Native security watchdog active.\n");

    g_DisplayInfo = android::ANativeWindowCreator::GetDisplayInfo();
    int screenW = (g_DisplayInfo.width > 0) ? g_DisplayInfo.width : 2400;
    int screenH = (g_DisplayInfo.height > 0) ? g_DisplayInfo.height : 1080;

    if (!InitEGL(screenW, screenH)) {
        printf("[-] EGL initialization failed!\n");
        return 1;
    }

    InitImGui();
    display::init(screenW, screenH);
    storage::load_config();
    touch::init(screenW, screenH, (g_DisplayInfo.orientation != 0) ? g_DisplayInfo.orientation : 1);

    // Check target game process status at terminal startup
    int initial_pid = engine::mem::find_pid_by_name(engine::TARGET_PACKAGE);
    if (initial_pid > 0) {
        printf("[+] Target Game Process Detected: %s (PID: %d)\n", engine::TARGET_PACKAGE, initial_pid);
    } else {
        printf("[*] Target Game Process: Monitoring for %s (Background Running)...\n", engine::TARGET_PACKAGE);
    }

    printf("[+] Target Package: %s (%s)\n", engine::get_target_package(), engine::get_target_region());
    printf("[+] Multi-ABI Engine: arm64-v8a & armeabi-v7a\n");
    printf("[+] NativeShield Engine: ACTIVE (AntiDebug, AntiDump, CRC32 Checksum)\n");
    printf("[+] Protection Framework: DobbyHook & ByteDance ShadowHook ACTIVE\n");
    printf("[+] Resolution: %d x %d (Orientation: %d)\n", screenW, screenH, (g_DisplayInfo.orientation != 0) ? g_DisplayInfo.orientation : 1);
    printf("[OK] Awaiting VIP License Activation before game engine attach...\n");

    static bool s_engine_hooked = false;

    while (true) {
        touch::poll();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplAndroid_NewFrame(screenW, screenH);
        ImGui::NewFrame();

        // 1. Stepwise License Verification Gate
        // Engine only attaches and runs after license key is verified
        if (ui::license::g_licensed) {
            if (!s_engine_hooked) {
                printf("[OK] VIP License Validated! Attaching Engine to Game Process...\n");
                if (!ui::license::g_expiration_date.empty()) {
                    if (ui::license::g_exp_year > 0) {
                        printf("[+] License Expiry: %s (Month: %02d | Year: %04d | Days Left: %d)\n",
                               ui::license::g_expiration_date.c_str(),
                               ui::license::g_exp_month,
                               ui::license::g_exp_year,
                               ui::license::g_days_left);
                    } else {
                        printf("[+] License Expiry: %s\n", ui::license::g_expiration_date.c_str());
                    }
                }
                engine::init();
                s_engine_hooked = true;
            }

            // Engine Update & Feature Slots
            engine::update();
            engine::render_esp();

            // Periodic Live Terminal Game Status (Every ~3 seconds)
            static int s_info_tick = 0;
            if (++s_info_tick >= 1500) {
                s_info_tick = 0;
                size_t entity_count = 0;
                {
                    std::lock_guard<std::mutex> lk(engine::data::g_players_mutex);
                    entity_count = engine::data::g_players.size();
                }
                bool in_match = engine::data::g_in_match.load(std::memory_order_relaxed);
                uint64_t target_addr = engine::aim::get_current_target();
                printf("[GAME-INFO] PID: %d | Status: %s | Entities: %zu | Aim Circle: %.0fpx | Target: %s | FPS: %.1f\n",
                       engine::mem::g_pid,
                       (engine::mem::g_pid <= 0) ? "DORMANT" : (in_match ? "IN_MATCH" : "LOBBY"),
                       entity_count,
                       cfg::aim::fov,
                       (target_addr != 0) ? "LOCKED" : "IDLE",
                       ImGui::GetIO().Framerate);
                fflush(stdout);
            }
        }

        // 2. ImGui Menu & UI Rendering
        // Displays License Modal when unlicensed; Draggable Floating Puck & Main Menu when licensed
        ui::menu::render();

        ImGui::Render();
        glViewport(0, 0, screenW, screenH);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        eglSwapBuffers(g_EglDisplay, g_EglSurface);

        usleep(2000);
    }

    storage::save_config();
    auth::stop_heartbeat_watchdog();
    engine::shutdown();
    touch::shutdown();
    return 0;
}

