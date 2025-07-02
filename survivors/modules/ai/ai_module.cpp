#include "ai_module.h"
#include "components.h"
#include "systems.h"

#include <modules/engine/core/components.h>
#include <modules/engine/physics/components.h>

namespace ai {
    void AIModule::register_components(flecs::world world) {
        world.component<Target>();
        world.component<FollowTarget>();
        world.component<StoppingDistance>();
    }

    void AIModule::register_systems(flecs::world world) {
        world.system<const core::Position, const core::Speed, physics::DesiredVelocity>(
                     "Follow target")
                .with<Target>(flecs::Wildcard)
                .with<FollowTarget>()
                .each(systems::follow_target);

        world.system<const StoppingDistance, const core::Position, physics::DesiredVelocity>(
                     "Stop moving if within proximity to the target")
                .with<Target>(flecs::Wildcard)
                .each(systems::stop_moving_near_target);
    }
} // namespace ai
