#pragma once

#include "components.h"
#include "../render/render.h"
#include "../render/interaction.h"
#include "graphics.h"

namespace cloth {

struct interaction {
    interaction(flecs::world& ecs) {
        ecs.import<cloth::render>();
        ecs.component<ParticleInteractionState>()
            .add(flecs::Singleton);
        ecs.component<TriangleInteractionState>()
            .add(flecs::Singleton);

        ecs.set<ParticleInteractionState>({});
        ecs.set<TriangleInteractionState>({});

        ecs.system("PickParticles")
            .kind(flecs::OnLoad)
            .run(systems::pick_particles)
            .disable(0);

        ecs.system("DragParticlesKinematic")
            .kind(graphics::PreRender)
            .run(systems::drag_particles_kinematic)
            .disable(0);

        ecs.system("DragParticlesSpring")
            .kind(graphics::PreRender)
            .run(systems::drag_particles_spring)
            .disable();

        ecs.system("DrawDragPlane")
            .kind(graphics::OnRender)
            .run(systems::draw_drag_plane_debug)
            .disable(0);

        ecs.system("PickTriangles")
            .kind(flecs::OnLoad)
            .run(systems::pick_triangles)
            .disable(0);

        ecs.system("DragTrianglesKinematic")
            .kind(graphics::PreRender)
            .run(systems::drag_triangles_kinematic)
            .disable(0);

        ecs.system("DragTrianglesSpring")
            .kind(graphics::PreRender)
            .run(systems::drag_triangles_spring)
            .disable();

        ecs.system("DrawDragPlaneTri")
            .kind(graphics::OnRender)
            .run(systems::draw_drag_plane_debug_tri)
            .disable(0);
    }
};

} // namespace cloth
