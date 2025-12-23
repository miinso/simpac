#include "input_module.h"
#include "components.h"
#include "systems.h"

namespace input {
    void InputModule::register_components(flecs::world &world) {
        world.component<InputHorizontal>();
        world.component<InputVertical>();
        world.component<KeyBinding>();
    }

    void InputModule::register_systems(flecs::world &world) {
        world.system<const KeyBinding, InputHorizontal>("set horizontal input")
                .term_at(1)
                .cascade() // traverse upward the entity hierarchy in breadth-first order, find the
                           // first parent entities with `InputHorizontal(term_at(1))` component.
                           // interpretation: find any entities with `KeyBinding`, but their parent
                           // must have `InputHorizontal`, and give me the refs for both components.
                .kind(flecs::PreUpdate)
                .each(systems::set_horizontal_input);

        world.system<const KeyBinding, InputVertical>("set vertical input")
                .term_at(1)
                .cascade()
                .kind(flecs::PreUpdate)
                .each(systems::set_vertical_input);

        world.system<InputHorizontal>("Reset Input Horizontal")
                .kind(flecs::PostUpdate)
                .each(systems::reset_horizontal_input);

        world.system<InputVertical>("Reset Input Vertical")
                .kind(flecs::PostUpdate)
                .each(systems::reset_vertical_input);
    }
} // namespace input
