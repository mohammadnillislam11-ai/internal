#include "config_store.h"
#include "../../menu/config.hpp"
#include <cstdio>
#include <unistd.h>

namespace storage {

namespace {
    static const char* CONFIG_FILE_PATH = "/data/local/tmp/ninmods_config.json";
}

bool load_config() {
    // Purge any stale legacy config file from disk to ensure zero residual state
    unlink(CONFIG_FILE_PATH);

    // Strict In-Memory Session Policy:
    // Every option starts 100% OFF (false) on each launch.
    // Options only turn ON when explicitly clicked in the menu during this session.
    // When the terminal is restarted, everything resets to OFF.
    cfg::esp::enabled    = false;
    cfg::esp::box        = false;
    cfg::esp::box_type   = 0;
    cfg::esp::box_thickness = 1.0f;
    cfg::esp::box_rounding  = 2.0f;
    cfg::esp::max_range     = 250.0f;
    cfg::esp::loot_range    = 150.0f;
    cfg::esp::name       = false;
    cfg::esp::health     = false;
    cfg::esp::armor      = false;
    cfg::esp::distance   = false;
    cfg::esp::weapon     = false;
    cfg::esp::bot_tag    = false;
    cfg::esp::tracer     = false;
    cfg::esp::skeleton   = false;
    cfg::esp::crosshair  = false;
    cfg::esp::firing_ind = false;
    cfg::esp::loot       = false;

    cfg::aim::enabled      = false;
    cfg::aim::silent       = false;
    cfg::aim::nobita       = false;
    cfg::aim::draw_fov     = false;
    cfg::aim::target_bone  = 0;
    cfg::aim::trigger_type = 0;
    cfg::aim::fov          = 120.0f;
    cfg::aim::distance     = 250.0f;
    cfg::aim::smooth       = 8.0f;

    return true;
}

bool save_config() {
    // Intentionally no-op: No saving to ninmods_config.json.
    // Every restart opens fresh with all cheat options strictly OFF.
    return true;
}

} // namespace storage
