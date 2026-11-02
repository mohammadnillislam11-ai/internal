#include "touch_input.h"
#include "imgui.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <cmath>
#include <cstring>
#include <linux/input.h>
#include <linux/uinput.h>

/**
 * @file touch_input.cpp
 * @brief Proven Native Touch Engine adapted from External_Panel_Brutal TouchHelperA.
 * Zero-touch freezing: reads multi-touch digitizer events asynchronously via kernel input subsystem
 * without locking, grabbing, or injecting, ensuring 100% native game responsiveness.
 */

#define maxE 5
#define maxF 10
#define UNGRAB 0
#define GRAB 1

static bool other_touch = false;
static uint32_t orientation = 1;
static float screenHeight = 0, screenWidth = 0;
static int screenX = 0, screenY = 0;

struct touchObj {
    bool isDown = false;
    int x = 0;
    int y = 0;
    int id = 0;
};

struct targ {
    int fdNum;
    float S2TX;
    float S2TY;
};

static targ targF[maxE];
static touchObj Finger[maxE][maxF];
static int fdNum = 0, origfd[maxE];
static float scale_x = 1.0f, scale_y = 1.0f;
static bool Touch_initialized = false;
static bool Touch_readOnly = true;

static bool checkDeviceIsTouch(int fd) {
    uint8_t *bits = NULL;
    ssize_t bits_size = 0;
    int res = 0, j, k;
    bool itmp = false, itmp2 = false, itmp3 = false;
    struct input_absinfo abs{};
    while (true) {
        res = ioctl(fd, EVIOCGBIT(EV_ABS, bits_size), bits);
        if (res < 0 || res < bits_size)
            break;
        bits_size = res + 16;
        bits = (uint8_t *) realloc(bits, bits_size * 2);
    }
    if (res > 0 && bits != NULL) {
        for (j = 0; j < res; j++) {
            for (k = 0; k < 8; k++) {
                if (bits[j] & (1 << k)) {
                    int code = j * 8 + k;
                    if (ioctl(fd, EVIOCGABS(code), &abs) == 0) {
                        if (code == ABS_MT_SLOT || code == ABS_MT_TRACKING_ID) {
                            itmp = true;
                        }
                        if (code == ABS_MT_POSITION_X || code == ABS_X) {
                            itmp2 = true;
                        }
                        if (code == ABS_MT_POSITION_Y || code == ABS_Y) {
                            itmp3 = true;
                        }
                    }
                }
            }
        }
    }
    if (bits) free(bits);
    return itmp && itmp2 && itmp3;
}

static void *TypeA(void *arg) {
    targ tmp = *(targ *) arg;
    int i = tmp.fdNum;
    float S2TX = tmp.S2TX;
    float S2TY = tmp.S2TY;
    int latest = 0;
    input_event inputEvent[64]{0};

    while (Touch_initialized) {
        auto readSize = (int32_t) read(origfd[i], inputEvent, sizeof(inputEvent));
        if (readSize <= 0 || (readSize % sizeof(input_event)) != 0) {
            usleep(2000);
            continue;
        }
        size_t count = size_t(readSize) / sizeof(input_event);
        for (size_t j = 0; j < count; j++) {
            input_event &ie = inputEvent[j];
            if (ie.type == EV_ABS) {
                if (ie.code == ABS_MT_SLOT) {
                    if (ie.value >= 0 && ie.value < maxF) {
                        latest = ie.value;
                    }
                    continue;
                }
                if (ie.code == ABS_MT_TRACKING_ID) {
                    if (ie.value == -1) {
                        Finger[i][latest].isDown = false;
                    } else {
                        Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                        Finger[i][latest].isDown = true;
                    }
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_X || ie.code == ABS_X) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].x = (int) (ie.value * S2TX);
                    continue;
                }
                if (ie.code == ABS_MT_POSITION_Y || ie.code == ABS_Y) {
                    Finger[i][latest].id = (i * 2 + 1) * maxF + latest;
                    Finger[i][latest].y = (int) (ie.value * S2TY);
                    continue;
                }
            }
            if (ie.type == EV_KEY) {
                if (ie.code == BTN_TOUCH || ie.code == BTN_TOOL_FINGER || ie.code == BTN_LEFT) {
                    if (ie.value == 0) {
                        for (int f = 0; f < maxF; f++) {
                            Finger[i][f].isDown = false;
                        }
                    } else {
                        Finger[i][latest].isDown = true;
                    }
                }
            }
            if (ie.code == SYN_REPORT) {
                if (ImGui::GetCurrentContext() == nullptr) {
                    continue;
                }
                ImGuiIO &io = ImGui::GetIO();

                // Multi-finger tracking resolution: determine if any touch is active
                bool anyDown = false;
                int activeIndex = latest;
                if (Finger[i][latest].isDown) {
                    anyDown = true;
                    activeIndex = latest;
                } else {
                    for (int f = 0; f < maxF; f++) {
                        if (Finger[i][f].isDown) {
                            anyDown = true;
                            activeIndex = f;
                            break;
                        }
                    }
                }

                if (anyDown) {
                    float x = (float)Finger[i][activeIndex].x;
                    float y = (float)Finger[i][activeIndex].y;
                    float xt = (scale_x > 0.0f) ? (x / scale_x) : x;
                    float yt = (scale_y > 0.0f) ? (y / scale_y) : y;

                    if (other_touch) {
                        switch (orientation) {
                            case 1:
                                x = xt;
                                y = yt;
                                break;
                            case 2:
                                y = yt;
                                x = screenHeight - xt;
                                break;
                            case 3:
                                x = screenHeight - xt;
                                y = screenWidth - yt;
                                break;
                            default:
                                y = xt;
                                x = screenHeight - yt;
                                break;
                        }
                    } else {
                        switch (orientation) {
                            case 1:
                                x = yt;
                                y = screenHeight - xt;
                                break;
                            case 2:
                                x = screenHeight - xt;
                                y = screenWidth - yt;
                                break;
                            case 3:
                                y = xt;
                                x = screenWidth - yt;
                                break;
                            default:
                                x = xt;
                                y = yt;
                                break;
                        }
                    }
                    io.MousePos = ImVec2(x, y);
                    io.MouseDown[0] = true;
                } else {
                    io.MouseDown[0] = false;
                }
                continue;
            }
        }
    }
    return nullptr;
}

static void RecalcScale(int w, int h, uint32_t ori) {
    screenWidth = (float)w;
    screenHeight = (float)h;
    orientation = ori;
    if (orientation == 1 || orientation == 3) {
        scale_x = (float) screenX / (float)h;
        scale_y = (float) screenY / (float)w;
    } else {
        scale_x = (float) screenX / (float)w;
        scale_y = (float) screenY / (float)h;    
    }
}

namespace touch {

    bool init(int32_t screen_w, int32_t screen_h, uint8_t ori) {
        if (Touch_initialized) {
            shutdown();
        }
        char temp[128];
        DIR *dir = opendir("/dev/input/");
        if (!dir) return false;

        dirent *ptr = NULL;
        int eventCount = 0;
        while ((ptr = readdir(dir)) != NULL) {
            if (strstr(ptr->d_name, "event"))
                eventCount++;
        }
        closedir(dir);

        struct input_absinfo absX[maxE], absY[maxE];
        int fd, i, tmp1, tmp2;
        int minCnt = eventCount + 1;
        fdNum = 0;
        for (i = 0; i <= eventCount; i++) {
            snprintf(temp, sizeof(temp), "/dev/input/event%d", i);
            fd = open(temp, O_RDWR);
            if (fd < 0) {
                fd = open(temp, O_RDONLY);
            }
            if (fd < 0) {
                continue;
            }
            if (checkDeviceIsTouch(fd)) {
                tmp1 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &absX[fdNum]);
                if (tmp1 != 0) {
                    tmp1 = ioctl(fd, EVIOCGABS(ABS_X), &absX[fdNum]);
                }
                tmp2 = ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &absY[fdNum]);
                if (tmp2 != 0) {
                    tmp2 = ioctl(fd, EVIOCGABS(ABS_Y), &absY[fdNum]);
                }
                if (tmp1 == 0 && tmp2 == 0) {
                    origfd[fdNum] = fd;
                    if (i < minCnt) {
                        screenX = absX[fdNum].maximum;
                        screenY = absY[fdNum].maximum;
                        minCnt = i;
                    }
                    fdNum++;
                    if (fdNum >= maxE)
                        break;
                } else {
                    close(fd);
                }
            } else {
                close(fd);
            }
        }

        if (fdNum == 0) {
            return false;
        }

        Touch_initialized = true;
        Touch_readOnly = true;

        // Auto-detect digitizer natural aspect ratio (portrait phone vs landscape tablet)
        other_touch = (screenX > screenY);

        RecalcScale(screen_w, screen_h, ori);

        for (i = 0; i < fdNum; i++) {
            targF[i].fdNum = i;
            targF[i].S2TX = (float) screenX / (float) absX[i].maximum;
            targF[i].S2TY = (float) screenY / (float) absY[i].maximum;
            pthread_t t;
            pthread_create(&t, NULL, TypeA, &targF[i]);
            pthread_detach(t);
        }

        return true;
    }

    void update(int32_t screen_w, int32_t screen_h, uint8_t ori) {
        RecalcScale(screen_w, screen_h, ori);
    }

    void update_orientation(uint8_t ori) {
        orientation = ori;
        RecalcScale((int)screenWidth, (int)screenHeight, ori);
    }

    uint8_t get_orientation() {
        return (uint8_t)orientation;
    }

    void toggle_orientation() {
        update_orientation((orientation == 1) ? 3 : 1);
    }

    void poll() {
        // Asynchronously handled in background pthreads
    }

    void shutdown() {
        if (Touch_initialized) {
            Touch_initialized = false;
            usleep(20000);
            for (int i = 0; i < maxE; ++i) {
                if (origfd[i] > 0) {
                    close(origfd[i]);
                    origfd[i] = 0;
                }
            }
            fdNum = 0;
            memset(Finger, 0, sizeof(Finger));
        }
    }
}
