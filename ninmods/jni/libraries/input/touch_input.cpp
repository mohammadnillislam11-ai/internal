#include <atomic>
#include <cstdio>
#include <cstring>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <cmath>
#include <unistd.h>
#include <vector>
#include "imgui.h"

#define TOUCH_MODE 2

namespace touch {
    struct input {
        int fd;
        bool canBeUsed{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{0}, absYMultiplier{0};
#if TOUCH_MODE == 2
        bool trackingIDPresent{false};
#endif
        bool isDown{false};
        float x{0.0f}, y{0.0f};

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) {
            fd = open(_path, O_RDWR);
            if (fd == -1)
                return;
            canBeUsed = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1 && ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1 && absX.maximum > 0 && absY.maximum > 0;
            if (!canBeUsed) {
                close();
                return;
            }

#if TOUCH_MODE == 2
            input_absinfo absTrackingID{};
            trackingIDPresent = ioctl(fd, EVIOCGABS(ABS_MT_TRACKING_ID), &absTrackingID) != -1 && absTrackingID.minimum <= 0 && absTrackingID.maximum > 0;
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
            absXMultiplier = (float) _screen_w / (_screen_w > _screen_h ? max : min);
            absYMultiplier = (float) _screen_h / (_screen_w > _screen_h ? min : max);
        }
    };

    static std::atomic<int> g_active_touch_fd{-1};
uint8_t orientation = 1;
    ImGuiIO *imgui_io = nullptr;
    std::vector<std::shared_ptr<input>> inputs;

    void input_thread(std::shared_ptr<input> _input) {
        sleep(1);
        if (!imgui_io)
            imgui_io = &ImGui::GetIO();
        input_event events[64]{0};
        while (_input->fd > 0 && imgui_io && imgui_io->BackendRendererUserData) {
            auto event_readed_count = read(_input->fd, events, sizeof(events));
            if (event_readed_count == -1)
                continue;
            event_readed_count /= (ssize_t) sizeof(input_event);

            for (long j = 0; j < event_readed_count; j++) {
                auto &event = events[j];
                if (event.type == EV_KEY && (event.code == BTN_TOUCH || event.code == BTN_LEFT)) {
                    _input->isDown = (event.value != 0);
                }
                if (event.type == EV_ABS) {
#if TOUCH_MODE == 2
                    if (_input->trackingIDPresent && event.code == ABS_MT_TRACKING_ID) {
                        _input->isDown = (event.value != -1);
                        continue;
                    }
#elif TOUCH_MODE == 0
                    if (event.code == ABS_MT_TRACKING_ID) {
                        _input->isDown = (event.value != -1);
                        continue;
                    }
#endif
                    if (event.code == ABS_MT_POSITION_X) {
                        _input->x = (float) event.value;
                        continue;
                    }
                    if (event.code == ABS_MT_POSITION_Y) {
                        _input->y = (float) event.value;
                        continue;
                    }
                }
                if (event.code == SYN_REPORT && imgui_io) {
#if TOUCH_MODE == 2
                    bool curDown = _input->trackingIDPresent ? _input->isDown : (event_readed_count > 2);
#elif TOUCH_MODE == 0
                    bool curDown = _input->isDown;
#else
                    bool curDown = event_readed_count > 2;
#endif
                    if (curDown) {
                        g_active_touch_fd.store(_input->fd, std::memory_order_relaxed);
                        imgui_io->MouseDown[0] = true;

                        float calcX = 0.0f;
                        float calcY = 0.0f;
                        switch (orientation) {
                            case 1:
                                calcX = _input->y;
                                calcY = _input->absX.maximum - _input->x;
                                break;
                            case 2:
                                calcX = _input->absX.maximum - _input->x;
                                calcY = _input->absY.maximum - _input->y;
                                break;
                            case 3:
                                calcX = _input->absY.maximum - _input->y;
                                calcY = _input->x;
                                break;
                            default:
                                calcX = _input->x;
                                calcY = _input->y;
                                break;
                        }
                        imgui_io->MousePos.x = calcX * _input->absXMultiplier;
                        imgui_io->MousePos.y = calcY * _input->absYMultiplier;
                    } else {
                        int expected = _input->fd;
                        if (g_active_touch_fd.compare_exchange_strong(expected, -1, std::memory_order_relaxed)) {
                            imgui_io->MouseDown[0] = false;
                        }
                    }
                }
            }
        }
    }

    bool init(int32_t _screen_w, int32_t _screen_h, uint8_t _orientation) {
        orientation = _orientation;

        bool result = false;

        const char *path = "/dev/input/";
        auto dir = opendir(path);
        if (!dir) return false;

        dirent *ptr;
        while ((ptr = readdir(dir))) {
            if (strstr(ptr->d_name, "event")) {
                char event_path[128];
                sprintf(event_path, "%s%s", path, ptr->d_name);
                auto _input = std::make_shared<input>(event_path, _screen_w, _screen_h);
                if (_input->canBeUsed) {
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
            _input->calculateMultipliers(_screen_w, _screen_h);
        }
    }

    void update_orientation(uint8_t _orientation) {
        orientation = _orientation;
    }

    void updateOrientation(uint8_t _orientation) {
        update_orientation(_orientation);
    }

    uint8_t get_orientation() {
        return orientation;
    }

    void toggle_orientation() {
        orientation = (orientation == 1) ? 3 : 1;
    }

    void poll() {
        // Events are read asynchronously in worker thread
    }

    void shutdown() {
        for (auto &_input: inputs)
            _input->close();
        inputs.clear();
        imgui_io = nullptr;
    }
}
