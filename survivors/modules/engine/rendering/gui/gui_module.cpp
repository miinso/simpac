#include <modules/engine/core/components.h>
#include <modules/engine/rendering/components.h>
#include <modules/engine/rendering/pipelines.h>

#include "components.h"
#include "gui_module.h"
#include "systems.h"

namespace rendering {
    namespace gui {
        flecs::entity GUIModule::menu_bar;
        flecs::entity GUIModule::gui_canvas;

        void GUIModule::register_components(flecs::world &world) {
            world.component<Button>();
            world.component<Text>();
            world.component<Outline>();
        }

        void GUIModule::register_systems(flecs::world &world) {
            world.system("Load GUI theme").kind(flecs::OnStart).run(systems::load_style);

            world.system<core::GameSettings>("Init canvas size")
                    .kind(flecs::OnStart)
                    .each(systems::init_canvas_size);

            world.system<const Rectangle, Anchor>("Init anchor position")
                    .kind(flecs::OnStart)
                    .each(systems::init_anchor_position);

            world.observer<const Rectangle>("Observe parent rectangle change")
                    .term_at(0)
                    .parent()
                    .event(flecs::OnSet)
                    .each(systems::observe_parent_rectangle_change);

            world.system<core::GameSettings>("Resize window")
                    .kind<PreRender>()
                    .each(systems::resize_window);

            world.system<const Button, const Rectangle>("Draw Button")
                    .kind<RenderGUI>()
                    .each(systems::draw_button);

            world.system<const Panel, const Rectangle>("Draw Panel")
                    .kind<RenderGUI>()
                    .each(systems::draw_panel);

            world.system<const Text, const Rectangle>("Draw Text")
                    .kind<RenderGUI>()
                    .each(systems::draw_text);

            world.system<const Outline, const Rectangle>("Draw Outline")
                    .kind<RenderGUI>()
                    .each(systems::draw_outline);

            world.system<MenuBar>("Draw Menu Bar").kind<RenderGUI>().each(systems::draw_menu_bar);

            world.system<MenuBarTab, MenuBar>("Draw Tabs")
                    .term_at(1)
                    .parent()
                    .kind<RenderGUI>()
                    .each(systems::draw_tab);

            world.system<MenuBarTabItem, MenuBarTab, Rectangle>("Draw Tab Items")
                    .term_at(1)
                    .parent()
                    .term_at(2)
                    .parent()
                    .kind<RenderGUI>()
                    .each(systems::draw_tab_item);
        }

        void GUIModule::register_entities(flecs::world &world) {
            gui_canvas = world.entity("gui_canvas")
                                 .set<Rectangle>({0, 0, static_cast<float>(GetScreenWidth()),
                                                  static_cast<float>(GetScreenHeight())});

            menu_bar = world.entity("menu_bar")
                               .set<MenuBar>({200, 1, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)),
                                              GetColor(GuiGetStyle(BUTTON, BACKGROUND_COLOR))});
        }
    } // namespace gui
} // namespace rendering
