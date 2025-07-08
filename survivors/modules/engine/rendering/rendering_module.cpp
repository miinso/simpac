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

    flecs::query<Renderable> queries::query_all_renderables;
    flecs::query<Renderable> queries::query_visible_renderables;
    void RenderingModule::register_queries(flecs::world world) {
        auto base_query = world.query_builder<Renderable>();
        queries::query_all_renderables = base_query.build();
        queries::query_visible_renderables = base_query.with<Visible>().build();
    }

    void RenderingModule::register_systems(flecs::world world) {
        world.system<TrackingCamera>("Init camera 2d")
                .term_at(0)
                .singleton()
                .kind(flecs::OnStart)
                .each(systems::create_camera);

        world.system("Begin draw").kind<PreRender>().run(systems::begin_drawing);

        world.system<TrackingCamera, core::GameSettings>("Begin camera 2d mode")
                .term_at(0)
                .singleton()
                .term_at(1)
                .singleton()
                .kind<PreRender>()
                .each(systems::begin_camera_mode);

        world.system<const core::Position, const Renderable, const core::GameSettings,
                     const TrackingCamera>("Determine Visible Entities")
                .term_at(2)
                .singleton()
                .term_at(3)
                .singleton()
                .write<Visible>()
                .kind<PreRender>()
                .each(systems::determine_visible_entity);

        world.system<const Renderable>("Draw background textures")
                .kind<RenderBackground>()
                .without<core::Position>()
                .with<Priority>()
                .order_by<Priority>([](flecs::entity_t e1, const Priority *p1, flecs::entity_t e2,
                                       const Priority *p2) {
                    int order = p1->priority - p2->priority;
                    if (order == 0)
                        return static_cast<int>(e1 - e2);
                    return p1->priority - p2->priority;
                })
                .each(systems::draw_background_texture);

        world.system<const Renderable, const core::Position, const Rotation *>(
                     "Draw entitys with texture")
                .kind<Render>()
                .with<Visible>()
                .group_by<Priority>()
                .each(systems::draw_entity_with_texture);

        world.system<ProgressBar, const core::Position, const Renderable>("Draw health bar")
                .kind<Render>()
                .each(systems::draw_health_bar);

        world.system("End camera 2d mode").kind<RenderGUI>().run(systems::end_camera_mode);

        world.system("End drawing").kind<PostRender>().run(systems::end_drawing);
    }

    void RenderingModule::register_pipeline(flecs::world world) {
        world.component<PreRender>().add(flecs::Phase).depends_on(flecs::OnStore);
        world.component<RenderBackground>().add(flecs::Phase).depends_on<PreRender>();
        world.component<Render>().add(flecs::Phase).depends_on<RenderBackground>();
        world.component<RenderGizmo>().add(flecs::Phase).depends_on<Render>();
        world.component<RenderGUI>().add(flecs::Phase).depends_on<RenderGizmo>();
        world.component<PostRender>().add(flecs::Phase).depends_on<RenderGUI>();
    }

    void RenderingModule::register_submodules(flecs::world world) {
        world.import <gui::GUIModule>();
    }


} // namespace rendering
