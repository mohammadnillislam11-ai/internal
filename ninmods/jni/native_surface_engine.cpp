#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_android.h"

#include "Font.h"
#include "Icon.h"
#include "Iconcpp.h"
#include "font_data.hpp"
#include "clipboard.h"

#include "menu/menu.hpp"
#include "menu/bar.hpp"
#include "menu/styles.h"
#include "menu/config.hpp"
#include "engine/engine.hpp"
#include "engine/game_data.hpp"
#include "engine/data/game_data.hpp"
#include "engine/esp.hpp"
#include "engine/aimbot.hpp"
#include "engine/shared_channel.hpp"
#include "logger.h"

namespace {
    JavaVM* g_jvm = nullptr;
    ANativeWindow* g_window = nullptr;
    EGLDisplay g_display = EGL_NO_DISPLAY;
    EGLSurface g_surface = EGL_NO_SURFACE;
    EGLContext g_context = EGL_NO_CONTEXT;

    std::thread g_render_thread;
    std::atomic<bool> g_running{false};
    std::mutex g_window_mutex;

    int g_screen_w = 0;
    int g_screen_h = 0;

    void init_imgui_context() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;

        ImGui::StyleColorsDark();

        ImGui_ImplAndroid_Init(g_window);
        ImGui_ImplOpenGL3_Init("#version 300 es");

        clipboard::install_imgui_hooks();

        ImFontConfig cfg;
        cfg.FontDataOwnedByAtlas = false;
        cfg.PixelSnapH = true;
        cfg.OversampleH = 2;
        cfg.OversampleV = 2;
        cfg.RasterizerDensity = 2.0f;
        float fontSize = 25.5f;

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

        ImGui::GetStyle().ScaleAllSizes(1.55f);
    }

    bool init_egl() {
        if (!g_window) return false;

        g_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (g_display == EGL_NO_DISPLAY) return false;

        if (!eglInitialize(g_display, nullptr, nullptr)) return false;

        const EGLint attribList[] = {
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
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
        if (!eglChooseConfig(g_display, attribList, &config, 1, &numConfigs) || numConfigs <= 0) {
            return false;
        }

        EGLint format;
        eglGetConfigAttrib(g_display, config, EGL_NATIVE_VISUAL_ID, &format);
        ANativeWindow_setBuffersGeometry(g_window, 0, 0, format);

        const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        g_context = eglCreateContext(g_display, config, EGL_NO_CONTEXT, contextAttribs);
        if (g_context == EGL_NO_CONTEXT) return false;

        g_surface = eglCreateWindowSurface(g_display, config, g_window, nullptr);
        if (g_surface == EGL_NO_SURFACE) return false;

        if (!eglMakeCurrent(g_display, g_surface, g_surface, g_context)) return false;

        eglSwapInterval(g_display, 1);
        return true;
    }

    void cleanup_egl() {
        if (g_display != EGL_NO_DISPLAY) {
            eglMakeCurrent(g_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (g_context != EGL_NO_CONTEXT) {
                eglDestroyContext(g_display, g_context);
                g_context = EGL_NO_CONTEXT;
            }
            if (g_surface != EGL_NO_SURFACE) {
                eglDestroySurface(g_display, g_surface);
                g_surface = EGL_NO_SURFACE;
            }
            eglTerminate(g_display);
            g_display = EGL_NO_DISPLAY;
        }
    }

    void render_thread_func() {
        {
            std::lock_guard<std::mutex> lock(g_window_mutex);
            if (!init_egl()) {
                logger::log_error("Failed to init EGL on official WindowManager surface");
                return;
            }
            init_imgui_context();
        }

        // Try initializing memory engine (will succeed if root is present)
        engine::init();

        while (g_running.load()) {
            auto frame_start = std::chrono::steady_clock::now();

            int sw = g_screen_w > 0 ? g_screen_w : 1920;
            int sh = g_screen_h > 0 ? g_screen_h : 1080;
            g_sw = static_cast<float>(sw);
            g_sh = static_cast<float>(sh);

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplAndroid_NewFrame(sw, sh);
            ImGui::NewFrame();

            // Background memory worker update
            engine::update();

            // Pure C++ ImGui Menu, Floating Puck, and ESP
            ui::menu::render();

            ImGui::Render();
            glViewport(0, 0, sw, sh);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            {
                std::lock_guard<std::mutex> lock(g_window_mutex);
                if (g_display != EGL_NO_DISPLAY && g_surface != EGL_NO_SURFACE) {
                    eglSwapBuffers(g_display, g_surface);
                }
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - frame_start).count();
            if (elapsed < 16) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16 - elapsed));
            }
        }

        engine::shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        ImGui::DestroyContext();

        std::lock_guard<std::mutex> lock(g_window_mutex);
        cleanup_egl();
    }
}

extern "C" {

JNIEXPORT jstring JNICALL Java_com_google_android_gemini_NativeBridge_popClipboardRequest(
    JNIEnv* env, jclass clazz) {
    std::string text = clipboard::pop_pending_text();
    if (text.empty()) return nullptr;
    return env->NewStringUTF(text.c_str());
}

JNIEXPORT void JNICALL Java_com_google_android_gemini_NativeBridge_initSurface(
    JNIEnv* env, jclass clazz, jobject surface, jint width, jint height) {
    
    if (env && !g_jvm) env->GetJavaVM(&g_jvm);
    std::lock_guard<std::mutex> lock(g_window_mutex);
    g_screen_w = width;
    g_screen_h = height;

    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }

    if (surface) {
        g_window = ANativeWindow_fromSurface(env, surface);
    }

    if (g_window && !g_running.load()) {
        g_running.store(true);
        if (g_render_thread.joinable()) {
            g_render_thread.join();
        }
        g_render_thread = std::thread(render_thread_func);
    }
}

JNIEXPORT void JNICALL Java_com_google_android_gemini_NativeBridge_surfaceChanged(
    JNIEnv* env, jclass clazz, jobject surface, jint width, jint height) {
    g_screen_w = width;
    g_screen_h = height;
}

JNIEXPORT void JNICALL Java_com_google_android_gemini_NativeBridge_surfaceDestroyed(
    JNIEnv* env, jclass clazz) {
    g_running.store(false);
    if (g_render_thread.joinable()) {
        g_render_thread.join();
    }
    std::lock_guard<std::mutex> lock(g_window_mutex);
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

JNIEXPORT void JNICALL Java_com_google_android_gemini_NativeBridge_onTouch(
    JNIEnv* env, jclass clazz, jint action, jfloat x, jfloat y) {
    if (env && !g_jvm) env->GetJavaVM(&g_jvm);
    
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);

    int masked = action & 0xff;
    if (masked == 0) { // ACTION_DOWN
        io.AddMouseButtonEvent(0, true);
    } else if (masked == 1 || masked == 3) { // ACTION_UP || ACTION_CANCEL
        io.AddMouseButtonEvent(0, false);
    }
}

JNIEXPORT jboolean JNICALL Java_com_google_android_gemini_NativeBridge_isTouchInside(
    JNIEnv* env, jclass clazz, jfloat x, jfloat y) {
    
    if (ui::bar::g_open) {
        return ui::menu::is_inside(x, y) ||
               (x >= (ui::bar::g_puck_x - 24.0f) && x <= (ui::bar::g_puck_x + ui::bar::g_puck_w + 24.0f) &&
                y >= (ui::bar::g_puck_y - 24.0f) && y <= (ui::bar::g_puck_y + ui::bar::g_puck_h + 24.0f));
    } else {
        return (x >= (ui::bar::g_puck_x - 24.0f) && x <= (ui::bar::g_puck_x + ui::bar::g_puck_w + 24.0f) &&
                y >= (ui::bar::g_puck_y - 24.0f) && y <= (ui::bar::g_puck_y + ui::bar::g_puck_h + 24.0f));
    }
}

JNIEXPORT jboolean JNICALL Java_com_google_android_gemini_NativeBridge_isMenuOpen(
    JNIEnv* env, jclass clazz) {
    return ui::bar::g_open;
}

JNIEXPORT void JNICALL Java_com_google_android_gemini_NativeBridge_setMenuOpen(
    JNIEnv* env, jclass clazz, jboolean open) {
    ui::bar::g_open = open;
}

JNIEXPORT void JNICALL Java_com_google_android_gemini_NativeBridge_stopEngine(
    JNIEnv* env, jclass clazz) {
    g_running.store(false);
    if (g_render_thread.joinable()) {
        g_render_thread.join();
    }
    std::lock_guard<std::mutex> lock(g_window_mutex);
    if (g_window) {
        ANativeWindow_release(g_window);
        g_window = nullptr;
    }
}

JNIEXPORT jint JNICALL Java_com_google_android_gemini_NativeBridge_fetchESPData(
    JNIEnv* env, jclass clazz, jfloatArray outBuffer) {
    if (env && !g_jvm) env->GetJavaVM(&g_jvm);
    if (!outBuffer) return 0;
    jsize len = env->GetArrayLength(outBuffer);
    if (len < 5) return 0;

    static engine::shm::SharedChannel* s_shm = nullptr;
    if (!s_shm) {
        s_shm = engine::shm::open_shm(false);
    }

    // Sync UI control options into Shared Memory for Root Engine
    if (s_shm && s_shm->magic == GEMINI_SHM_MAGIC) {
        s_shm->config.esp_enabled     = cfg::esp::enabled;
        s_shm->config.tracer_enabled  = cfg::esp::tracer;
        s_shm->config.box_enabled     = cfg::esp::box;
        s_shm->config.health_enabled  = cfg::esp::health;
        s_shm->config.aim_enabled     = cfg::aim::enabled;
        s_shm->config.aim_target      = cfg::aim::target_bone;
        s_shm->config.aim_fov         = cfg::aim::fov;
        s_shm->config.aim_smooth      = cfg::aim::smooth;
        s_shm->config.zero_recoil     = cfg::aim::zero_recoil;

        if (s_shm->entity_count > 0) {
            jfloat temp[1024];
            temp[0] = cfg::aim::draw_fov ? 1.0f : 0.0f;
            temp[1] = cfg::aim::fov;
            temp[2] = static_cast<float>(cfg::esp::box_col_idx);
            temp[3] = static_cast<float>(cfg::esp::vis_col_idx);
            temp[4] = cfg::esp::tracer ? 1.0f : 0.0f;

            int idx = 5;
            for (int i = 0; i < s_shm->entity_count && idx + 8 <= len && idx + 8 <= 1024; i++) {
                const auto& e = s_shm->entities[i];
                if (!e.is_on_screen) continue;

                temp[idx++] = e.head_x;
                temp[idx++] = e.head_y;
                temp[idx++] = e.toe_x;
                temp[idx++] = e.toe_y;
                temp[idx++] = e.distance;
                temp[idx++] = static_cast<float>(e.hp > 0 ? e.hp : 100);
                temp[idx++] = e.is_visible ? 1.0f : 0.0f;
                temp[idx++] = e.is_tracer ? 1.0f : 0.0f;
            }
            env->SetFloatArrayRegion(outBuffer, 0, idx, temp);
            return idx;
        }
    }

    // Fallback: local engine reading
    jfloat temp[1024];
    temp[0] = cfg::aim::draw_fov ? 1.0f : 0.0f;
    temp[1] = cfg::aim::fov;
    temp[2] = static_cast<float>(cfg::esp::box_col_idx);
    temp[3] = static_cast<float>(cfg::esp::vis_col_idx);
    temp[4] = cfg::esp::tracer ? 1.0f : 0.0f;

    int idx = 5;

    bool any_esp = cfg::esp::tracer || cfg::esp::box || cfg::esp::health || cfg::esp::enabled;
    if (any_esp) {
        std::lock_guard<std::mutex> lock(engine::data::g_players_mutex);
        for (const auto& p : engine::data::g_players) {
            if (idx + 8 > len || idx + 8 > 1024) break;
            if (!p.isOnScreen) continue;

            temp[idx++]     = p.Head.x;
            temp[idx++]     = p.Head.y;
            temp[idx++]     = p.Toe.x;
            temp[idx++]     = p.Toe.y;
            temp[idx++]     = p.distance;
            temp[idx++]     = static_cast<float>(p.curHP > 0 ? p.curHP : 100);
            temp[idx++]     = p.isVisible ? 1.0f : 0.0f;
            temp[idx++]     = cfg::esp::tracer ? 1.0f : 0.0f;
        }
    }

    env->SetFloatArrayRegion(outBuffer, 0, idx, temp);
    return idx;
}

JNIEXPORT jint JNICALL Java_com_google_android_gemini_NativeBridge_getTargetPid(
    JNIEnv* env, jclass clazz) {
    static engine::shm::SharedChannel* s_shm = nullptr;
    if (!s_shm) s_shm = engine::shm::open_shm(false);
    if (s_shm && s_shm->active_pid > 0) return s_shm->active_pid;
    if (engine::mem::g_pid > 0) return engine::mem::g_pid;
    if (proc::pid > 0) return proc::pid;
    return engine::mem::g_detected_pid;
}

}
