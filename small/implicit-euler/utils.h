// =========================================================================
// Cloth component registration and creation helpers
// =========================================================================
#pragma once

#include "components.h"
#include <flecs.h>
#include <cmath>

// =========================================================================
// Internal: Build cloth geometry (particles + springs) as children
// =========================================================================

namespace detail {

inline void build_cloth_geometry(flecs::entity cloth_entity, GridCloth& cloth) {
    auto world = cloth_entity.world();
    world.defer_suspend();

    // Get current global particle index (for solver indexing)
    int particle_base_idx = world.get<Scene>().num_particles();

    // Create particles in grid as children of cloth entity
    std::vector<flecs::entity> particles;
    particles.resize(cloth.width * cloth.height);

    Vector3r offset(cloth.offset[0], cloth.offset[1], cloth.offset[2]);

    for (int y = 0; y < cloth.height; y++) {
        for (int x = 0; x < cloth.width; x++) {
            int local_idx = y * cloth.width + x;
            int global_idx = particle_base_idx + local_idx;

            Vector3r pos = offset + Vector3r(
                x * cloth.spacing,
                0.0f,
                -y * cloth.spacing
            );

            particles[local_idx] = world.entity()
                .child_of(cloth_entity)  // make it a child
                .set(Position{pos})
                .set(OldPosition{pos})
                .set(Velocity{Vector3r::Zero()})
                .set(Acceleration{Vector3r::Zero()})
                .set(Mass{cloth.mass})
                .set(InverseMass{1.0f / cloth.mass})
                .set(ParticleIndex{global_idx})
                .add<Particle>();
        }
    }

    cloth.particle_count = cloth.width * cloth.height;

    // Helper to get particle at grid position
    auto get_particle = [&](int x, int y) -> flecs::entity {
        if (x < 0 || x >= cloth.width || y < 0 || y >= cloth.height)
            return flecs::entity::null();
        return particles[y * cloth.width + x];
    };

    // Create springs as children of cloth entity
    int spring_count = 0;

    for (int y = 0; y < cloth.height; y++) {
        for (int x = 0; x < cloth.width; x++) {
            auto current = get_particle(x, y);

            // Structural springs (horizontal)
            if (x < cloth.width - 1) {
                auto right = get_particle(x + 1, y);
                world.entity()
                    .child_of(cloth_entity)
                    .set(Spring{
                        current, right,
                        cloth.spacing,
                        cloth.k_structural,
                        cloth.k_damping
                    });
                spring_count++;
            }

            // Structural springs (vertical)
            if (y < cloth.height - 1) {
                auto down = get_particle(x, y + 1);
                world.entity()
                    .child_of(cloth_entity)
                    .set(Spring{
                        current, down,
                        cloth.spacing,
                        cloth.k_structural,
                        cloth.k_damping
                    });
                spring_count++;
            }

            // Shear springs (diagonals)
            if (x < cloth.width - 1 && y < cloth.height - 1) {
                auto diagonal = get_particle(x + 1, y + 1);
                Real diag_len = cloth.spacing * std::sqrt(2.0f);

                world.entity()
                    .child_of(cloth_entity)
                    .set(Spring{
                        current, diagonal,
                        diag_len,
                        cloth.k_shear,
                        cloth.k_damping
                    });
                spring_count++;

                auto right = get_particle(x + 1, y);
                auto down = get_particle(x, y + 1);
                world.entity()
                    .child_of(cloth_entity)
                    .set(Spring{
                        right, down,
                        diag_len,
                        cloth.k_shear,
                        cloth.k_damping
                    });
                spring_count++;
            }

            // Bending springs (skip one particle)
            if (x < cloth.width - 2) {
                auto right2 = get_particle(x + 2, y);
                world.entity()
                    .child_of(cloth_entity)
                    .set(Spring{
                        current, right2,
                        cloth.spacing * 2.0f,
                        cloth.k_bending,
                        cloth.k_damping * 0.5f
                    });
                spring_count++;
            }

            if (y < cloth.height - 2) {
                auto down2 = get_particle(x, y + 2);
                world.entity()
                    .child_of(cloth_entity)
                    .set(Spring{
                        current, down2,
                        cloth.spacing * 2.0f,
                        cloth.k_bending,
                        cloth.k_damping * 0.5f
                    });
                spring_count++;
            }
        }
    }

    cloth.spring_count = spring_count;

    // Apply pinning based on pin_mode
    PinMode mode = static_cast<PinMode>(cloth.pin_mode);

    switch (mode) {
        case PinMode::Corners:
            get_particle(0, 0).set(InverseMass{0}).add<IsPinned>();
            get_particle(cloth.width - 1, 0).set(InverseMass{0}).add<IsPinned>();
            break;

        case PinMode::TopRow:
            for (int x = 0; x < cloth.width; x++) {
                get_particle(x, 0).set(InverseMass{0}).add<IsPinned>();
            }
            break;

        case PinMode::None:
            // No pinning
            break;
    }
    world.defer_resume();
}

inline void destroy_cloth_geometry(flecs::entity cloth_entity) {
    // Children are automatically deleted when parent is deleted in flecs
    // But if we need explicit cleanup (e.g., for re-creation), we can do it here
    cloth_entity.world().defer_suspend();
    cloth_entity.children([](flecs::entity child) {
        child.destruct();
    });
    cloth_entity.world().defer_resume();
}

} // namespace detail

// =========================================================================
// Register GridCloth component with hooks and reflection
// =========================================================================

inline void register_cloth_component(flecs::world& ecs) {
    
    // Register hooks first (typed component)
    ecs.component<GridCloth>()
        .on_set([&](flecs::entity e, GridCloth& cloth) {
            // Destroy existing geometry if any (for re-creation on parameter change)
            detail::destroy_cloth_geometry(e);
            // Build new geometry
            detail::build_cloth_geometry(e, cloth);
        })
        .on_remove([](flecs::entity e, GridCloth& cloth) {
            // Explicit cleanup (though children should auto-delete)
            detail::destroy_cloth_geometry(e);
        });

    // Add reflection separately (becomes untyped_component)
    ecs.component<GridCloth>()
        .member<int>("width")
        .member<int>("height")
        .member<float>("spacing")
        .member<float>("mass")
        .member<float>("k_structural")
        .member<float>("k_shear")
        .member<float>("k_bending")
        .member<float>("k_damping")
        .member<float>("offset", 3)
        .member<int>("pin_mode")
        .member<int>("particle_count")
        .member<int>("spring_count");

}
