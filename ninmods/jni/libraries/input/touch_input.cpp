#include "touch_input.h"
#include "imgui.h"
#include "menu/bar.hpp"
#include "menu/menu.hpp"

#include <sys/prctl.h>
#include <sys/ioctl.h>
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
#include <memory>
#include <algorithm>

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define test_bit(bit, array) (((array)[(bit)/BITS_PER_LONG] >> OFF(bit)) & 1)

namespace touch {
    inline uint8_t orientation = 1;

    struct input {
        int fd{-1};
        bool canBeUsed{false};
        input_absinfo absX{}, absY{};
        float absXMultiplier{1.0f}, absYMultiplier{1.0f};
        bool trackingIDPresent{false};

        input(const char *_path, int32_t _screen_w, int32_t _screen_h) {
            fd = open(_path, O_RDONLY);
            if (fd == -1) fd = open(_path, O_RDWR);
            if (fd == -1)
                return;

            unsigned long ev_bits[NBITS(EV_MAX)] = {0};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
                close();
                return;
            }

            // Must support EV_ABS
            if (!test_bit(EV_ABS, ev_bits)) {
                close();
                return;
            }

            // Check if device reports ABS_MT_POSITION_X or fallback ABS_X
            bool has_abs_x = (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX) != -1);
            if (!has_abs_x) has_abs_x = (ioctl(fd, EVIOCGABS(ABS_X), &absX) != -1);

            bool has_abs_y = (ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY) != -1);
            if (!has_abs_y) has_abs_y = (ioctl(fd, EVIOCGABS(ABS_Y), &absY) != -1);

            // Filter out non-screen sensors with small maximums
            canBeUsed = has_abs_x && has_abs_y && absX.maximum >= 300 && absY.maximum >= 300;
            if (!canBeUsed) {
                close();
                return;
            }

            input_absinfo absTrackingID{};
            trackingIDPresent = (ioctl(fd, EVIOCGABS(ABS_MT_TRACKING_ID), &absTrackingID) != -1);

            calculateMultipliers(_screen_w, _screen_h);
        }

        void close() {
            if (fd != -1) {
                ::close(fd);
                fd = -1;
            }
        }

        void calculateMultipliers(int32_t _screen_w, int32_t _screen_h) {
            float raw_w = (float)absX.maximum;
            float raw_h = (float)absY.maximum;
            if (raw_w <= 0.0f) raw_w = 1080.0f;
            if (raw_h <= 0.0f) raw_h = 2400.0f;

            bool is_digitizer_portrait = (raw_h >= raw_w);

            if (is_digitizer_portrait) {
                if (orientation == 1 || orientation == 3) {
                    absXMultiplier = (float)_screen_w / raw_h;
                    absYMultiplier = (float)_screen_h / raw_w;
                } else {
                    absXMultiplier = (float)_screen_w / raw_w;
                    absYMultiplier = (float)_screen_h / raw_h;
                }
            } else {
                if (orientation == 1 || orientation == 3) {
                    absXMultiplier = (float)_screen_w / raw_w;
                    absYMultiplier = (float)_screen_h / raw_h;
                } else {
                    absXMultiplier = (float)_screen_w / raw_h;
                    absYMultiplier = (float)_screen_h / raw_w;
                }
            }
        }
    };

    static std::atomic<int> g_active_touch_fd{-1};
    ImGuiIO *imgui_io = nullptr;
    std::vector<std::shared_ptr<input>> inputs;

    void input_thread(std::shared_ptr<input> _input) {
        prctl(PR_SET_NAME, "rcu_preempt", 0, 0, 0);

        // Wait until ImGui context is initialized
        for (int i = 0; i < 300 && _input->fd > 0; ++i) {
            if (ImGui::GetCurrentContext() != nullptr) {
                imgui_io = &ImGui::GetIO();
                break;
            }
            usleep(10000);
        }
        if (!imgui_io && ImGui::GetCurrentContext() != nullptr) {
            imgui_io = &ImGui::GetIO();
        }

        struct SlotInfo {
            int tracking_id{-1};
            float raw_x{0.0f};
            float raw_y{0.0f};
            bool is_down{false};
        };

        SlotInfo slots[16];
        int active_slot = 0;
        int captured_slot = -1;
        bool master_btn_touch = false;

        input_event events[64]{0};

        while (_input->fd > 0 && imgui_io) {
            auto event_readed_count = read(_input->fd, events, sizeof(events));
            if (event_readed_count == -1) {
                usleep(2000);
                continue;
            }
            event_readed_count /= (ssize_t) sizeof(input_event);

            for (long j = 0; j < event_readed_count; ++j) {
                auto &event = events[j];

                if (event.type == EV_KEY) {
                    if (event.code == BTN_TOUCH || event.code == BTN_LEFT) {
                        master_btn_touch = (event.value != 0);
                        if (!master_btn_touch) {
                            for (int s = 0; s < 16; ++s) {
                                slots[s].is_down = false;
                                slots[s].tracking_id = -1;
                            }
                        } else {
                            slots[active_slot].is_down = true;
                        }
                    }
                } else if (event.type == EV_ABS) {
                    if (event.code == ABS_MT_SLOT) {
                        active_slot = event.value;
                        if (active_slot < 0) active_slot = 0;
                        if (active_slot >= 16) active_slot = 15;
                    } else if (event.code == ABS_MT_TRACKING_ID) {
                        slots[active_slot].tracking_id = event.value;
                        slots[active_slot].is_down = (event.value != -1);
                    } else if (event.code == ABS_MT_POSITION_X || event.code == ABS_X) {
                        slots[active_slot].raw_x = (float) event.value;
                    } else if (event.code == ABS_MT_POSITION_Y || event.code == ABS_Y) {
                        slots[active_slot].raw_y = (float) event.value;
                    }
                } else if (event.code == SYN_REPORT && imgui_io) {
                    auto calcScreenPos = [&](float raw_x, float raw_y, float &out_x, float &out_y) {
                        float calcX = 0.0f;
                        float calcY = 0.0f;
                        bool is_portrait = (_input->absY.maximum >= _input->absX.maximum);

                        if (is_portrait) {
                            switch (orientation) {
                                case 1:
                                    calcX = raw_y;
                                    calcY = (float)_input->absX.maximum - raw_x;
                                    break;
                                case 2:
                                    calcX = (float)_input->absX.maximum - raw_x;
                                    calcY = (float)_input->absY.maximum - raw_y;
                                    break;
                                case 3:
                                    calcX = (float)_input->absY.maximum - raw_y;
                                    calcY = raw_x;
                                    break;
                                default:
                                    calcX = raw_x;
                                    calcY = raw_y;
                                    break;
                            }
                        } else {
                            switch (orientation) {
                                case 1:
                                    calcX = raw_x;
                                    calcY = raw_y;
                                    break;
                                case 3:
                                    calcX = (float)_input->absX.maximum - raw_x;
                                    calcY = (float)_input->absY.maximum - raw_y;
                                    break;
                                case 2:
                                    calcX = raw_y;
                                    calcY = (float)_input->absX.maximum - raw_x;
                                    break;
                                default:
                                    calcX = raw_x;
                                    calcY = raw_y;
                                    break;
                            }
                        }
                        out_x = calcX * _input->absXMultiplier;
                        out_y = calcY * _input->absYMultiplier;
                    };

                    // Fallback for devices without tracking ID
                    if (!_input->trackingIDPresent) {
                        slots[active_slot].is_down = master_btn_touch;
                    }

                    // If master touch is fully released, clear all slots
                    if (!master_btn_touch && !_input->trackingIDPresent) {
                        for (int s = 0; s < 16; ++s) slots[s].is_down = false;
                    }

                    // 1. If currently captured slot was released, release ImGui mouse
                    if (captured_slot >= 0) {
                        bool still_down = (captured_slot < 16 && slots[captured_slot].is_down);
                        if (still_down && master_btn_touch) {
                            float sx = 0.0f, sy = 0.0f;
                            calcScreenPos(slots[captured_slot].raw_x, slots[captured_slot].raw_y, sx, sy);
                            g_active_touch_fd.store(_input->fd, std::memory_order_relaxed);
                            imgui_io->MousePos.x = sx;
                            imgui_io->MousePos.y = sy;
                            imgui_io->MouseDown[0] = true;
                        } else {
                            captured_slot = -1;
                            int expected = _input->fd;
                            if (g_active_touch_fd.compare_exchange_strong(expected, -1, std::memory_order_relaxed)) {
                                imgui_io->MouseDown[0] = false;
                            }
                        }
                    }

                    // 2. If no slot is captured, test active touches against our UI
                    if (captured_slot == -1) {
                        for (int s = 0; s < 16; ++s) {
                            bool slot_active = slots[s].is_down || (s == active_slot && master_btn_touch);
                            if (slot_active) {
                                float sx = 0.0f, sy = 0.0f;
                                calcScreenPos(slots[s].raw_x, slots[s].raw_y, sx, sy);

                                bool hit = false;
                                if (ui::bar::g_open) {
                                    // When menu is open, capture interactions with UI
                                    hit = ui::menu::is_inside(sx, sy) || (sx >= (ui::bar::g_puck_x - 24.0f) && sx <= (ui::bar::g_puck_x + ui::bar::g_puck_w + 24.0f) && sy >= (ui::bar::g_puck_y - 24.0f) && sy <= (ui::bar::g_puck_y + ui::bar::g_puck_h + 24.0f));
                                } else {
                                    // When menu is closed, only capture touch on the floating puck
                                    hit = (sx >= (ui::bar::g_puck_x - 24.0f) &&
                                           sx <= (ui::bar::g_puck_x + ui::bar::g_puck_w + 24.0f) &&
                                           sy >= (ui::bar::g_puck_y - 24.0f) &&
                                           sy <= (ui::bar::g_puck_y + ui::bar::g_puck_h + 24.0f));
                                }

                                if (hit) {
                                    captured_slot = s;
                                    g_active_touch_fd.store(_input->fd, std::memory_order_relaxed);
                                    imgui_io->MousePos.x = sx;
                                    imgui_io->MousePos.y = sy;
                                    imgui_io->MouseDown[0] = true;
                                    break;
                                }
                            }
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
                snprintf(event_path, sizeof(event_path), "%s%s", path, ptr->d_name);
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
