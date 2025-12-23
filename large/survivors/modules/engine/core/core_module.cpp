#include "core_module.h"
#include "components.h"
#include "queries.h"
#include "systems.h"

#include <flecs.h>

namespace core {

    void CoreModule::register_components(flecs::world &world) {
        world.component<Position>();
        world.component<Speed>();
        world.component<GameSettings>().add(flecs::Singleton);
        world.component<Tag>();
        world.component<DestroyAfterTime>();
        world.component<DestroyAfterFrame>();
    }

    flecs::query<Position, Tag> queries::position_and_tag;
    void CoreModule::register_queries(flecs::world &world) {
        queries::position_and_tag = world.query<Position, Tag>();
    }

    void CoreModule::register_systems(flecs::world &world) {
        std::cout << "Registering core systems" << std::endl;

        world.system<DestroyAfterTime>("Destroy entities after time")
                .kind(flecs::PostFrame)
                .write<DestroyAfterFrame>()
                .each(systems::destroy_entity_after_time);

        world.system("Destroy entities after frame")
                .kind(flecs::PostFrame)
                .with<DestroyAfterFrame>()
                .each(systems::destroy_entity_after_frame);

        world.system("Table cleanups").kind(flecs::PostFrame).run([world](flecs::iter &it) {
            systems::remove_empty_tables(world);
        });
    }

} // namespace core
