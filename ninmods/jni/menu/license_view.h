#pragma once
#include <string>

namespace ui::license {
    inline bool g_licensed = false;
    inline std::string g_expiration_date = "";
    inline int g_days_left = 0;
    inline int g_exp_year = 0;
    inline int g_exp_month = 0;
    inline int g_exp_day = 0;

    /**
     * @brief Render the License / Key Verification modal window.
     * @return True if license is valid and active, False if still awaiting activation.
     */
    bool render();
}
