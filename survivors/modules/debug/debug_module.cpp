#include "debug_module.h"
#include "systems.h"

#include <modules/engine/core/components.h>
#include <modules/engine/physics/components.h>
#include <modules/engine/rendering/components.h>
#include <modules/engine/rendering/gui/components.h>
#include <modules/engine/rendering/gui/gui_module.h>
#include <modules/engine/rendering/pipelines.h>

#include <raylib.h>

namespace debug {
    void DebugModule::register_components(flecs::world &world) {}

    void DebugModule::register_systems(flecs::world &world) {

        debug_collidable_entity_id = world.system<const core::Position, const physics::Collider>(
                                                  "Draw collidable entity id")
                                             .kind<rendering::RenderGizmo>()
                                             .each(systems::draw_collidable);
        debug_collidable_entity_id.disable();

        debug_collider_bound =
                world.system<const core::Position, const physics::Collider>("Draw collider bound")
                        .kind<rendering::RenderGizmo>()
                        .each(systems::draw_collider_bound);
        debug_collider_bound.disable();

        debug_circle_collider = world.system<const core::Position, const physics::CircleCollider>(
                                             "Draw circle collider")
                                        .kind<rendering::RenderGizmo>()
                                        .with<rendering::Visible>()
                                        .group_by<rendering::Priority>()
                                        .each(systems::draw_circle_collider);
        debug_circle_collider.disable();

        debug_box_collider =
                world.system<const core::Position, const physics::Collider>("Draw box collider")
                        .kind<rendering::RenderGizmo>()
                        .with<rendering::Visible>()
                        .group_by<rendering::Priority>()
                        .each(systems::draw_box_collider);
        debug_box_collider.disable();

        debug_FPS = world.system("Draw FPS").kind<rendering::RenderGUI>().run(systems::draw_fps);
        debug_FPS.disable();

        debug_entity_count = world.system("Draw entity count")
                                     .kind<rendering::RenderGUI>()
                                     .run(systems::draw_entity_count);
        debug_entity_count.disable();

        debug_mouse_pos = world.system("Draw mouse position")
                                  .kind<rendering::RenderGUI>()
                                  .run(systems::draw_mouse_position);
        debug_mouse_pos.disable();

        debug_grid =
                world.system("Draw grid").kind<rendering::RenderGizmo>().run(systems::draw_grid);
        debug_grid.disable();

        debug_closest_enemy = world.system("Draw ray to the closest target")
                                      .kind<rendering::RenderGizmo>()
                                      .run(systems::draw_closest_enemy);
        debug_closest_enemy.disable();
    }

    void DebugModule::register_entities(flecs::world &world) {
        auto dropdown = world.entity("debug_dropdown")
                                .child_of(rendering::gui::GUIModule::menu_bar)
                                .set<rendering::gui::MenuBarTab>({"Debug Tools", 25});

        world.entity("debug_collisions_item_0")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>({"Toggle collidable entity id",
                                                      debug_collidable_entity_id,
                                                      rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_1_0")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle collider bound", debug_collider_bound, rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_1_1")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle circle collider", debug_circle_collider, rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_1_2")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle box collider", debug_box_collider, rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_2")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle FPS", debug_FPS, rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_3")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle Entity Count", debug_entity_count, rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_4")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle Mouse Pos", debug_mouse_pos, rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_5")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle Grid", debug_grid, rendering::gui::TOGGLE});
        world.entity("debug_collisions_item_6")
                .child_of(dropdown)
                .set<rendering::gui::MenuBarTabItem>(
                        {"Toggle View Closest Enemy", debug_closest_enemy, rendering::gui::TOGGLE});
    }
} // namespace debug
