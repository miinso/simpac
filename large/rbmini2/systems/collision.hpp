// systems/collision.hpp
#pragma once
#include "collision/broadphase.hpp"
#include "collision/components.hpp"
#include "collision/narrowphase.hpp"
#include "collision/resolution.hpp"
#include "collision/types.hpp"
#include "geometry/components.hpp"
#include "util/logging.hpp"
#include <flecs.h>

using namespace phys::components;

namespace phys {
    namespace systems {
        void update_bounding_volumes(flecs::iter& it,
                                     size_t i,
                                     BoundingVolume& volume,
                                     const WorldMesh& mesh,
                                     const Position& position,
                                     const Scale& scale) {
            volume.aabb = collision::compute_mesh_aabb(
                mesh.vertices.getVertices(),
                position.value,
                Quaternionr::Identity(),
                scale.value,
                0.01f // margin
            );

            log_info("update_bounding_volumes::%d", it.entity(i).view().id());
        }

        void generate_collision_pairs(flecs::iter& it) {
            // discard existing pairs
            it.world().each<collision::CollisionPair>(
                [](flecs::entity e, collision::CollisionPair&) { e.destruct(); });

            // collect collision pair
            auto query = it.world().query<const BoundingVolume>();
            query.each([&](flecs::entity e1, const BoundingVolume& vol1) {
                query.each([&](flecs::entity e2, const BoundingVolume& vol2) {
                    if (e1 < e2) {
                        if (vol1.aabb.overlaps(vol2.aabb)) {
                            it.world().entity().set<collision::CollisionPair>({e1, e2});
                            log_info(
                                "generate_collision_pairs::%d, %d", e1.view().id(), e2.view().id());
                        }
                    }
                });
            });
        }

        void detect_collisions(flecs::iter& it) {
            it.world().each<collision::ContactConstraint>(
                [](flecs::entity e, collision::ContactConstraint&) { e.destruct(); });

            it.world().each<collision::CollisionPair>(
                [&](flecs::entity, const collision::CollisionPair& pair) {
                    // traverse bvh then do sdf-sdf test
                    // combination of {box, sphere, sdf}
                });
        }

        void solve_contacts(flecs::iter& it) {
            const Real dt = it.delta_time();
            const int iteration_count = 10;

            for (int iter = 0; iter < iteration_count; ++iter) {
                it.world().each<collision::ContactConstraint>(
                    [&](collision::ContactConstraint& constraint) {
                        // TODO: add some iterative methods
                    });
            }
        }

        void register_collision_systems(flecs::world& ecs) {
            ecs.system<BoundingVolume, const WorldGeometry, const Position, const Scale>()
                .kind(flecs::PreUpdate)
                .each(update_bounding_volumes);

            ecs.system().kind(flecs::PreUpdate).run(generate_collision_pairs);

            ecs.system().kind(flecs::OnUpdate).each(detect_collisions);

            // ecs.system().kind(flecs::OnUpdate).each(solve_contacts);
        }
    } // namespace systems
} // namespace phys