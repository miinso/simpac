#pragma once

#include <flecs.h>

#include <modules/engine/core/components.h>
#include <modules/engine/physics/components.h>

#include "components.h"

namespace ai {
    namespace systems {
        inline void follow_target(flecs::iter &it, size_t i, const core::Position &position,
                                  const core::Speed &speed, physics::DesiredVelocity &velocity) {
            const auto target = it.pair(3).second();

            if (!target.is_valid())
                return;

            const auto dir =
                    (target.try_get<core::Position>()->value - position.value).normalized();
            velocity.value = dir * speed.value;
        };

        inline void stop_moving_near_target(flecs::iter &it, size_t i,
                                            const StoppingDistance &distance,
                                            const core::Position &pos,
                                            physics::DesiredVelocity &velocity) {
            const auto target = it.pair(3).second();

            if (!target.is_valid())
                return;

            const auto delta = target.try_get<core::Position>()->value - pos.value;
            const float distSquared = delta.squaredNorm();

            if (distSquared < distance.value * distance.value) {
                velocity.value = Eigen::Vector3f::Zero();
            }
        };
    } // namespace systems
} // namespace ai
