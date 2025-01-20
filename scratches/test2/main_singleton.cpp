#include <flecs.h>
#include <iostream>

// singleton components
struct PhysicsConfig {
    float gravity;
    float time_step;
    int iterations;
};

struct GameState {
    bool is_paused;
    float elapsed_time;
    int score;
};

struct RenderSettings {
    int width;
    int height;
    bool vsync;
};

// example regular component
struct Position {
    float x, y, z;
};

void print_game_state(const GameState* state) {
    std::cout << "Game State - "
              << "Paused: " << (state->is_paused ? "yes" : "no") << ", "
              << "Time: " << state->elapsed_time << ", "
              << "Score: " << state->score << std::endl;
}

// NOTE: term_at uses index `0` for the first item

int main() {
    flecs::world ecs;

    // register singletons
    ecs.set<PhysicsConfig>({-9.81f, 0.016f, 10});
    ecs.set<GameState>({false, 0.0f, 0});
    ecs.set<RenderSettings>({1920, 1080, true});

    // create a test entity with Position
    auto test_entity = ecs.entity()
        .set<Position>({1.0f, 2.0f, 3.0f});

    // example 1: using term_at for singleton
    std::cout << "\nExample 1: System with singleton using term_at" << std::endl;
    ecs.system<Position, const PhysicsConfig>()
        .term_at(1).singleton()
        .each([](Position& pos, const PhysicsConfig& physics) {
            std::cout << "Position with physics singleton (term_at):" << std::endl;
            std::cout << "Gravity: " << physics.gravity << std::endl;
        });

    // example 2: direct singleton access in system
    std::cout << "\nExample 2: Direct singleton access in system" << std::endl;
    ecs.system<Position>()
        .each([](flecs::entity e, Position& pos) {
            // read-only singleton access
            const auto* physics = e.world().get<PhysicsConfig>();
            const auto* game = e.world().get<GameState>();
            
            std::cout << "Position with direct singleton access:" << std::endl;
            std::cout << "Physics - gravity: " << physics->gravity << std::endl;
            std::cout << "Game - score: " << game->score << std::endl;
        });

    // example 3: mutable singleton access in system
    std::cout << "\nExample 3: Mutable singleton access in system" << std::endl;
    ecs.system<Position>()
        .each([](flecs::entity e, Position& pos) {
            // mutable singleton access
            auto* game = e.world().get_mut<GameState>();
            game->score += 10;
            
            // mix of const and mutable access
            const auto* physics = e.world().get<PhysicsConfig>();
            const auto* render = e.world().get<RenderSettings>();
            
            std::cout << "Mixed direct singleton access:" << std::endl;
            std::cout << "Updated score: " << game->score << std::endl;
            std::cout << "Using dt: " << physics->time_step << std::endl;
            std::cout << "Screen size: " << render->width << "x" << render->height << std::endl;
        });

    // example 4: combining term_at and direct access
    std::cout << "\nExample 4: Combining term_at and direct access" << std::endl;
    ecs.system<Position, const PhysicsConfig>()
        .term_at(1).singleton()
        .each([](flecs::entity e, Position& pos, const PhysicsConfig& physics1) {
            // direct access alongside term_at singleton
            const auto* physics2 = e.world().get<PhysicsConfig>();
            auto* game = e.world().get_mut<GameState>();
            
            std::cout << "Combined access methods:" << std::endl;
            std::cout << "Physics from term_at: " << physics1.gravity << std::endl;
            std::cout << "Physics from direct: " << physics2->gravity << std::endl;
            game->score += 5;
        });

    // run systems
    ecs.progress();

    return 0;
}