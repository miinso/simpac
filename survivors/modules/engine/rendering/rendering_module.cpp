#include "rendering_module.h"
#include "components.h"
#include "pipelines.h"
#include "queries.h"
#include "systems.h"

#include <modules/engine/core/components.h>
#include <modules/engine/rendering/gui/gui_module.h>

#include <raylib.h>

namespace rendering {
    void RenderingModule::register_components(flecs::world world) { world.component<Priority>(); }

    void RenderingModule::register_systems(flecs::world world) {
        world.system("Before Draw").kind<PreRender>().run(systems::begin_drawing);

        world.system<const core::Position, const Renderable, const core::GameSettings>(
                     "Determine visible entities")
                .term_at(2)
                .singleton()
                .write<Visible>()
                .kind<PreRender>()
                .each(systems::determine_visible_entity);

        world.system<const Renderable, const core::Position, const Rotation *>(
                     "Draw entities with texture")
                .kind<Render>()
                .with<Visible>()
                .group_by<Priority>()
                .each(systems::draw_entity_with_texture);

        world.system<ProgressBar, const core::Position, const Renderable>("Draw health bar")
                .kind<Render>()
                .each(systems::draw_health_bar);

        world.system("End drawing").kind<PostRender>().run(systems::end_drawing);
    }

    void RenderingModule::register_pipeline(flecs::world world) {
        world.component<PreRender>().add(flecs::Phase).depends_on(flecs::OnStore);
        world.component<Render>().add(flecs::Phase).depends_on<PreRender>();
        world.component<RenderGUI>().add(flecs::Phase).depends_on<Render>();
        world.component<PostRender>().add(flecs::Phase).depends_on<RenderGUI>();
    }

    void RenderingModule::register_submodules(flecs::world world) {
        world.import <gui::GUIModule>();
    }

    void RenderingModule::register_queries(flecs::world world) {
        auto base_query = world.query_builder<Renderable>();
        queries::query_all_renderables() = base_query.build();
        queries::query_visible_renderables() = base_query.with<Visible>().build();
    }
} // namespace rendering
