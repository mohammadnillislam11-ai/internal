#pragma once
#include "../styles/styles.h"
#include <string>

/**
 * @file widgets.hpp
 * @brief Custom BOYHHA UI controls (Toggles and Sliders) styled with Premium Pink.
 */

/**
 * @brief Renders an animated capsule toggle button with outline, circle indicator, and optional crown icon.
 * @param label Button display name.
 * @param v Pointer to boolean state variable.
 * @param has_crown True to render gold crown icon on the right.
 * @return True if button was clicked this frame.
 */
bool BooyahToggle(const char* label, bool* v, bool has_crown = false);

/**
 * @brief Renders a styled slider with title, live percentage/value readout, and pink grabber.
 * @param label Slider display name.
 * @param v Pointer to float value variable.
 * @param v_min Minimum range value.
 * @param v_max Maximum range value.
 * @param fmt Format string for value display (default: "%.2f").
 */
void BooyahSlider(const char* label, float* v, float v_min, float v_max, const char* fmt = "%.2f");
