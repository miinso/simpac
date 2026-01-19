#include "graphics.h"

#include <gtest/gtest.h>

namespace {
void disable_graphics_systems(flecs::world& world) {
    const char* names[] = {
        "graphics::update_camera",
        "graphics::sync_camera",
        "graphics::begin",
        "graphics::render3d",
        "graphics::render2d",
        "graphics::end",
    };
    for (const char* name : names) {
        auto system_entity = world.lookup(name);
        if (system_entity.is_alive()) {
            system_entity.disable();
        }
    }
}
} // namespace

TEST(GraphicsLifecycle, InitCreatesPhases) {
    flecs::world world;
    graphics::init(world);

    EXPECT_TRUE(world.lookup("PreRender").is_alive());
    EXPECT_TRUE(world.lookup("OnRender").is_alive());
    EXPECT_TRUE(world.lookup("PostRender").is_alive());
    EXPECT_TRUE(world.lookup("OnPresent").is_alive());

    graphics::close_window();
}

TEST(GraphicsLifecycle, RunFrameTicksSystems) {
    flecs::world world;
    graphics::init(world);
    disable_graphics_systems(world);

    struct Tick {
        int value = 0;
    };

    auto entity = world.entity().set<Tick>({});
    world.system<Tick>("TickSystem")
        .kind(flecs::OnUpdate)
        .each([](Tick& tick) {
            ++tick.value;
        });

    graphics::detail::run_frame(0.016f);
    graphics::detail::run_frame(0.016f);

    const Tick* tick = entity.try_get<Tick>();
    ASSERT_TRUE(tick != nullptr);
    EXPECT_EQ(tick->value, 2);

    graphics::close_window();
}

TEST(GraphicsLifecycle, CloseKeepsWorldAlive) {
    flecs::world world;
    graphics::init(world);

    auto before = world.entity("before_close");
    EXPECT_TRUE(before.is_alive());

    graphics::close_window();

    auto after = world.entity("after_close");
    EXPECT_TRUE(after.is_alive());
}
