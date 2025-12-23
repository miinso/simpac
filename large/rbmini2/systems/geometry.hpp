// systems/geometry.hpp
#pragma once
#include "core/components.hpp"
#include "geometry/components.hpp"
#include "geometry/operations.hpp"
#include <flecs.h>

using namespace phys::components;

namespace phys {
    namespace systems {

        void update_world_mesh(flecs::iter& it,
                               size_t,
                               MeshFilter& filter,
                               const Position& position,
                               const Orientation& orientation,
                               const Scale& scale) {
            filter.transform_vertices(position.value, orientation.value, scale.value, filter);
        }

        void update_mesh_renderer(
            flecs::iter& it, size_t, MeshRenderer& renderer, const MeshFilter& filter) {
            renderer.update_renderable(filter);
        }

        void register_geometry_systems(flecs::world& ecs) {

            ecs.system<MeshFilter, const Position, const Orientation, const Scale>(
                   "UpdateWorldMesh")
                .kind(flecs::PreUpdate)
                .each(update_world_mesh);

            ecs.system<MeshRenderer, const MeshFilter>("UpdateMeshRenderer")
                .kind(flecs::PostUpdate)
                .each(update_mesh_renderer);
        }

    } // namespace systems
} // namespace phys