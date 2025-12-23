# Boids Simulator

Flecs-based boid flocking simulation with two architectural variants for benchmarking.

## Build Targets

```bash
bazel build //scratches/homeworks:boids -c opt --copt=/Zi --linkopt=/DEBUG --linkopt=/PROFILE          # Parent System version
bazel build //scratches/homeworks:boids_original -c opt --copt=/Zi --linkopt=/DEBUG --linkopt=/PROFILE # Pipeline version
```

## Architecture Comparison

### `boids.cpp` - Parent System Pattern

One parent system orchestrates all child systems with explicit `.run(dt)` calls:

```cpp
auto sim_tick = ecs.timer().interval(1.0f / 60.0f);

ecs.system("BoidSimulation")
    .kind(flecs::PreUpdate)
    .tick_source(sim_tick)
    .run([&](flecs::iter& it) {
        float dt = it.delta_time();

        handle_keypress.run(dt);
        if (debug.use_spatial_hash) {
            update_hash.run(dt);
            find_neighbors_spatial.run(dt);
        } else {
            find_neighbors_brute.run(dt);
        }
        reset_acceleration.run(dt);
        separation.run(dt);
        alignment.run(dt);
        cohesion.run(dt);
        // ...
    });
```

**Pros:**
- Algorithm visible in one place
- Easy conditional logic (if/else for spatial hash)
- Explicit control flow

### `boids_original.cpp` - Pipeline Pattern

Each system registered directly on Flecs phases:

```cpp
auto sim_tick = ecs.timer().interval(1.0f / 60.0f);

ecs.system("Separation")
    .kind(flecs::PreUpdate)
    .tick_source(sim_tick)
    .with<Boid>()
    .each([](flecs::entity e, ...) { ... });

ecs.system("Alignment")
    .kind(flecs::PreUpdate)
    .tick_source(sim_tick)
    .with<Boid>()
    .each([](flecs::entity e, ...) { ... });
```

**Pros:**
- Idiomatic Flecs usage
- Systems can be enabled/disabled individually
- Potential for parallel execution by Flecs scheduler

## Timestep Configuration

Both versions use fixed 60Hz simulation with variable display rate:

| Component | Rate | Source |
|-----------|------|--------|
| Simulation | Fixed 60 Hz | `ecs.timer().interval(1.0f / 60.0f)` |
| Rendering | Variable | `GetFrameTime()` via `progress()` |

## Controls

| Key | Action |
|-----|--------|
| `+` | Add 500 boids |
| `-` | Remove 500 boids |
| `H` | Toggle spatial hash |
| `V` | Toggle hash visualization |
| `R` | Remove all boids |

## Profiling

For VS2022 profiling, executables are at:
```
bazel-bin/scratches/homeworks/boids.exe
bazel-bin/scratches/homeworks/boids_original.exe
```

## Benchmark Results

VS2022 CPU profiler, 10 second sessions, release build (`-c opt`):

| Metric | Parent System | Pipeline |
|--------|---------------|----------|
| **Total CPU samples** | 12,766 | 13,058 |
| `find_neighbors_spatial` | 4,799 (37.59%) | 4,724 (36.18%) |
| `SpatialHash::query` | 844 (6.61%) | 860 (6.59%) |

**Result: Parent system is ~2.2% faster**

The parent system pattern introduces **no overhead**. It's slightly more efficient due to:
- Single system dispatch vs multiple dispatches
- Less Flecs scheduler bookkeeping
- `.run(dt)` calls are direct function invocations

The hot path (`find_neighbors_spatial`) dominates both versions equally. Architectural choice has negligible performance impact - choose based on code organization preference.
