// NINMODS — Standalone Client
#include "Android_draw/draw.h"
#include "ui/styles/styles.h"
#include "ui/menu.hpp"
#include "ui/bar.hpp"
#include "other/memory.hpp"
#include "game/data.hpp"
#include "game/AimSilent.h"
#include "game/Offsets.h"
#include "func/esp.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/wait.h>
#include <android/log.h>
#include <iostream>
#include <string>
#include <atomic>

#define LOG_TAG "CLIENT"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

/** ANSI terminal color escapes */
#define RST        "\033[0m"
#define BOLD       "\033[1m"
#define FG_BRED    "\033[91m"
#define FG_BGREEN  "\033[92m"
#define FG_BYELLOW "\033[93m"
#define FG_BBLUE   "\033[94m"
#define FG_BMAGENTA "\033[95m"
#define FG_BCYAN   "\033[96m"
#define FG_BWHITE  "\033[97m"

/** Global overlay lifecycle flags */
std::atomic<bool> g_menu_started{true};
extern float global_anim_time;

/** Bypass initialization routine */
extern "C" void init_bypass();

/** Recording protection flag */
bool g_hideRecord = false;

/** Supported target game variants */
enum TargetGame {
    GAME_FF     = 0,
    GAME_FF_MAX = 1
};

static TargetGame g_targetGame = GAME_FF;

static const char* kFFActivity    = "com.dts.freefireth/com.dts.freefireth.FFMainActivity";
static const char* kFFMaxActivity = "com.dts.freefiremax/com.dts.freefiremax.FFMainActivity";
static const char* kFFPackage     = "com.dts.freefireth";
static const char* kFFMaxPackage  = "com.dts.freefiremax";

static const char* GetTargetActivity() {
    return (g_targetGame == GAME_FF) ? kFFActivity : kFFMaxActivity;
}
static const char* GetTargetPackage() {
    return (g_targetGame == GAME_FF) ? kFFPackage : kFFMaxPackage;
}

/**
 * @brief Silently launches the target activity via Android activity manager.
 * @return True if process execution was dispatched successfully.
 */
static bool launch_target_silent() {
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        execl("/system/bin/am", "am", "start",
              "--activity-brought-to-front",
              "-n", GetTargetActivity(), (char*)nullptr);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/**
 * @brief Resolves target game PID, launching the application if not already running.
 * @return Positive PID upon success, or -1 on failure.
 */
static int attach_or_launch() {
    proc::pid = get_pid();
    if (proc::pid > 0) {
        LOGI("[+] Already running, pid=%d", proc::pid);
        return proc::pid;
    }
    LOGI("[+] Launching %s...", GetTargetPackage());
    launch_target_silent();
    for (int i = 0; i < 120; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        proc::pid = get_pid();
        if (proc::pid > 0) {
            LOGI("[+] Attached, pid=%d", proc::pid);
            return proc::pid;
        }
    }
    return -1;
}

/**
 * @brief Prints client startup header to standard output.
 */
static void print_banner() {
    printf("\n");
    printf(FG_BCYAN BOLD "  [ NINMODS CLIENT ]\n" RST);
    printf(FG_BCYAN      "  Purple Glass Edition (v1.0)\n" RST);
    printf("\n");
}

/**
 * @brief Initializes native surface, touch system, bypass, memory hook, and render loop.
 * @return 0 on exit, or -1 on initialization failure.
 */
static int start_overlay() {
    LOGI("=== Starting Client Overlay ===");

    // ─── INIT GUI ───
    printf(FG_BBLUE "  [1/3] " RST FG_BWHITE "Init GUI...\n" RST);

    screen_config();

    // Fallback if dimensions are 0
    if (displayInfo.width <= 0 || displayInfo.height <= 0) {
        displayInfo.width = 1080;
        displayInfo.height = 2400;
    }

    int max_size = (displayInfo.height > displayInfo.width)
                   ? displayInfo.height : displayInfo.width;
    int min_size = (displayInfo.height < displayInfo.width)
                   ? displayInfo.height : displayInfo.width;

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    printf(FG_BWHITE "  [i] Display size: %d x %d (ori: %d)\n" RST,
           max_size, min_size, displayInfo.orientation);

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        printf(FG_BRED "  [!] initGUI FAILED!\n" RST);
        return -1;
    }
    printf(FG_BGREEN "  [✓] GUI ready\n" RST);

    uint8_t initial_ori = (displayInfo.orientation != 0) ? (uint8_t)displayInfo.orientation : 1;
    touch::init(max_size, min_size, initial_ori);

    // Open menu directly without login screens
    ui::bar::g_open = true;
    g_menu_started.store(true);

    // ─── BYPASS ───
    printf(FG_BBLUE "  [2/3] " RST FG_BWHITE "Init bypass...\n" RST);
    init_bypass();

    // ─── ATTACH KE GAME ───
    printf(FG_BBLUE "  [3/3] " RST FG_BWHITE "Attaching to " FG_BYELLOW "%s" FG_BWHITE "...\n" RST,
           GetTargetPackage());

    if (attach_or_launch() <= 0) {
        printf(FG_BRED "  [!] Gagal attach\n" RST);
        return -1;
    }

    proc::lib = get_lib();
    printf(FG_BGREEN "  [✓] Attached! PID: " FG_BYELLOW "%d\n" RST, proc::pid);

    // ─── INIT AIM SILENT HOOK ───
    InitAimSilentHook();
    printf(FG_BGREEN "  [✓] Aim Silent Hook initialized\n" RST);

    // ─── START DATA THREAD ───
    data::thread_start();
    printf(FG_BGREEN "  [✓] Data thread started\n" RST);
    fflush(stdout);

    printf(FG_BGREEN BOLD "  ⭐ Menu Started Successfully! ⭐\n" RST);
    fflush(stdout);

    /** Main rendering loop */
    while (true) {
        global_anim_time += 0.016f;
        drawBegin();
        func::esp::draw();
        ui::menu::render();
        drawEnd();
        usleep(2000);
    }
    return 0;
}

/**
 * @brief Application entry point.
 * @param argc Argument count.
 * @param argv Argument vectors.
 * @return Exit status code.
 */
int main(int argc, char** argv) {
    print_banner();

    if (argc >= 2) {
        if (strcmp(argv[1], "ffmax") == 0) {
            g_targetGame = GAME_FF_MAX;
            printf(FG_BGREEN "  [+] Target: " FG_BYELLOW "Free Fire MAX\n" RST);
        }
    }

    return start_overlay();
}
