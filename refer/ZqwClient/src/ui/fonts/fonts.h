#pragma once

#include "imgui.h"
#include <cmath>

namespace fonts
{
    /**
     * @brief Initializes and loads font sets into Dear ImGui atlas.
     */
    void init();

    /**
     * @brief Cleans up font references.
     */
    void shutdown();

    /**
     * @brief Returns pixel font size.
     */
    float size(ImFont* font);

    inline float snap(float v) { return floorf(v + 0.5f); }

    /**
     * @brief Resolves Inter text font by design size.
     */
    ImFont* inter(float design_size, bool bold = false);

    /**
     * @brief Resolves Fontello icon font by design size.
     */
    ImFont* icon(float design_size);
}

