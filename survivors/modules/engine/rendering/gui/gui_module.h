#pragma once

#include <modules/base_module.h>

#include "components.h"

#include <raylib.h>


namespace rendering {
    namespace gui {
        struct GUIModule : public BaseModule<GUIModule> {
        public:
            GUIModule(flecs::world world) : BaseModule(world){};
            static flecs::entity menu_bar;
            static flecs::entity gui_canvas;

            static Color font_color() { return GetColor(GuiGetStyle(TEXTBOX, TEXT_COLOR_NORMAL)); }

        private:
            void register_components(flecs::world &world);
            void register_systems(flecs::world &world);
            void register_entities(flecs::world &world);

            friend struct BaseModule<GUIModule>;
        };
    } // namespace gui
} // namespace rendering
