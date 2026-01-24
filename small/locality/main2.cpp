#include <flecs.h>

#include <cstdio>

#include "components.h"

struct TagA {};

static void seed_entities(flecs::world& ecs) {
    for (int i = 0; i < 3; ++i) {
        ecs.entity().set<Position>({(Real)i, 0.0f, 0.0f});
    }
    for (int i = 0; i < 4; ++i) {
        ecs.entity()
            .set<Position>({(Real)i, 1.0f, 0.0f})
            .set<Velocity>({0.0f, (Real)i, 0.0f});
    }
    for (int i = 0; i < 5; ++i) {
        ecs.entity()
            .set<Position>({(Real)i, 2.0f, 0.0f})
            .set<Velocity>({(Real)i, 0.0f, 0.0f})
            .add<TagA>();
    }
}

static void dump_position_query(flecs::world& ecs) {
    auto q = ecs.query_builder<const Position>().build();

    int single_count = 0;
    q.run([&](flecs::iter& it) {
        if (it.next()) {
            auto pos = it.field<const Position>(0);
            int count = it.count();
            const Position* base = count ? &pos[0] : nullptr;
            std::printf("Position first table: count=%d base=%p\n", count, (void*)base);
            single_count = count;
        }
    });

    int total_count = 0;
    int table_index = 0;
    q.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            int count = it.count();
            const Position* base = count ? &pos[0] : nullptr;
            std::printf("Position table %d: count=%d base=%p\n",
                table_index, count, (void*)base);
            total_count += count;
            table_index++;
        }
    });

    std::printf("Position count (one it.next): %d\n", single_count);
    std::printf("Position count (while it.next): %d\n", total_count);
}

static void dump_position_velocity_query(flecs::world& ecs) {
    auto q = ecs.query_builder<const Position, const Velocity>().build();
    int table_index = 0;
    int total_count = 0;

    q.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto vel = it.field<const Velocity>(1);
            int count = it.count();
            const Position* pos_base = count ? &pos[0] : nullptr;
            const Velocity* vel_base = count ? &vel[0] : nullptr;
            std::printf("Pos+Vel table %d: count=%d pos=%p vel=%p\n",
                table_index, count, (void*)pos_base, (void*)vel_base);
            total_count += count;
            table_index++;
        }
    });

    std::printf("Pos+Vel tables: %d total=%d\n", table_index, total_count);
}

int main() {
    flecs::world ecs;
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<TagA>();

    seed_entities(ecs);

    std::printf("Expected totals: Position=12, Position+Velocity=9\n");
    dump_position_query(ecs);
    dump_position_velocity_query(ecs);
    return 0;
}

// Expected totals: Position=12, Position+Velocity=9
// Position first table: count=3 base=0000017514343D30
// Position table 0: count=3 base=0000017514343D30
// Position table 1: count=4 base=00000175140E1900
// Position table 2: count=5 base=00000175140E19C0
// Position count (one it.next): 3
// Position count (while it.next): 12
// Pos+Vel table 0: count=4 pos=00000175140E1900 vel=00000175140E1960
// Pos+Vel table 1: count=5 pos=00000175140E19C0 vel=00000175140E1A20
// Pos+Vel tables: 2 total=9