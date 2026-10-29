#include <cstdio>
#include <cstring>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <cmath>
#include <unistd.h>
#include <vector>
#include <atomic>
#include "imgui.h"
#include "Android_touch/Touch.hpp"

namespace touch {
    struct input {
        int fd{-1};
        bool canBeUsed{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{1.0f}, absYMultiplier{1.0f};

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) {
            // First try read-only, safe under all SELinux domains
            fd = open(_path, O_RDONLY);
            if (fd == -1) {
                // Fallback to read-write if required
                fd = open(_path, O_RDWR);
            }
            if (fd == -1)
                return;

            // Check multi-touch position capability
            bool ok = (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1 &&
                       ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1 &&
                       absX.maximum > 0 && absY.maximum > 0);

            if (!ok) {
                // Fallback to ABS_X / ABS_Y (single-touch or legacy)
                ok = (ioctl(fd, EVIOCGABS(ABS_X), &absX) != -1 &&
                      ioctl(fd, EVIOCGABS(ABS_Y), &absY) != -1 &&
                      absX.maximum > 0 && absY.maximum > 0);
            }

            canBeUsed = ok;
            if (!canBeUsed) {
                close();
                return;
            }

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
    static ImGuiIO *imgui_io = nullptr;
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

    void input_thread(input dev) {
        printf("  [Touch] Event thread started for fd=%d (absX_max=%d, absY_max=%d)\n",
               dev.fd, dev.absX.maximum, dev.absY.maximum);
        fflush(stdout);

        input_event events[64]{0};
        bool isDown = false;
        float x = 0.0f, y = 0.0f;
        int dbgTouchCount = 0;

        while (dev.fd > 0 && g_touch_running.load()) {
            auto event_readed_count = read(dev.fd, events, sizeof(events));
            if (event_readed_count <= 0) {
                usleep(2000);
                continue;
            }
            event_readed_count /= (ssize_t) sizeof(input_event);

            for (long j = 0; j < event_readed_count; j++) {
                auto &event = events[j];

                // 1. Check BTN_TOUCH or BTN_LEFT (EV_KEY)
                if (event.type == EV_KEY) {
                    if (event.code == BTN_TOUCH || event.code == BTN_LEFT) {
                        isDown = (event.value != 0);
                    }
                }
                // 2. Check ABS events (EV_ABS)
                else if (event.type == EV_ABS) {
                    if (event.code == ABS_MT_TRACKING_ID) {
                        // -1 or 0xFFFFFFFF means touch released
                        isDown = (event.value != -1 && (uint32_t)event.value != 0xFFFFFFFF);
                    } else if (event.code == ABS_MT_POSITION_X || event.code == ABS_X) {
                        x = (float) event.value;
                    } else if (event.code == ABS_MT_POSITION_Y || event.code == ABS_Y) {
                        y = (float) event.value;
                    }
                }
                // 3. SYN_REPORT -> submit coordinates & state to ImGui
                else if (event.type == EV_SYN && event.code == SYN_REPORT) {
                    if (!imgui_io)
                        imgui_io = &ImGui::GetIO();

                    if (imgui_io) {
                        float screen_x = x;
                        float screen_y = y;

                        switch (orientation) {
                            case 1: // Landscape (ROTATION_90)
                                screen_x = y * dev.absXMultiplier;
                                screen_y = (dev.absX.maximum - x) * dev.absYMultiplier;
                                break;
                            case 2: // Reverse Portrait (ROTATION_180)
                                screen_x = (dev.absX.maximum - x) * dev.absXMultiplier;
                                screen_y = (dev.absY.maximum - y) * dev.absYMultiplier;
                                break;
                            case 3: // Reverse Landscape (ROTATION_270)
                                screen_x = (dev.absY.maximum - y) * dev.absXMultiplier;
                                screen_y = x * dev.absYMultiplier;
                                break;
                            case 0: // Portrait
                            default:
                                screen_x = x * dev.absXMultiplier;
                                screen_y = y * dev.absYMultiplier;
                                break;
                        }

                        // Clamp to ImGui display boundaries
                        if (imgui_io->DisplaySize.x > 100.f)
                            screen_x = fmax(0.0f, fmin(screen_x, imgui_io->DisplaySize.x));
                        if (imgui_io->DisplaySize.y > 100.f)
                            screen_y = fmax(0.0f, fmin(screen_y, imgui_io->DisplaySize.y));

                        // Set ImGui mouse state (both legacy & modern 1.87+ APIs)
                        imgui_io->MousePos = ImVec2(screen_x, screen_y);
                        imgui_io->MouseDown[0] = isDown;
                        imgui_io->AddMousePosEvent(screen_x, screen_y);
                        imgui_io->AddMouseButtonEvent(0, isDown);

                        if (isDown && ++dbgTouchCount <= 10) {
                            printf("  [Touch #%d] Click at (%.1f, %.1f) [raw: x=%.0f, y=%.0f, ori=%d]\n",
                                   dbgTouchCount, screen_x, screen_y, x, y, (int)orientation);
                            fflush(stdout);
                        }
                    }
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
            // Free Fire is strictly a landscape game!
            orientation = 1;
        }

        // Ensure permissions for all input event devices
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
        int devCount = 0;
        while ((ptr = readdir(dir))) {
            if (strstr(ptr->d_name, "event")) {
                char event_path[128];
                snprintf(event_path, sizeof(event_path), "%s%s", path, ptr->d_name);
                input _input(event_path, _screen_w, _screen_h);
                if (_input.canBeUsed) {
                    result = true;
                    devCount++;
                    printf("  [✓] Found touch device: %s (absX: 0..%d, absY: 0..%d)\n",
                           event_path, _input.absX.maximum, _input.absY.maximum);
                    fflush(stdout);
                    inputs.push_back(_input);
                    std::thread(input_thread, _input).detach();
                }
            }
        }
        closedir(dir);

        if (!result) {
            printf("  [!] WARNING: No touch device could be opened in /dev/input/!\n");
        } else {
            printf("  [✓] Touch system initialized (%d device(s), ori=%d, screen=%dx%d)\n",
                   devCount, (int)orientation, _screen_w, _screen_h);
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
        imgui_io = nullptr;
    }
}
