#include "game.h"

#if defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#endif

// engine references
#include "modules/engine/core/components.h"
#include "modules/engine/core/core_module.h"
#include "modules/engine/input/components.h"
#include "modules/engine/input/input_module.h"
#include "modules/engine/physics/components.h"
#include "modules/engine/physics/physics_module.h"
#include "modules/engine/rendering/components.h"
#include "modules/engine/rendering/rendering_module.h"

// user defined module
#include "modules/ai/ai_module.h"
#include "modules/ai/components.h"
#include "modules/debug/debug_module.h"
#include "modules/gameplay/components.h"
#include "modules/gameplay/gameplay_module.h"
#include "modules/player/player_module.h"

#include <raylib.h>

Game::Game(const char *windowName, int windowWidth, int windowHeight) :
    m_world(flecs::world()), m_windowName(windowName), m_windowWidth(windowWidth),
    m_windowHeight(windowHeight) {

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(m_windowWidth, m_windowHeight, m_windowName.c_str());
    // SetTargetFPS(60);
    // SetTargetFPS(GetMonitorRefreshRate(0));

#ifndef EMSCRIPTEN
    m_world.import <flecs::units>();
    m_world.import <flecs::stats>();
    m_world.set<flecs::Rest>({});
#endif
    m_world.import <core::CoreModule>();
    m_world.import <input::InputModule>();
    m_world.import <rendering::RenderingModule>();
    m_world.import <physics::PhysicsModule>();
    m_world.import <player::PlayerModule>();
    m_world.import <ai::AIModule>();
    m_world.import <gameplay::GameplayModule>();
    m_world.import <debug::DebugModule>();

    m_world.set<core::GameSettings>(
            {m_windowName, m_windowWidth, m_windowHeight, m_windowWidth, m_windowHeight});
    m_world.add<physics::CollisionRecordList>();

    // "{{entity}}" <{{component list}}>
    //
    // "player" <Position, Speed, Velocity, ...>
    //      "player_horizontal_input" <InputHorizontal>
    //          (anonymous) <KeyBinding>
    //          (anonymous) <KeyBinding>
    //          (anonymous) <KeyBinding>
    //          (anonymous) <KeyBinding>
    //      "player_vertical_input" <InputVertical>
    //          (anonymous) <KeyBinding>
    //          (anonymous) <KeyBinding>
    //          (anonymous) <KeyBinding>
    //          (anonymous) <KeyBinding>

    flecs::entity player =
            m_world.entity("player")
                    .set<core::Tag>({"player"})
                    .set<gameplay::Damage>({1})
                    .set<core::Position>(
                            {Eigen::Vector3f(GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f, 0)})
                    .set<core::Speed>({150})
                    .set<physics::Velocity>({Eigen::Vector3f::Zero()})
                    .set<physics::DesiredVelocity>({Eigen::Vector3f::Zero()})
                    .set<physics::AccelerationSpeed>({5.0})
                    .set<physics::Collider>(
                            {24, true, physics::CollisionFilter::player, physics::player_filter})
                    .set<rendering::Priority>({2}) // same as .add<Priority, int>
                    .set<rendering::Renderable>({LoadTexture("resources/player.png"), // 8x8
                                                 {0, 0},
                                                 3.0f,
                                                 WHITE})
                    .set<gameplay::Health>({150, 150})
                    .set<gameplay::RegenHealth>({2.5f});

    m_world.entity("dagger attack")
            .child_of(player)
            .add<gameplay::Projectile>()
            .set<gameplay::Attack>({"projectile", "enemy"})
            .set<gameplay::Cooldown>({1.0f, 1})
            .add<gameplay::CooldownCompleted>()
            // .set<gameplay::MultiProj>({3, 30.0f, 150.0f, 30.0f})
            .set<core::Speed>({150.0f});

    m_world.prefab("projectile")
            .add<gameplay::Projectile>()
            .set<gameplay::Attack>({"projectile", "enemy"})
            .set<gameplay::Chain>({6, std::unordered_set<int>()})
            .set<gameplay::Split>({std::unordered_set<int>()})
            .set<gameplay::Damage>({2})
            .set<physics::Velocity>({Eigen::Vector3f::Zero()})
            .set<physics::Collider>(
                    {24, false, physics::CollisionFilter::player, physics::player_filter})
            .set<rendering::Priority>({1})
            .set<rendering::Renderable>({LoadTexture("resources/dagger.png"), // 8x8
                                         {0, 0},
                                         3.0f,
                                         WHITE})
            .set<core::DestroyAfterTime>({5});

    auto hori = m_world.entity("player_horizontal_input") // create a new entity
                        .child_of(player) // that is a child of some other entity
                        .set<input::InputHorizontal>({}); // which holds this component
    m_world.entity().child_of(hori).set<input::KeyBinding>({KEY_A, -1});
    m_world.entity().child_of(hori).set<input::KeyBinding>({KEY_D, 1});
    m_world.entity().child_of(hori).set<input::KeyBinding>({KEY_LEFT, -1});
    m_world.entity().child_of(hori).set<input::KeyBinding>({KEY_RIGHT, 1});

    auto vert =
            m_world.entity("player_vertical_input").child_of(player).set<input::InputVertical>({});
    m_world.entity().child_of(vert).set<input::KeyBinding>({KEY_W, -1});
    m_world.entity().child_of(vert).set<input::KeyBinding>({KEY_S, 1});
    m_world.entity().child_of(vert).set<input::KeyBinding>({KEY_UP, -1});
    m_world.entity().child_of(vert).set<input::KeyBinding>({KEY_DOWN, 1});

    auto enemy_prefab =
            m_world.prefab("enemy")
                    .set<core::Tag>({"enemy"})
                    .set<core::Position>({Eigen::Vector3f(800, 400, 0)})
                    .set<core::Speed>({25})
                    .set<gameplay::Health>({10, 10})
                    .set<gameplay::Damage>({1})
                    .add<ai::Target>(player)
                    .add<ai::FollowTarget>()
                    .set<ai::StoppingDistance>({16.0})
                    .set<physics::Velocity>({Eigen::Vector3f::Zero()})
                    .set<physics::DesiredVelocity>({Eigen::Vector3f::Zero()})
                    .set<physics::AccelerationSpeed>({5.0})
                    .set<physics::Collider>(
                            {24, true, physics::CollisionFilter::enemy, physics::enemy_filter})
                    .set<rendering::Renderable>({LoadTexture("resources/ghost.png"), // 8x8
                                                 {0, 0},
                                                 3.0f,
                                                 WHITE})
                    .set<rendering::Priority>({0});

    m_world.entity("enemy_spawner").set<gameplay::Spawner>({enemy_prefab});
}

void Game::run() {
    m_world.progress(); // run once for `OnStart` phase

    while (!WindowShouldClose()) {
        m_world.progress(GetFrameTime());
    }

    CloseWindow();
}
