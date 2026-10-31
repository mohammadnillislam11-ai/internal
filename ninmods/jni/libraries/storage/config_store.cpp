#include "config_store.h"
#include "../../menu/config.hpp"
#include "../network/json/json.hpp"
#include <fstream>
#include <cstdio>

namespace storage {

namespace {
    static const char* CONFIG_FILE_PATH = "/data/local/tmp/ninmods_config.json";
}

bool load_config() {
    std::ifstream file(CONFIG_FILE_PATH);
    if (!file.is_open()) return false;

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("esp")) {
            auto& esp = j["esp"];
            // Strict Security Policy: All cheat feature toggles must start strictly OFF (false).
            // No automated activation on cold boot. Only user-adjusted sliders are restored.
            cfg::esp::enabled    = false;
            cfg::esp::box        = false;
            cfg::esp::tracer     = false;
            cfg::esp::skeleton   = false;
            cfg::esp::health     = false;
            cfg::esp::name       = false;
            cfg::esp::distance   = false;
            cfg::esp::armor      = false;
            cfg::esp::weapon     = false;
            cfg::esp::bot_tag    = false;
            cfg::esp::crosshair  = false;
            cfg::esp::firing_ind = false;
            cfg::esp::loot       = false;

            cfg::esp::box_type      = esp.value("box_type", 0);
            cfg::esp::box_thickness = esp.value("box_thickness", 1.0f);
            cfg::esp::box_rounding  = esp.value("box_rounding", 2.0f);
            cfg::esp::max_range     = esp.value("max_range", 250.0f);
            cfg::esp::loot_range    = esp.value("loot_range", 150.0f);
        }

        if (j.contains("aim")) {
            auto& aim = j["aim"];
            // Strict Security Policy: All aim toggles start strictly OFF (false).
            cfg::aim::enabled      = false;
            cfg::aim::silent       = false;
            cfg::aim::nobita       = false;
            cfg::aim::draw_fov     = false;

            cfg::aim::target_bone  = aim.value("target_bone", 0);
            cfg::aim::trigger_type = aim.value("trigger_type", 0);
            cfg::aim::fov          = aim.value("fov", 120.0f);
            cfg::aim::distance     = aim.value("distance", 250.0f);
            cfg::aim::smooth       = aim.value("smooth", 8.0f);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool save_config() {
    try {
        nlohmann::json j;

        nlohmann::json esp;
        esp["enabled"]       = cfg::esp::enabled;
        esp["box"]           = cfg::esp::box;
        esp["box_type"]      = cfg::esp::box_type;
        esp["box_thickness"] = cfg::esp::box_thickness;
        esp["box_rounding"]  = cfg::esp::box_rounding;
        esp["tracer"]        = cfg::esp::tracer;
        esp["skeleton"]      = cfg::esp::skeleton;
        esp["health"]        = cfg::esp::health;
        esp["armor"]         = cfg::esp::armor;
        esp["name"]          = cfg::esp::name;
        esp["distance"]      = cfg::esp::distance;
        esp["weapon"]        = cfg::esp::weapon;
        esp["bot_tag"]       = cfg::esp::bot_tag;
        esp["crosshair"]     = cfg::esp::crosshair;
        esp["firing_ind"]    = cfg::esp::firing_ind;
        esp["loot"]          = cfg::esp::loot;
        esp["max_range"]     = cfg::esp::max_range;
        esp["loot_range"]    = cfg::esp::loot_range;
        j["esp"] = esp;

        nlohmann::json aim;
        aim["enabled"]      = cfg::aim::enabled;
        aim["silent"]       = cfg::aim::silent;
        aim["nobita"]       = cfg::aim::nobita;
        aim["target_bone"]  = cfg::aim::target_bone;
        aim["trigger_type"] = cfg::aim::trigger_type;
        aim["draw_fov"]     = cfg::aim::draw_fov;
        aim["fov"]          = cfg::aim::fov;
        aim["distance"]     = cfg::aim::distance;
        aim["smooth"]       = cfg::aim::smooth;
        j["aim"] = aim;

        std::ofstream file(CONFIG_FILE_PATH);
        if (!file.is_open()) return false;

        file << j.dump(4);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace storage
