#include <flecs.h>

#include <cstdio>

#include "components.h"

struct Particle {};
struct Camera {};
struct Red {};
struct Blue {};

static void seed_scene_a(flecs::world& ecs) {
    for (int i = 0; i < 10; ++i) {
        ecs.entity()
            .add<Particle>()
            .set<Position>({(Real)i, 0.0f, 0.0f})
            .set<Velocity>({0.0f, (Real)i, 0.0f});
    }

    ecs.entity()
        .set<Position>({0.0f, 10.0f, 0.0f})
        .add<Camera>();
}

static void seed_scene_b(flecs::world& ecs) {
    for (int i = 0; i < 3; ++i) {
        ecs.entity()
            .add<Particle>()
            .add<Red>()
            .set<Position>({(Real)i, 1.0f, 0.0f})
            .set<Velocity>({(Real)i, 0.0f, 0.0f});
    }
    for (int i = 0; i < 7; ++i) {
        ecs.entity()
            .add<Particle>()
            .add<Blue>()
            .set<Position>({(Real)i, 2.0f, 0.0f})
            .set<Velocity>({0.0f, (Real)i, 0.0f});
    }

    ecs.entity()
        .set<Position>({0.0f, 10.0f, 0.0f})
        .add<Camera>();
}

static void dump_position_only(flecs::world& ecs, const char* label, bool with_particle) {
    auto qb = ecs.query_builder<const Position>();
    if (with_particle) {
        qb.with<Particle>();
    }
    auto q = qb.build();
    int table_index = 0;
    int total = 0;

    std::printf("== %s ==\n", label);
    q.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            int count = it.count();
            const Position* base = count ? &pos[0] : nullptr;
            std::printf("table %d: count=%d base=%p\n", table_index, count, (void*)base);
            total += count;
            table_index++;
        }
    });
    std::printf("tables=%d total=%d\n", table_index, total);
}

static void dump_position_velocity(
    flecs::world& ecs, const char* label, bool with_particle) {
    auto qb = ecs.query_builder<const Position, const Velocity>();
    if (with_particle) {
        qb.with<Particle>();
    }
    auto q = qb.build();
    int table_index = 0;
    int total = 0;

    std::printf("== %s ==\n", label);
    q.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto vel = it.field<const Velocity>(1);
            int count = it.count();
            const Position* pos_base = count ? &pos[0] : nullptr;
            const Velocity* vel_base = count ? &vel[0] : nullptr;
            std::printf("table %d: count=%d pos=%p vel=%p\n",
                table_index, count, (void*)pos_base, (void*)vel_base);
            total += count;
            table_index++;
        }
    });
    std::printf("tables=%d total=%d\n", table_index, total);
}

static void run_scene_a() {
    flecs::world ecs;
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<Particle>();
    ecs.component<Camera>();
    ecs.component<Red>();
    ecs.component<Blue>();

    std::printf("Scene A: 10 Particle(pos,vel) + 1 Camera(pos)\n");
    seed_scene_a(ecs);

    dump_position_only(ecs, "Position-only (no filter)", false);
    dump_position_only(ecs, "Position-only + Particle", true);
    dump_position_velocity(ecs, "Position+Velocity + Particle", true);
}

static void run_scene_b() {
    flecs::world ecs;
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<Particle>();
    ecs.component<Red>();
    ecs.component<Blue>();

    std::printf("Scene B: 3 Red + 7 Blue (all Particle pos,vel)\n");
    seed_scene_b(ecs);

    dump_position_velocity(ecs, "Position+Velocity + Particle", true);
}

int main() {
    run_scene_a();
    std::printf("\n");
    run_scene_b();
    return 0;
}


// Scene A: 10 Particle(pos,vel) + 1 Camera(pos)
// == Position-only (no filter) ==
// table 0: count=10 base=000001E9D1CA31E0
// table 1: count=1 base=000001E9D1DA44E0
// tables=2 total=11
// == Position-only + Particle ==
// table 0: count=10 base=000001E9D1CA31E0
// tables=1 total=10
// == Position+Velocity + Particle ==
// table 0: count=10 pos=000001E9D1CA31E0 vel=000001E9D1CA32A0
// tables=1 total=10

// Scene B: 3 Red + 7 Blue (all Particle pos,vel) + 1 Camera(pos)
// == Position+Velocity + Particle ==
// table 0: count=3 pos=000001E9D1C80A20 vel=000001E9D1C80A50
// table 1: count=7 pos=000001E9D1D4FD00 vel=000001E9D1D4FD60
// tables=2 total=10