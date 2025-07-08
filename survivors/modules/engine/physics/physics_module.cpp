#include "physics_module.h"
#include "components.h"
#include "pipelines.h"
#include "queries.h"
#include "systems.h"


#include <modules/engine/core/components.h>
#include <modules/engine/rendering/components.h>

namespace physics {
    void PhysicsModule::register_components(flecs::world &world) {
        world.component<Velocity>();
        world.component<AccelerationSpeed>();
        world.component<CollidedWith>();
    }

    flecs::query<core::Position, Collider> visible_collision_bodies_query;
    flecs::query<core::Position, Collider> box_collider_query;
    void PhysicsModule::register_queries(flecs::world &world) {
        queries::visible_collision_bodies_query =
                world.query_builder<core::Position, Collider>().with<rendering::Visible>().build();
        queries::box_collider_query =
                world.query_builder<core::Position, Collider>().with<BoxCollider>().build();
    }

    void PhysicsModule::register_systems(flecs::world &world) {
        m_physicsTick = world.timer().interval(PHYSICS_TICK_LENGTH);

        world.system<const Velocity, DesiredVelocity>("Reset desired velocity")
                .kind(flecs::PreUpdate)
                .tick_source(m_physicsTick)
                .each(systems::reset_desired_velocity);

        world.system<Velocity, const DesiredVelocity, const AccelerationSpeed>("Integrate velocity")
                .kind<Integration>()
                .tick_source(m_physicsTick)
                .each(systems::integrate_velocity);

        world.system<core::Position, const Velocity>("Integrate position")
                .kind<Integration>()
                .tick_source(m_physicsTick)
                .each(systems::integrate_position);

        world.system<CollisionRecordList, const core::Position, const Collider>(
                     "Detect discrete collision (static??)")
                .term_at(0)
                .with<StaticCollider>()
                .singleton()
                .kind<CollisionDetection>()
                .tick_source(m_physicsTick)
                .each(systems::detect_collision);

        world.system<CollisionRecordList, const core::Position, const Collider>(
                     "Detect discrete collision (non-static??)")
                .term_at(0)
                .singleton()
                .with<rendering::Visible>()
                .kind<CollisionDetection>()
                .tick_source(m_physicsTick)
                .each(systems::detect_collision_alt);

        world.system<CollisionRecordList>("Resolve collision")
                .term_at(0)
                .singleton()
                .kind<CollisionResolution>()
                .tick_source(m_physicsTick)
                .each(systems::resolve_collision);

        world.system<CollisionRecordList>("Create collision relationship")
                .term_at(0)
                .singleton()
                .kind<CollisionResolution>()
                .tick_source(m_physicsTick)
                .each(systems::create_collision_relationship);

        world.system("Delete collision relationship")
                .with<Collider>()
                .kind<CollisionCleanup>()
                .tick_source(m_physicsTick)
                .each(systems::delete_collision_relationship);

        world.system<CollisionRecordList>("Clean collision record list")
                .term_at(0)
                .singleton()
                .kind<CollisionCleanup>()
                .tick_source(m_physicsTick)
                .each(systems::clean_collision_record);
    }

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
