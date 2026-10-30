#pragma once
#include "aheasing.h"
#include <cmath>
#include <algorithm>

/**
 * @file animator.h
 * @brief Animation driver powered by official Robert Penner easing equations (AHEasing).
 */
namespace anim {

    struct ValueAnimator {
        float current_value{0.0f};
        float target_value{0.0f};
        float speed{12.0f};

        void set(float target) {
            target_value = target;
        }

        void snap_to(float val) {
            current_value = val;
            target_value = val;
        }

        float update(float delta_time) {
            float diff = target_value - current_value;
            float factor = 1.0f - std::exp(-speed * delta_time);
            current_value += diff * factor;
            return current_value;
        }

        bool is_finished(float epsilon = 0.001f) const {
            return std::abs(current_value - target_value) < epsilon;
        }
    };

} // namespace anim
