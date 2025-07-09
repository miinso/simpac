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
        world.component<ContainedIn>().add(
                flecs::DontFragment); // not sure what exactly and how effectively it does
    }

    ///// queries
    flecs::query<core::Position, Collider> queries::visible_collision_bodies_query;
    flecs::query<core::Position, Collider> queries::box_collider_query;

    void PhysicsModule::register_queries(flecs::world &world) {
        queries::visible_collision_bodies_query =
                world.query_builder<core::Position, Collider>().with<rendering::Visible>().build();
        queries::box_collider_query =
                world.query_builder<core::Position, Collider>().with<BoxCollider>().build();
    }
    ///// queries

    ///// systems
    flecs::system PhysicsModule::m_collision_detection_baseline;
    flecs::system PhysicsModule::m_collision_detection_with_hash_grid;
    flecs::system PhysicsModule::m_collision_detection_with_hash_grid_alt;

    void PhysicsModule::register_systems(flecs::world &world) {
        m_physics_tick = world.timer().interval(PHYSICS_TICK_LENGTH);

        world.system<HashGrid, core::GameSettings>("Init hash grid")
                .term_at(0)
                .singleton()
                .term_at(1)
                .singleton()
                .kind(flecs::OnStart)
                .each(systems::init_hash_grid);

        world.system<HashGrid, core::GameSettings>("Flush and init hash grid on window resized")
                .term_at(0)
                .singleton()
                .term_at(1)
                .singleton()
                .kind(flecs::OnUpdate)
                .each(systems::reset_hash_grid_on_window_resize);

        world.observer<HashGrid, core::GameSettings>("Flush and init hash grid")
                .term_at(1)
                .singleton()
                .event(flecs::OnSet)
                .each(systems::reset_hash_grid);

        world.system<HashGrid, rendering::TrackingCamera, core::GameSettings, GridCell>(
                     "Flush grid cell")
                .term_at(0)
                .singleton()
                .term_at(1)
                .singleton()
                .term_at(2)
                .singleton()
                .kind(flecs::PreUpdate)
                .each(systems::empty_hash_grid_cell_on_camera_move);

        world.system<const Velocity, DesiredVelocity>("Reset desired velocity")
                .kind(flecs::PreUpdate)
                .tick_source(m_physics_tick)
                .each(systems::reset_desired_velocity);

        world.system<Velocity, const DesiredVelocity, const AccelerationSpeed>("Integrate velocity")
                .kind<Integration>()
                .tick_source(m_physics_tick)
                .each(systems::integrate_velocity);

        world.system<core::Position, const Velocity>("Integrate position")
                .kind<Integration>()
                .tick_source(m_physics_tick)
                .each(systems::integrate_position);

        world.system<HashGrid, Collider, core::Position>("Populate hash grid cell")
                .term_at(0)
                .singleton()
                .without<StaticCollider>()
                .kind<Integration>()
                .each(systems::populate_hash_grid_cell);

        world.system<CollisionRecordList, const core::Position, const Collider>(
                     "Detect collision (static?)")
                .term_at(0)
                .with<StaticCollider>()
                .singleton()
                .kind<CollisionDetection>()
                .tick_source(m_physics_tick)
                .each(systems::detect_collision);

        m_collision_detection_baseline =
                world.system<CollisionRecordList, const core::Position, const Collider>(
                             "Detect collision (non-static?)")
                        .term_at(0)
                        .singleton()
                        .with<rendering::Visible>()
                        .kind<CollisionDetection>()
                        .tick_source(m_physics_tick)
                        .each(systems::detect_collision_alt);
        m_collision_detection_baseline.disable();

        m_collision_detection_with_hash_grid =
                world.system<CollisionRecordList, HashGrid, GridCell>(
                             "Detect collision with spatial hashing (non-static?)")
                        .term_at(0)
                        .singleton()
                        .term_at(1)
                        .singleton()
                        .kind<CollisionDetection>()
                        .tick_source(m_physics_tick)
                        .each(systems::detect_collision_with_hash_grid);
        m_collision_detection_with_hash_grid.disable();

        m_collision_detection_with_hash_grid_alt =
                world.system<CollisionRecordList, HashGrid, GridCell>(
                             "Detect collision with spatial hashing + `flecs::relationship`")
                        .term_at(0)
                        .singleton()
                        .term_at(1)
                        .singleton()
                        .kind<CollisionDetection>()
                        .tick_source(m_physics_tick)
                        .each(systems::detect_collision_with_hash_grid_alt);
        m_collision_detection_with_hash_grid_alt.disable();

        world.system<CollisionRecordList>("Resolve collision")
                .term_at(0)
                .singleton()
                .kind<CollisionResolution>()
                .tick_source(m_physics_tick)
                .each(systems::resolve_collision);

        world.system<CollisionRecordList>("Create collision relationship")
                .term_at(0)
                .singleton()
                .kind<CollisionResolution>()
                .tick_source(m_physics_tick)
                .each(systems::create_collision_relationship);

        world.system("Delete collision relationship")
                .with<Collider>()
                .kind<CollisionCleanup>()
                .tick_source(m_physics_tick)
                .each(systems::delete_collision_relationship);

        world.system<CollisionRecordList>("Clean collision record list")
                .term_at(0)
                .singleton()
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
