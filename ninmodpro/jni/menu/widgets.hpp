#pragma once
#include "imgui.h"

/**
 * @file widgets.hpp
 * @brief Standard animated and interactive widgets for ninmods client.
 */

namespace ui::widgets {

bool Toggle(const char* label, bool* v, bool has_crown = false);
void Slider(const char* label, float* v, float v_min, float v_max, const char* fmt = "%.1f");
void Combo(const char* label, int* current_item, const char* const items[], int items_count);
void ColorEdit(const char* label, ImVec4* col);

} // namespace ui::widgets
