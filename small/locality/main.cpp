#include <flecs.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "components.h"

struct TagA {};
struct TagB {};
struct TagC {};

struct TableSpan {
    const Position* base = nullptr;
    int count = 0;
    std::uintptr_t start = 0;
    std::uintptr_t end = 0;
    bool contiguous = false;
};

template <typename T>
static bool is_contiguous(const T* data, int count) {
    if (!data || count <= 1) return true;
    for (int i = 1; i < count; ++i) {
        if (&data[i] != &data[i - 1] + 1) return false;
    }
    return true;
}

static int scaled_count(int base, int num, int den) {
    int count = (base * num) / den;
    if (count < 1) count = 1;
    return count;
}

static void seed_entities(flecs::world& ecs, int base_count) {
    int count_pos = scaled_count(base_count, 4, 4); // numerator / denominator = 1
    int count_pos_vel = scaled_count(base_count, 3, 4); // 0.75
    int count_pos_tag = scaled_count(base_count, 5, 4); // 1.25
    int count_pos_vel_tag = scaled_count(base_count, 2, 4); // 0.5
    int count_pos_vel_tags = scaled_count(base_count, 1, 3); // 0.333

    for (int i = 0; i < count_pos; ++i) {
        ecs.entity().set<Position>({(Real)i, 0.0f, 0.0f});
    }
    for (int i = 0; i < count_pos_vel; ++i) {
        ecs.entity()
            .set<Position>({(Real)i, 1.0f, 0.0f})
            .set<Velocity>({0.0f, (Real)i, 0.0f});
    }
    for (int i = 0; i < count_pos_tag; ++i) {
        ecs.entity()
            .set<Position>({(Real)i, 2.0f, 0.0f})
            .add<TagA>();
    }
    for (int i = 0; i < count_pos_vel_tag; ++i) {
        ecs.entity()
            .set<Position>({(Real)i, 3.0f, 0.0f})
            .set<Velocity>({0.0f, 0.0f, (Real)i})
            .add<TagB>();
    }
    for (int i = 0; i < count_pos_vel_tags; ++i) {
        ecs.entity()
            .set<Position>({(Real)i, 4.0f, 0.0f})
            .set<Velocity>({0.0f, 0.0f, (Real)i})
            .add<TagA>()
            .add<TagC>();
    }
}

static std::vector<TableSpan> dump_position_tables(flecs::world& ecs) {
    std::vector<TableSpan> spans;
    auto q = ecs.query_builder<const Position>().build();
    int table_index = 0;
    int total = 0;

    std::printf("== Position query ==\n");
    q.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            int count = it.count();
            total += count;
            const Position* base = count ? &pos[0] : nullptr;

            TableSpan span;
            span.base = base;
            span.count = count;
            span.start = (std::uintptr_t)base;
            span.end = (std::uintptr_t)(base ? base + count : base);
            span.contiguous = is_contiguous(base, count);
            spans.push_back(span);

            const Position* last = count ? &pos[count - 1] : base;
            size_t bytes = (size_t)count * sizeof(Position);
            std::printf("table %d: count=%d pos=%p..%p bytes=%zu contiguous=%s\n",
                table_index, count, (void*)base, (void*)last, bytes,
                span.contiguous ? "yes" : "no");
            table_index++;
        }
    });

    std::printf("tables matched: %d total entities: %d\n", table_index, total);
    return spans;
}

static void dump_position_velocity_tables(flecs::world& ecs) {
    auto q = ecs.query_builder<const Position, const Velocity>().build();
    int table_index = 0;

    std::printf("== Position + Velocity query ==\n");
    q.run([&](flecs::iter& it) {
        while (it.next()) {
            auto pos = it.field<const Position>(0);
            auto vel = it.field<const Velocity>(1);
            int count = it.count();
            const Position* pos_base = count ? &pos[0] : nullptr;
            const Velocity* vel_base = count ? &vel[0] : nullptr;

            bool pos_ok = is_contiguous(pos_base, count);
            bool vel_ok = is_contiguous(vel_base, count);

            const Position* pos_last = count ? &pos[count - 1] : pos_base;
            const Velocity* vel_last = count ? &vel[count - 1] : vel_base;
            std::printf("table %d: count=%d pos=%p..%p vel=%p..%p pos_contig=%s vel_contig=%s\n",
                table_index, count,
                (void*)pos_base, (void*)pos_last,
                (void*)vel_base, (void*)vel_last,
                pos_ok ? "yes" : "no",
                vel_ok ? "yes" : "no");
            table_index++;
        }
    });

    std::printf("tables matched: %d\n", table_index);
}

static void check_global_contiguity(std::vector<TableSpan> spans) {
    if (spans.empty()) {
        std::printf("global contiguous position buffer: no (no tables)\n");
        return;
    }

    std::sort(spans.begin(), spans.end(),
        [](const TableSpan& a, const TableSpan& b) { return a.start < b.start; });

    bool global_contiguous = true;
    for (size_t i = 1; i < spans.size(); ++i) {
        if (spans[i - 1].end != spans[i].start) {
            global_contiguous = false;
            break;
        }
    }

    std::printf("global contiguous position buffer: %s\n",
        global_contiguous ? "yes (adjacent spans)" : "no (gaps between tables)");
}

int main(int argc, char** argv) {
    int base_count = 20000;
    if (argc > 1) base_count = std::atoi(argv[1]);
    if (base_count < 1) base_count = 1;

    std::printf("Flecs locality probe\n");
    std::printf("base count: %d\n", base_count);

    flecs::world ecs;
    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<TagA>();
    ecs.component<TagB>();
    ecs.component<TagC>();

    seed_entities(ecs, base_count);

    auto spans = dump_position_tables(ecs);
    check_global_contiguity(spans);
    dump_position_velocity_tables(ecs);

    return 0;
}



// verdict: no single global contiguous array. contiguous within tables tho

// Flecs locality probe
// base count: 20000
// == Position query ==
// table 0: count=20000 pos=0000022CB3BF0CD0..0000022CB3C2B644 bytes=240000 contiguous=yes
// table 1: count=15000 pos=0000022CB3CE2020..0000022CB3D0DF34 bytes=180000 contiguous=yes
// table 2: count=25000 pos=0000022CB3E79030..0000022CB3EC2404 bytes=300000 contiguous=yes
// table 3: count=10000 pos=0000022CB3B70CB0..0000022CB3B8E164 bytes=120000 contiguous=yes
// table 4: count=6666 pos=0000022CB3C50CE0..0000022CB3C6454C bytes=79992 contiguous=yes
// tables matched: 5 total entities: 76666
// global contiguous position buffer: no (gaps between tables)
// == Position + Velocity query ==
// table 0: count=15000 pos=0000022CB3CE2020..0000022CB3D0DF34 vel=0000022CB3D12030..0000022CB3D3DF44 pos_contig=yes vel_contig=yes
// table 1: count=10000 pos=0000022CB3B70CB0..0000022CB3B8E164 vel=0000022CB3D84010..0000022CB3DA14C4 pos_contig=yes vel_contig=yes
// table 2: count=6666 pos=0000022CB3C50CE0..0000022CB3C6454C vel=0000022CB3B3E010..0000022CB3B5187C pos_contig=yes vel_contig=yes
// tables matched: 3