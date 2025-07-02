#pragma once

#include <flecs.h>
#include <raylib.h>

#include "components.h"

namespace input {
    namespace systems {
        inline void set_horizontal_input(const KeyBinding &binding, InputHorizontal &horizontal) {
            if (IsKeyDown(binding.key)) {
                horizontal.value += binding.value;
            }
        }

        inline void set_vertical_input(const KeyBinding &binding, InputVertical &vertical) {
            if (IsKeyDown(binding.key)) {
                vertical.value += binding.value;
            }
        }

        inline void reset_horizontal_input(InputHorizontal &horizontal) { horizontal.value = 0.0f; }

        inline void reset_vertical_input(InputVertical &vertical) { vertical.value = 0.0f; }
    } // namespace systems
} // namespace input
