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
#include <atomic>
#include "imgui.h"
#include "touch_input.h"

#define TOUCH_MODE 2

namespace touch {
    struct input {
        int fd{-1};
        bool canBeUsed{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{1.0f}, absYMultiplier{1.0f};
#if TOUCH_MODE == 2
        bool trackingIDPresent{false};
#endif

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) {
            fd = open(_path, O_RDWR);
            if (fd == -1) {
                fd = open(_path, O_RDONLY);
            }
            if (fd == -1)
                return;
            canBeUsed = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1 &&
                        ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1 &&
                        absX.maximum > 0 && absY.maximum > 0;
            if (!canBeUsed) {
                close();
                return;
            }

#if TOUCH_MODE == 2
            input_absinfo absTrackingID{};
            trackingIDPresent = ioctl(fd, EVIOCGABS(ABS_MT_TRACKING_ID), &absTrackingID) != -1 &&
                                absTrackingID.minimum <= 0 && absTrackingID.maximum > 0;
#endif

            calculateMultipliers(_screen_w, _screen_h);
        }

        void close() {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }

        void calculateMultipliers(int32_t _screen_w, int32_t _screen_h) {
            auto max = fmax((float) absX.maximum, (float) absY.maximum);
            auto min = fmin((float) absX.maximum, (float) absY.maximum);
            if (max <= 0.f || min <= 0.f) return;
            absXMultiplier = (float) _screen_w / (_screen_w > _screen_h ? max : min);
            absYMultiplier = (float) _screen_h / (_screen_w > _screen_h ? min : max);
        }
    };

    static uint8_t orientation = 1;
    static ImGuiIO *imgui_io = nullptr;
    static std::vector<input> inputs;
    static std::atomic<bool> g_touch_running{true};

    void input_thread(input dev) {
        usleep(100000);
        if (!imgui_io) {
            imgui_io = &ImGui::GetIO();
        }
        input_event events[64]{0};
        bool isDown = false;
        float x = 0.0f, y = 0.0f;

        while (dev.fd > 0 && g_touch_running.load()) {
            auto event_readed_count = read(dev.fd, events, sizeof(events));
            if (event_readed_count <= 0) {
                usleep(2000);
                continue;
            }
            event_readed_count /= (ssize_t) sizeof(input_event);

            for (long j = 0; j < event_readed_count; j++) {
                auto &event = events[j];
                if (event.type == EV_ABS) {
#if TOUCH_MODE == 2
                    if (dev.trackingIDPresent && event.code == ABS_MT_TRACKING_ID) {
                        isDown = (event.value != -1 && (uint32_t)event.value != 0xFFFFFFFF);
                        continue;
                    }
#elif TOUCH_MODE == 0
                    if (event.code == ABS_MT_TRACKING_ID) {
                        isDown = (event.value != -1 && (uint32_t)event.value != 0xFFFFFFFF);
                        continue;
                    }
#endif
                    if (event.code == ABS_MT_POSITION_X) {
                        x = (float) event.value;
                        continue;
                    }
                    if (event.code == ABS_MT_POSITION_Y) {
                        y = (float) event.value;
                        continue;
                    }
                }
                if (event.code == SYN_REPORT && imgui_io) {
#if TOUCH_MODE == 2
                    imgui_io->MouseDown[0] = dev.trackingIDPresent ? isDown : event_readed_count > 2;
#elif TOUCH_MODE == 0
                    imgui_io->MouseDown[0] = isDown;
#else
                    imgui_io->MouseDown[0] = event_readed_count > 2;
#endif
                    if (imgui_io->MouseDown[0]) {
                        switch (orientation) {
                            case 1:
                                imgui_io->MousePos.x = y;
                                imgui_io->MousePos.y = dev.absX.maximum - x;
                                break;
                            case 2:
                                imgui_io->MousePos.x = dev.absX.maximum - x;
                                imgui_io->MousePos.y = dev.absY.maximum - y;
                                break;
                            case 3:
                                imgui_io->MousePos.x = dev.absY.maximum - y;
                                imgui_io->MousePos.y = x;
                                break;
                            default:
                                imgui_io->MousePos.x = x;
                                imgui_io->MousePos.y = y;
                                break;
                        }
                        imgui_io->MousePos.x *= dev.absXMultiplier;
                        imgui_io->MousePos.y *= dev.absYMultiplier;
                    }
                }
            }
        }
    }

    bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        orientation = _orientation;
        g_touch_running.store(true);
        imgui_io = &ImGui::GetIO();

        bool result = false;
        const char *path = "/dev/input/";
        auto dir = opendir(path);
        if (!dir) return false;

        dirent *ptr;
        while ((ptr = readdir(dir))) {
            if (strstr(ptr->d_name, "event")) {
                char event_path[128];
                snprintf(event_path, sizeof(event_path), "%s%s", path, ptr->d_name);
                input _input(event_path, _screen_w, _screen_h);
                if (_input.canBeUsed) {
                    result = true;
                    inputs.push_back(_input);
                    std::thread(input_thread, _input).detach();
                }
            }
        }
        closedir(dir);
        return result;
    }

    void update(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        orientation = _orientation;
        for (auto &_input: inputs) {
            _input.calculateMultipliers(_screen_w, _screen_h);
        }
    }

    void update_orientation(uint8_t _orientation) {
        orientation = _orientation;
    }

    uint8_t get_orientation() {
        return orientation;
    }

    void toggle_orientation() {
        if (orientation == 1) orientation = 3;
        else orientation = 1;
    }

    void poll() {
        // Direct event injection: input_thread continuously updates imgui_io
    }

    void shutdown() {
        g_touch_running.store(false);
        for (auto &_input: inputs) {
            _input.close();
        }
        inputs.clear();
        imgui_io = nullptr;
    }
}
