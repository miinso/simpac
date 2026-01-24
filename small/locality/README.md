flecs memory contiguity: why it feels confusing + what is true

the confusing claim
- "component values are stored in contiguous arrays"
- sounds like one global array per component across the whole world
- but in archetype ecs, "contiguous" is per table, not global

what is actually true
- each table (archetype) stores each component column as a contiguous array
- tables are separate allocations; they are not guaranteed adjacent
- a query can match many tables, so you can get many contiguous blocks
- global contiguity only happens when all matched entities share one table

why this matters for gpu upload
- base pointer is valid per table only
- to render all entities, iterate tables with while(it.next())
- if you want one upload, pack table blocks into a staging buffer
- if you want one table, enforce one archetype (same component set)

TODO: refactor gpu renderes
snippets (table-wise bulk copy)
```cpp
auto q = ecs.query_builder<const Position, const Velocity>()
    .with<Particle>()
    .build();

size_t offset = 0; // or get info directly from `flecs::iter`
q.run([&](flecs::iter& it) {
    while (it.next()) {
        auto pos = it.field<const Position>(0);
        auto vel = it.field<const Velocity>(1);
        int n = it.count();
        std::memcpy(pos_out + offset * 3, pos[0].data(), (size_t)n * 3 * sizeof(Real));
        std::memcpy(vel_out + offset * 3, vel[0].data(), (size_t)n * 3 * sizeof(Real));
        offset += (size_t)n;
    }
});
```

snippets (interleaved output needs a per-entity loop)
```cpp
struct ParticleGpu { float px, py, pz, vx, vy, vz; };
std::vector<ParticleGpu> staging(total_count);

size_t offset = 0;
q.run([&](flecs::iter& it) {
    while (it.next()) {
        auto pos = it.field<const Position>(0);
        auto vel = it.field<const Velocity>(1);
        int n = it.count();
        for (int i = 0; i < n; ++i) {
            auto& dst = staging[offset + (size_t)i];
            dst.px = pos[i].x; dst.py = pos[i].y; dst.pz = pos[i].z;
            dst.vx = vel[i].x; dst.vy = vel[i].y; dst.vz = vel[i].z;
        }
        offset += (size_t)n;
    }
});
```

examples from main3
- scene a: <Position>-only query -> 2 tables (particles + camera)
- scene a: <Position, Velocity, Particle> -> 1 table (particles only)
- scene b: <Position, Velocity, Particle> -> 2 tables (red + blue)
- scene b: <Position, Velocity, Particle, Blue> -> 1 table, so a single contiguous block for blue; base ptr once to gpu is enough
