#include "physics_module.h"
#include "components.h"
#include "pipelines.h"
#include "queries.h"
#include "systems.h"

#include <modules/engine/core/components.h>
#include <modules/engine/rendering/components.h>

using namespace core;

namespace physics {

    void PhysicsModule::register_components(flecs::world &world) {
        world.component<gravity>().add(flecs::Singleton);
        world.component<v_max>().add(flecs::Singleton);
        world.component<CollisionRecordList>().add(flecs::Singleton);
    }

    ///// queries


    void PhysicsModule::register_queries(flecs::world &world) {}
    ///// queries

    ///// systems

    void PhysicsModule::register_systems(flecs::world &world) {
        m_physics_tick = world.timer().interval(PHYSICS_TICK_LENGTH);

        world.system<particle_q, particle_qd, particle_f, particle_inv_mass,
                     gravity, v_max>("Integrate particle")
                .kind<Integration>()
                .with<PARTICLE_FLAG_ACTIVE>()
                .tick_source(m_physics_tick)
                .each(systems::integrate_particle);

        world.system<CollisionRecordList, const core::Position, const Collider>(
                     "Detect collision (non-static?)")
                .kind<CollisionDetection>()
                .tick_source(m_physics_tick)
                .each(systems::detect_collision_alt);

        world.system<CollisionRecordList>("Resolve collision")
                .kind<CollisionResolution>()
                .tick_source(m_physics_tick)
                .each(systems::resolve_collision);

        world.system<CollisionRecordList>("Clean collision record list")
                .kind<CollisionCleanup>()
                .tick_source(m_physics_tick)
                .each(systems::clean_collision_record);
    }
    ///// systems

    void PhysicsModule::register_pipeline(flecs::world &world) {
        world.component<Integration>().add(flecs::Phase).depends_on(flecs::OnUpdate);
        world.component<CollisionDetection>().add(flecs::Phase).depends_on(flecs::OnValidate);
        world.component<CollisionResolution>().add(flecs::Phase).depends_on(flecs::PostUpdate);
        world.component<CollisionCleanup>().add(flecs::Phase).depends_on<CollisionResolution>();

        // "flecs built-ins"

        // OnStart
        // OnLoad
        // PostLoad
        // PreUpdate
        // OnUpdate
        //      Integration
        // OnValidate
        //      CollisionDetection
        //          OnCollisionDetected
        //          PostCollisionDetected
        // PostUpdate
        //      CollisionResolution
        //      CollisionCleanup
        // PreStore
        // OnStore
        //      PreRender
        //      Render
        //      RenderGUI
        //      PostRender
    }
} // namespace physics
