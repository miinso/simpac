#include "player_module.h"
#include "systems.h"

#include <modules/engine/core/components.h>
#include <modules/engine/input/components.h>
#include <modules/engine/physics/components.h>

namespace player {
    void PlayerModule::register_components(flecs::world world) {}

    void PlayerModule::register_systems(flecs::world world) {
        world.system<const input::InputHorizontal, physics::DesiredVelocity>()
                .term_at(1)
                // .parent() // i don't understand. can i use parent() and cascade() at the same
                // time?? .up() // same thing for parent()
                .cascade()
                // wow.. there's an unfortunate example of `parent().cascade()`. see:
                // flecs/examples/cpp/queries/hierarchy/src/main.cpp
                .each(systems::handle_horizontal_keypress);

        world.system<const input::InputVertical, physics::DesiredVelocity>()
                .term_at(1)
                .cascade()
                .each(systems::handle_vertical_keypress);

        world.system<physics::DesiredVelocity, const core::Speed>().each(
                systems::scale_desired_velocity_system);

        // now i get it. query with a component marked with `.parent()` modifier lists all the
        // entities with the component, then `.cascade()` modifier reorders the list in b-f order,
        // then the system is ran from the root (if one exists) node in b-f order.

        // huh? then why not just `.cascade()` but use both??

        // Q: And I think up() is similar to cascade() but in DFS right?
        // Sander: No, up() just gets you *the* parent component, it doesn't enforce order.

        // so `up()` or `parent()` give you "an" entity (if exists)?
        // and `cascade()` gives you a list of entities (might be empty) in b-f order?

        // from doc: The cascade modifier is similar to up but returns result*s* in breadth-first
        // order.

        // no! doc also says: Cascade, *Same* as Up, but iterate in breadth-first order
        // which one is it??

        // Sander: .parent().cascade() finds *the* component on *a* parent entity, and iterates
        // entiti*es* breadth-first.
        // guy-who-asked: I can't really wrap my head around what this means anyway, ...

        // this might be it:
        // `.parent()` modifier on a term makes the component is from *the* parent. and there might
        // be multiple entities that fits such description. and the system will be ran on those
        // entity(ies) in b-f order accounting relationship tree. done! ..or is it?

        // The problem is that queries currently can't do DFS, it's a lot harder to support than BFS
    }
} // namespace player
