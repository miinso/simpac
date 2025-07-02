#pragma once

#include <modules/engine/core/components.h>
#include <modules/engine/input/components.h>
#include <modules/engine/physics/components.h>

namespace player {
    namespace systems {
        inline void handle_horizontal_keypress(const input::InputHorizontal &horizontal,
                                               physics::DesiredVelocity &desired_vel) {
            desired_vel.value.x() = horizontal.value;
        }

        inline void handle_vertical_keypress(const input::InputVertical &vertical,
                                             physics::DesiredVelocity &desired_vel) {
            desired_vel.value.y() = vertical.value;
        }

        inline void scale_desired_velocity_system(physics::DesiredVelocity &velocity,
                                                  const core::Speed &speed) {
            velocity.value = velocity.value.normalized() * speed.value;
        }
    } // namespace systems
} // namespace player
