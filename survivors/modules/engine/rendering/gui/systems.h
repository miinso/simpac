#pragma once

#include "components.h"
#include "gui_module.h"

#include <modules/engine/core/components.h>

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

namespace rendering {
    namespace gui {
        namespace systems {
            inline void load_style(flecs::iter &it) {
                GuiLoadStyle("resources/styles/amber/style_amber.rgs");
            };

            inline void init_canvas_size(core::GameSettings &settings) {
                GUIModule::gui_canvas.set<Rectangle>({0, 0, static_cast<float>(GetScreenWidth()),
                                                      static_cast<float>(GetScreenHeight())});
                settings.windowWidth = GetScreenWidth();
                settings.windowHeight = GetScreenHeight();
            };

            inline void init_anchor_position(const Rectangle &rectangle, Anchor &anchor) {
                anchor.position.x = rectangle.x;
                anchor.position.y = rectangle.y;
            };

            inline void observe_parent_rectangle_change(flecs::entity e, const Rectangle &parent) {
                if (!e.has<Anchor>())
                    return;
                if (!e.has<Rectangle>())
                    return;

                auto anchor = e.try_get<Anchor>();

                Rectangle temp{*e.try_get<Rectangle>()};
                switch (anchor->horizontal_anchor) {
                    case HORIZONTAL_ANCHOR::CENTER:
                        temp.x = anchor->position.x + parent.x + parent.width / 2;
                        break;
                    case HORIZONTAL_ANCHOR::RIGHT:
                        temp.x = anchor->position.x + parent.x + parent.width;
                        break;
                    default:
                        temp.x = anchor->position.x + parent.x;
                        break;
                }
                switch (anchor->vertical_anchor) {
                    case VERTICAL_ANCHOR::MIDDLE:
                        temp.y = anchor->position.y + parent.y + parent.height / 2;
                        break;
                    case VERTICAL_ANCHOR::BOTTOM:
                        temp.y = anchor->position.y + parent.y + parent.height;
                        break;
                    default:
                        temp.y = anchor->position.y + parent.y;
                        break;
                }
                e.set<Rectangle>({temp});
            };

            inline void resize_window(core::GameSettings &settings) {
                if (IsWindowResized()) {
                    init_canvas_size(settings);
                }
#ifndef EMSCRIPTEN
                SetMouseScale(settings.windowWidth / (float) settings.initialWidth,
                              settings.windowHeight / (float) settings.initialHeight);
#endif
            };

            inline void draw_button(const Button &button, const Rectangle &rect) {
                GuiSetStyle(BUTTON, TEXT_WRAP_MODE, TEXT_WRAP_WORD);
                if (GuiButton(rect, button.text.c_str())) {
                    button.on_click_system.run();
                }
                GuiSetStyle(BUTTON, TEXT_WRAP_MODE, DEFAULT);
            };

            inline void draw_panel(const Panel &panel, const Rectangle &rect) {
                GuiPanel(rect, panel.name.c_str());
            };

            inline void draw_text(const Text &text, const Rectangle &rect) {
                GuiDrawText(text.text.c_str(), rect, text.alignment, GUIModule::font_color());
            };

            inline void draw_outline(const Outline &outline, const Rectangle &rect) {
                GuiDrawRectangle(rect, outline.border_size, outline.border_color,
                                 outline.fill_color);
            };

            inline void draw_menu_bar(MenuBar &bar) {
                GuiDrawRectangle({0, 0, (float) GetScreenWidth(), (float) 25}, bar.border_size,
                                 GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)),
                                 GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
            };

            inline void draw_tab(flecs::iter &it, size_t i, MenuBarTab &tab, MenuBar &bar) {
                Rectangle rec = {GetScreenWidth() - (float) (i + 1) * bar.item_width, 0,
                                 (float) bar.item_width, (float) 25};

                if (!it.entity(i).has<Rectangle>()) {
                    it.entity(i).set<Rectangle>({rec});
                }

                if (GuiButton(rec, tab.name.c_str())) {
                    tab.active = !tab.active;
                    it.entity(i).set<Rectangle>(rec);
                }
            };

            inline void draw_tab_item(flecs::iter &it, size_t i, MenuBarTabItem &item,
                                      MenuBarTab &tab, Rectangle &rec) {
                if (!tab.active)
                    return;
                if (GuiButton({rec.x, rec.y + (i + 1) * rec.height, rec.width, rec.height},
                              item.name.c_str())) {

                    if (item.type == TOGGLE) {
                        if (item.toggle_system_entity.enabled())
                            item.toggle_system_entity.disable();
                        else
                            item.toggle_system_entity.enable();
                    } else if (item.type == RUN) {
                        item.toggle_system_entity.run();
                    }
                }
            };
        } // namespace systems
    } // namespace gui
} // namespace rendering
