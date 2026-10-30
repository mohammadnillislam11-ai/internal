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
            cfg::esp::enabled    = esp.value("enabled", false);
            cfg::esp::box        = esp.value("box", false);
            cfg::esp::tracer     = esp.value("tracer", false);
            cfg::esp::skeleton   = esp.value("skeleton", false);
            cfg::esp::health     = esp.value("health", false);
            cfg::esp::name       = esp.value("name", false);
            cfg::esp::distance   = esp.value("distance", false);
            cfg::esp::max_range  = esp.value("max_range", 250.0f);
        }

        if (j.contains("aim")) {
            auto& aim = j["aim"];
            cfg::aim::enabled      = aim.value("enabled", false);
            cfg::aim::silent       = aim.value("silent", false);
            cfg::aim::nobita       = aim.value("nobita", false);
            cfg::aim::target_bone  = aim.value("target_bone", 0);
            cfg::aim::trigger_type = aim.value("trigger_type", 0);
            cfg::aim::draw_fov     = aim.value("draw_fov", false);
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
        esp["enabled"]    = cfg::esp::enabled;
        esp["box"]        = cfg::esp::box;
        esp["tracer"]     = cfg::esp::tracer;
        esp["skeleton"]   = cfg::esp::skeleton;
        esp["health"]     = cfg::esp::health;
        esp["name"]       = cfg::esp::name;
        esp["distance"]   = cfg::esp::distance;
        esp["max_range"]  = cfg::esp::max_range;
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
