#include <cstdio>
#include <cstring>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <cmath>
#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include <mutex>
#include "imgui.h"
#include "Android_touch/Touch.hpp"

namespace touch {
    static std::atomic<bool> s_phys_down{false};
    static std::atomic<float> s_phys_x{0.0f};
    static std::atomic<float> s_phys_y{0.0f};
    static bool s_touching = false;
    static double s_touch_start_time = 0.0;
    static float s_last_x = 0.0f;
    static float s_last_y = 0.0f;

    struct input {
        std::string path;
        int fd{-1};
        bool canBeUsed{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{1.0f}, absYMultiplier{1.0f};

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) : path(_path) {
            fd = open(_path, O_RDONLY);
            if (fd == -1) {
                fd = open(_path, O_RDWR);
            }
            if (fd == -1)
                return;

            // Check device name and reject non-touchscreen devices (sensors, accelerometers, power keys)
            char devName[256] = {0};
            ioctl(fd, EVIOCGNAME(sizeof(devName)), devName);
            std::string sName = devName;
            std::transform(sName.begin(), sName.end(), sName.begin(), ::tolower);

            if (sName.find("sensor") != std::string::npos ||
                sName.find("accel") != std::string::npos ||
                sName.find("gyro") != std::string::npos ||
                sName.find("pwr") != std::string::npos ||
                sName.find("power") != std::string::npos ||
                sName.find("button") != std::string::npos ||
                sName.find("volume") != std::string::npos ||
                sName.find("headset") != std::string::npos ||
                sName.find("keyboard") != std::string::npos ||
                sName.find("pen") != std::string::npos ||
                sName.find("stylus") != std::string::npos ||
                sName.find("key") != std::string::npos) {
                close();
                return;
            }

            // Real touchscreen MUST have ABS_MT_POSITION_X and ABS_MT_POSITION_Y
            if (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) == -1 ||
                ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) == -1 ||
                absX.maximum <= 0 || absY.maximum <= 0) {
                close();
                return;
            }

            canBeUsed = true;
            calculateMultipliers(_screen_w, _screen_h);
        }

        void close() {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }

        void calculateMultipliers(int32_t _screen_w, int32_t _screen_h) {
            if (_screen_w <= 0 || _screen_h <= 0) return;
            auto max = fmax((float) absX.maximum, (float) absY.maximum);
            auto min = fmin((float) absX.maximum, (float) absY.maximum);
            if (max <= 0.f || min <= 0.f) return;
            absXMultiplier = (float) _screen_w / (_screen_w > _screen_h ? max : min);
            absYMultiplier = (float) _screen_h / (_screen_w > _screen_h ? min : max);
        }
    };

    static std::atomic<bool> g_touch_running{true};
    static uint8_t orientation = 1;
    static std::vector<input> inputs;

    uint8_t getOrientation() {
        return orientation;
    }

    void toggleOrientation() {
        if (orientation == 1) orientation = 3;
        else orientation = 1;
        printf("  [Touch] Orientation toggled to: %d\n", (int)orientation);
        fflush(stdout);
    }

    void poll() {
        ImGuiIO& io = ImGui::GetIO();
        double now = ImGui::GetTime();

        bool physDown = s_phys_down.load();
        float x = s_phys_x.load();
        float y = s_phys_y.load();

        if (physDown) {
            if (!s_touching) {
                s_touch_start_time = now;
                s_touching = true;
            }
            s_last_x = x;
            s_last_y = y;
            io.AddMousePosEvent(x, y);
            io.AddMouseButtonEvent(0, true);
            io.MousePos = ImVec2(x, y);
            io.MouseDown[0] = true;
        } else {
            if (s_touching && (now - s_touch_start_time < 0.060)) {
                // Reference mechanism from PrabirxLive.cpp:
                // Hold MouseDown for at least 60ms so ImGui's ButtonBehavior reliably captures fast finger taps!
                io.AddMousePosEvent(s_last_x, s_last_y);
                io.AddMouseButtonEvent(0, true);
                io.MousePos = ImVec2(s_last_x, s_last_y);
                io.MouseDown[0] = true;
            } else if (s_touching) {
                // Finger released after minimum hold duration
                io.AddMousePosEvent(s_last_x, s_last_y);
                io.AddMouseButtonEvent(0, false);
                io.MousePos = ImVec2(s_last_x, s_last_y);
                io.MouseDown[0] = false;
                s_touching = false;
            }
        }
    }

    void input_thread(input dev) {
        printf("  [Touch] Listener thread active on %s\n", dev.path.c_str());
        fflush(stdout);

        input_event events[64]{0};
        bool isDown = false;
        bool lastDown = false;
        float raw_x = 0.0f, raw_y = 0.0f;
        float last_screen_x = -1.f, last_screen_y = -1.f;
        int clickCount = 0;

        while (dev.fd > 0 && g_touch_running.load()) {
            auto event_readed_count = read(dev.fd, events, sizeof(events));
            if (event_readed_count <= 0) {
                usleep(2000);
                continue;
            }
            event_readed_count /= (ssize_t) sizeof(input_event);

            for (long j = 0; j < event_readed_count; j++) {
                auto &event = events[j];

                if (event.type == EV_KEY) {
                    if (event.code == BTN_TOUCH || event.code == BTN_LEFT) {
                        isDown = (event.value != 0);
                    }
                } else if (event.type == EV_ABS) {
                    if (event.code == ABS_MT_TRACKING_ID) {
                        isDown = (event.value != -1 && (uint32_t)event.value != 0xFFFFFFFF);
                    } else if (event.code == ABS_MT_POSITION_X) {
                        raw_x = (float) event.value;
                    } else if (event.code == ABS_MT_POSITION_Y) {
                        raw_y = (float) event.value;
                    }
                } else if (event.type == EV_SYN && event.code == SYN_REPORT) {
                    float screen_x = raw_x;
                    float screen_y = raw_y;

                    switch (orientation) {
                        case 1: // Landscape (ROTATION_90)
                            screen_x = raw_y * dev.absXMultiplier;
                            screen_y = (dev.absX.maximum - raw_x) * dev.absYMultiplier;
                            break;
                        case 2: // Reverse Portrait (ROTATION_180)
                            screen_x = (dev.absX.maximum - raw_x) * dev.absXMultiplier;
                            screen_y = (dev.absY.maximum - raw_y) * dev.absYMultiplier;
                            break;
                        case 3: // Reverse Landscape (ROTATION_270)
                            screen_x = (dev.absY.maximum - raw_y) * dev.absXMultiplier;
                            screen_y = raw_x * dev.absYMultiplier;
                            break;
                        case 0: // Portrait
                        default:
                            screen_x = raw_x * dev.absXMultiplier;
                            screen_y = raw_y * dev.absYMultiplier;
                            break;
                    }

                    if (isDown) {
                        last_screen_x = screen_x;
                        last_screen_y = screen_y;
                        s_phys_x.store(screen_x);
                        s_phys_y.store(screen_y);
                        s_phys_down.store(true);
                    } else if (lastDown) {
                        s_phys_down.store(false);
                    }

                    lastDown = isDown;
                }
            }
        }

        printf("  [Touch] Thread ended for fd=%d\n", dev.fd);
        fflush(stdout);
    }

    bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        g_touch_running.store(true);
        orientation = _orientation;
        if (orientation == 0) {
            orientation = 1; // Landscape for Free Fire
        }

        system("chmod 666 /dev/input/event* >/dev/null 2>&1");

        bool result = false;
        const char *path = "/dev/input/";
        auto dir = opendir(path);
        if (!dir) {
            printf("  [!] Failed to open /dev/input/ directory!\n");
            fflush(stdout);
            return false;
        }

        dirent *ptr;
        while ((ptr = readdir(dir))) {
            if (strstr(ptr->d_name, "event")) {
                char event_path[128];
                snprintf(event_path, sizeof(event_path), "%s%s", path, ptr->d_name);
                input _input(event_path, _screen_w, _screen_h);
                if (_input.canBeUsed) {
                    result = true;
                    printf("  [✓] Primary touchscreen selected: %s (absX: 0..%d, absY: 0..%d)\n",
                           event_path, _input.absX.maximum, _input.absY.maximum);
                    fflush(stdout);
                    inputs.push_back(_input);
                    std::thread(input_thread, _input).detach();
                    break; // STOP searching! A phone has only ONE primary touchscreen!
                }
            }
        }
        closedir(dir);

        if (!result) {
            printf("  [!] WARNING: No touchscreen detected in /dev/input/!\n");
        } else {
            printf("  [✓] Touch system initialized (ori=%d, screen=%dx%d)\n",
                   (int)orientation, _screen_w, _screen_h);
        }
        fflush(stdout);

        return result;
    }

    void update(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        if (_orientation != 0) {
            orientation = _orientation;
        }
        if (_screen_w > 0 && _screen_h > 0) {
            for (auto &_input: inputs) {
                _input.calculateMultipliers(_screen_w, _screen_h);
            }
        }
    }

    void updateOrientation(uint8_t _orientation) {
        if (_orientation != 0) {
            orientation = _orientation;
        }
    }

    void shutdown() {
        g_touch_running.store(false);
        for (auto &_input: inputs)
            _input.close();
        inputs.clear();
        s_phys_down.store(false);
        s_touching = false;
    }
}
