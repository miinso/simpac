#pragma once

#include "../components.h"
#include <algorithm>

namespace physics::integration {

inline void update_position(Position& x, const Velocity& v, Real dt) {
    x.map() += dt * v.map();
}

inline void ground_collision(Position& x, const OldPosition& x_old, Real dt, Real friction = 0.8) {
    // not used. originally for pbd
    if (x[1] < 0) {
        x[1] = 0;
        auto dx = x[0] - x_old[0];
        auto dz = x[2] - x_old[2];
        auto friction_factor = std::min(Real(1), dt * friction);
        x[0] = x_old[0] + dx * (1 - friction_factor);
        x[2] = x_old[2] + dz * (1 - friction_factor);
    }
}

} // namespace physics::integration
