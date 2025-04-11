#include "flecs.h"

// #include "components.hpp"
#include "graphics.h"
// #include "pbd1.hpp"
#include <iostream>
#include <vector>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float delta_time = 0.01f;

// Vector3f gravity(0, -9.81f, 0);
// Vector3f gravity(0, 0, 0);

int main() {
    flecs::world ecs;
    ecs.set_threads(4); // it just works??

    graphics::init(ecs);
    graphics::init_window(800, 800, "muller2007position");

    // ecs.set<Scene>({delta_time, 1, 1, gravity});

    // std::vector<flecs::entity> particles;
    // int num_particles = 100;
    // float position_range = 10.0f;

    // for (int i = 0; i < num_particles; ++i) {
    //     Vector3f position((randf(200) / 100.0f - 1.0f) * 1.0f,
    //                       (randf(200) / 100.0f - 1.0f) * 1.0f + 2,
    //                       (randf(200) / 100.0f - 1.0f) * 1.0f);

    //     Vector3f velocity((randf(200) / 100.0f - 1.0f) * 1.0f,
    //                       (randf(200) / 100.0f - 1.0f) * 1.0f,
    //                       (randf(200) / 100.0f - 1.0f) * 1.0f);
    //     auto p = add_particle(ecs, position, 1.0f);
    //     p.get_mut<Velocity>()->value = velocity * 10;
    //     particles.push_back(p);
    // }

    // float stiffness = 1e+3;
    // for (size_t i = 0; i < particles.size(); ++i) {
    //     for (size_t j = i + 1; j < particles.size(); ++j) {
    //         add_distance_constraint(ecs, particles[i], particles[j], stiffness);
    //     }
    // }

    ////////// system registration
    ecs.system("progress").kind(flecs::OnUpdate).multi_threaded().run([](flecs::iter& it) {
        // auto params = it.world().get_mut<Scene>();
        // params->elapsed += params->timestep;
        printf("hi\n");
    });

    graphics::run_main_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}