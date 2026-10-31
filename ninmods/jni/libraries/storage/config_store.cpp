#include "config_store.h"
#include "../../menu/config.hpp"

namespace storage {

bool load_config() {
    // Pure real-time in-memory: All cheat toggles strictly start OFF (false).
    cfg::esp::enabled       = false;
    cfg::esp::box           = false;
    cfg::esp::box_type      = 0;
    cfg::esp::box_thickness = 1.8f;
    cfg::esp::box_rounding  = 3.0f;
    cfg::esp::max_range     = 250.0f;
    cfg::esp::tracer        = false;
    cfg::esp::health        = false;

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
    // Pure real-time: No persistence to disk.
    return true;
}

} // namespace storage
