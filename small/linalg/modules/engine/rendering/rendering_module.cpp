#include "rendering_module.h"
#include "components.h"
#include "pipelines.h"
#include "queries.h"
#include "systems.h"

#include "modules/engine/core/components.h"

#include <raylib.h>

using namespace core;

namespace rendering {
    void RenderingModule::register_components(flecs::world &world) { world.component<Priority>(); }

    void RenderingModule::register_queries(flecs::world &world) {
        queries::all_renderables = world.query_builder<Renderable>().build();
        queries::visible_renderables = world.query_builder<Renderable>().with<Visible>().build();
    }

    void RenderingModule::register_systems(flecs::world &world) {
        world.system<TrackingCamera>("Init camera 2d")
                .kind(flecs::OnStart)
                .each(systems::create_camera);

        world.system("Begin draw").kind<PreRender>().run(systems::begin_drawing);

        world.system<TrackingCamera, core::Settings>("Begin camera 2d mode")
                .kind<PreRender>()
                .each(systems::begin_camera_mode);

        world.system("End camera 2d mode").kind<RenderGUI>().run(systems::end_camera_mode);

        world.system("End drawing").kind<PostRender>().run(systems::end_drawing);
    }

    void RenderingModule::register_pipeline(flecs::world &world) {
        world.component<PreRender>().add(flecs::Phase).depends_on(flecs::OnStore);
        world.component<RenderBackground>().add(flecs::Phase).depends_on<PreRender>();
        world.component<Render>().add(flecs::Phase).depends_on<RenderBackground>();
        world.component<RenderGizmo>().add(flecs::Phase).depends_on<Render>();
        world.component<RenderGUI>().add(flecs::Phase).depends_on<RenderGizmo>();
        world.component<PostRender>().add(flecs::Phase).depends_on<RenderGUI>();
    }

    void RenderingModule::register_submodules(flecs::world &world) {
        // world.import <gui::GUIModule>();
    }


} // namespace rendering
