#include <flecs.h>
#include <raylib.h>

#define RAYGUI_IMPLEMENTATION
#include "ui.h"

#include "graphics.h"

int main() {
    flecs::world world;

    graphics::init(world);
    graphics::init_window(800, 600, "UI Prefab Buttons");

    int layout_id = 0;
    int layout_order = 0;

    auto button_prefab = world.prefab("UiButton")
        .set(ui::UiControl{ui::UI_BUTTON})
        .set(ui::UiLayoutRect{Rectangle{0.0f, 0.0f, 220.0f, 32.0f}})
        .set(ui::UiText{""})
        .set(ui::UiRect{});

    world.entity("Play Button")
        .is_a(button_prefab)
        .set(ui::UiLayoutRect{Rectangle{60.0f, 80.0f, 220.0f, 32.0f}})
        .set(ui::UiLayoutOwner{layout_id})
        .set(ui::UiDrawOrder{layout_id, layout_order++})
        .set(ui::UiText{"Play"});

    world.entity("Options Button")
        .is_a(button_prefab)
        .set(ui::UiLayoutRect{Rectangle{60.0f, 120.0f, 220.0f, 32.0f}})
        .set(ui::UiLayoutOwner{layout_id})
        .set(ui::UiDrawOrder{layout_id, layout_order++})
        .set(ui::UiText{"Options"});

    world.entity("Quit Button")
        .is_a(button_prefab)
        .set(ui::UiLayoutRect{Rectangle{60.0f, 160.0f, 220.0f, 32.0f}})
        .set(ui::UiLayoutOwner{layout_id})
        .set(ui::UiDrawOrder{layout_id, layout_order++})
        .set(ui::UiText{"Quit"});

    ui::register_layout_system(world, graphics::PreRender);
    ui::register_dispatch_system_ordered(world, graphics::PostRender);

    world.app().run();
    return 0;
}
