#pragma once

#include <flecs.h>
#include <raygui.h>
#include <string>

namespace rendering {
    namespace gui {
        struct Button {
            std::string text;
            flecs::system on_click_system;
        };

        struct Text {
            std::string text;
            int alignment;
        };

        struct Outline {
            int border_size;
            Color border_color;
            Color fill_color;
        };

        struct Panel {
            std::string name;
        };

        struct MenuBar {
            int item_width;
            int border_size;
            Color border_color;
            Color fill_color;
        };

        struct MenuBarTab {
            std::string name;
            int item_spacing;
            bool active = false;
        };

        enum MenuBarTabItemType { TOGGLE, RUN };
        struct MenuBarTabItem {
            std::string name;
            flecs::system toggle_system_entity;
            MenuBarTabItemType type;
        };

        enum HORIZONTAL_ANCHOR { LEFT, CENTER, RIGHT };
        enum VERTICAL_ANCHOR { TOP, MIDDLE, BOTTOM };

        struct Anchor {
            Vector2 position;
            HORIZONTAL_ANCHOR horizontal_anchor;
            VERTICAL_ANCHOR vertical_anchor;

            Anchor() {
                position = {};
                this->horizontal_anchor = HORIZONTAL_ANCHOR::LEFT;
                this->vertical_anchor = VERTICAL_ANCHOR::TOP;
            }

            Anchor(HORIZONTAL_ANCHOR h, VERTICAL_ANCHOR v) {
                position = {};
                this->horizontal_anchor = h;
                this->vertical_anchor = v;
            }
        };
    } // namespace gui
} // namespace rendering
