#define IMGUI_DEFINE_MATH_OPERATORS
#include "bar.hpp"
#include "styles/styles.h"
#include "imgui.h"

/**
 * @file bar.cpp
 * @brief Floating launcher state and helper routines.
 * Floating button rendering is executed directly in ui::menu::render() for 1:1 BOYHHA parity.
 */
namespace ui::bar {

void set_game_alpha(float a) { g_game_alpha = a; }
float game_alpha() { return g_game_alpha; }

/**
 * @brief Renders the floating launcher button (executed inside ui::menu::render()).
 */
void render() {
    // Render loop is consolidated in ui::menu::render() matching BOYHHA XINOX.cpp
}

} // namespace ui::bar
