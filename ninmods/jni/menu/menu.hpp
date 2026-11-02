#pragma once
#include <cstdint>
#include "imgui.h"
#include "engine/memory.hpp"
#include "engine/shared_channel.hpp"

namespace ui::menu {
    inline float g_menu_x = -1.f;
    inline float g_menu_y = -1.f;
    inline float g_menu_w = 760.f;
    inline float g_menu_h = 460.f;
    inline ImVec2 g_menu_pos(-1.f, -1.f);

    inline void set_pos(float x, float y) {
        g_menu_pos.x = x;
        g_menu_pos.y = y;
        g_menu_x = x;
        g_menu_y = y;
    }

    inline bool is_inside(float x, float y) {
        if (g_menu_x < 0.f || g_menu_y < 0.f) return false;
        return (x >= g_menu_x && x <= (g_menu_x + g_menu_w) &&
                y >= g_menu_y && y <= (g_menu_y + g_menu_h));
    }

    inline engine::shm::SharedChannel* get_shared_channel() {
        static engine::shm::SharedChannel* s_channel = nullptr;
        if (!s_channel) {
            s_channel = engine::shm::open_shm(false);
        }
        return s_channel;
    }

    inline bool is_driver_online_global() {
        auto* ch = get_shared_channel();
        if (ch && ch->magic == GEMINI_SHM_MAGIC) {
            if (ch->driver_connected || ch->is_attached || ch->active_pid > 0) return true;
        }
        return engine::mem::is_driver_online();
    }

    inline const char* get_driver_node_global() {
        auto* ch = get_shared_channel();
        if (ch && ch->magic == GEMINI_SHM_MAGIC) {
            if (ch->driver_node[0] != '\0') return ch->driver_node;
            if (ch->driver_connected || ch->is_attached || ch->active_pid > 0) return "/dev/mem_tool";
        }
        return engine::mem::get_driver_node();
    }

    inline int get_target_pid_global() {
        auto* ch = get_shared_channel();
        if (ch && ch->magic == GEMINI_SHM_MAGIC && ch->active_pid > 0) return ch->active_pid;
        return (engine::mem::g_pid > 0) ? engine::mem::g_pid : engine::mem::g_detected_pid;
    }

    inline uintptr_t get_lib_base_global() {
        auto* ch = get_shared_channel();
        if (ch && ch->magic == GEMINI_SHM_MAGIC && ch->lib_base > 0) return ch->lib_base;
        return engine::mem::g_lib_base;
    }

    inline uint64_t get_driver_reads_global() {
        auto* ch = get_shared_channel();
        if (ch && ch->magic == GEMINI_SHM_MAGIC && ch->driver_reads > 0) return ch->driver_reads;
        return engine::mem::get_driver_reads();
    }

    void render();
}
