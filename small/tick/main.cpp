#include <cstdio>

#include <flecs.h>

namespace {

void run_d() {
    const double frame_dt = 1.0 / 60.0;
    const double tick_dt = 1.0 / 1000.0;
    const int frame_count = 60;

    flecs::world ecs;
    auto sim_tick = ecs.timer("engine::SimTick").interval(tick_dt);

    int pre_count = 0;
    int on_count = 0;
    double last_tick_dt = 0.0;

    ecs.system("TickedPre")
        .kind(flecs::PreUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter& it) {
            pre_count++;
            last_tick_dt = it.delta_system_time();
        });

    ecs.system("TickedOnUpdate")
        .kind(flecs::OnUpdate)
        .tick_source(sim_tick)
        .run([&](flecs::iter&) {
            on_count++;
        });

    sim_tick.start();
    for (int i = 0; i < frame_count; ++i) {
        ecs.progress(frame_dt);
    }

    double total_time = frame_count * frame_dt;
    int expected = (int)((total_time / tick_dt) + 1e-6);
    std::printf("== D: tick_source only (60fps -> 1000Hz) ==\n");
    std::printf("frames=%d frame_dt=%.4f sim_dt=%.4f expected=%d pre=%d on=%d last_tick_dt=%.4f\n",
        frame_count, frame_dt, tick_dt, expected, pre_count, on_count, last_tick_dt);
}

void run_d1() {
    const double frame_dt = 1.0 / 60.0;
    const double step_dt = 1.0 / 1000.0;
    const int frame_count = 60;

    flecs::world ecs;
    int sim_steps = 0;

    ecs.system("SimStep")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter&) {
            sim_steps++;
        });

    double acc = 0.0;
    for (int i = 0; i < frame_count; ++i) {
        acc += frame_dt;
        while (acc + 1e-12 >= step_dt) {
            ecs.progress(step_dt);
            acc -= step_dt;
        }
    }

    double total_time = frame_count * frame_dt;
    int expected = (int)((total_time / step_dt) + 1e-6);
    std::printf("== D1: accumulator + progress(step_dt) ==\n");
    std::printf("frames=%d frame_dt=%.4f sim_dt=%.4f expected=%d sim=%d leftover=%.6f\n",
        frame_count, frame_dt, step_dt, expected, sim_steps, acc);
}

void run_d2() {
    const double frame_dt = 1.0 / 60.0;
    const double step_dt = 1.0 / 1000.0;
    const int frame_count = 60;

    flecs::world ecs;
    int sim_steps = 0;
    int render_steps = 0;

    auto sim_step = ecs.system("SimStep")
        .kind(0)
        .run([&](flecs::iter&) {
            sim_steps++;
        });

    ecs.system("Render")
        .kind(flecs::OnUpdate)
        .run([&](flecs::iter&) {
            render_steps++;
        });

    double acc = 0.0;
    for (int i = 0; i < frame_count; ++i) {
        acc += frame_dt;
        while (acc + 1e-12 >= step_dt) {
            sim_step.run(step_dt);
            acc -= step_dt;
        }
        ecs.progress(frame_dt);
    }

    double total_time = frame_count * frame_dt;
    int expected = (int)((total_time / step_dt) + 1e-6);
    std::printf("== D2: manual sim + render progress ==\n");
    std::printf("frames=%d frame_dt=%.4f sim_dt=%.4f expected=%d sim=%d render=%d leftover=%.6f\n",
        frame_count, frame_dt, step_dt, expected, sim_steps, render_steps, acc);
}

void run_d3() {
    const double frame_dt = 1.0 / 60.0;
    const double step_dt = 1.0 / 1000.0;
    const int frame_count = 60;

    flecs::world ecs;
    int sim_steps = 0;
    double last_tick_dt = 0.0;

    ecs.system("SimStep")
        .kind(flecs::PreUpdate)
        .interval(step_dt)
        .run([&](flecs::iter& it) {
            sim_steps++;
            last_tick_dt = it.delta_system_time();
        });

    for (int i = 0; i < frame_count; ++i) {
        ecs.progress(frame_dt);
    }

    double total_time = frame_count * frame_dt;
    int expected = (int)((total_time / step_dt) + 1e-6);
    std::printf("== D3: system.interval(0.001) ==\n");
    std::printf("frames=%d frame_dt=%.4f interval=%.4f expected=%d sim=%d last_tick_dt=%.4f\n",
        frame_count, frame_dt, step_dt, expected, sim_steps, last_tick_dt);
}

void run_d4() {
    const double frame_dt = 1.0 / 60.0;
    const double step_dt = 1.0 / 1000.0;
    const int frame_count = 60;

    flecs::world ecs;
    int sim_steps = 0;
    double last_tick_dt = 0.0;

    auto sim_tick = ecs.timer("engine::SimTick").interval(step_dt);
    sim_tick.start();

    ecs.system("SimStep")
        .kind(flecs::PreUpdate)
        .rate(sim_tick, 1)
        .run([&](flecs::iter& it) {
            sim_steps++;
            last_tick_dt = it.delta_system_time();
        });

    for (int i = 0; i < frame_count; ++i) {
        ecs.progress(frame_dt);
    }

    double total_time = frame_count * frame_dt;
    int expected = (int)((total_time / step_dt) + 1e-6);
    std::printf("== D4: rate(sim_tick, 1) ==\n");
    std::printf("frames=%d frame_dt=%.4f sim_dt=%.4f expected=%d sim=%d last_tick_dt=%.4f\n",
        frame_count, frame_dt, step_dt, expected, sim_steps, last_tick_dt);
}

void run_d5() {
    const double frame_dt = 1.0 / 60.0;
    const double step_dt = 1.0 / 1000.0;
    const int frame_count = 60;
    const int rate_div = 2;

    flecs::world ecs;
    int sim_steps = 0;
    double last_tick_dt = 0.0;

    auto sim_tick = ecs.timer("engine::SimTick").interval(step_dt);
    auto rate_filter = ecs.timer("engine::Rate").rate(rate_div, sim_tick);
    sim_tick.start();

    ecs.system("SimStep")
        .kind(flecs::PreUpdate)
        .tick_source(rate_filter)
        .run([&](flecs::iter& it) {
            sim_steps++;
            last_tick_dt = it.delta_system_time();
        });

    for (int i = 0; i < frame_count; ++i) {
        ecs.progress(frame_dt);
    }

    double total_time = frame_count * frame_dt;
    int expected = (int)((total_time / step_dt) + 1e-6);
    std::printf("== D5: tick_source(rate(sim_tick, 2)) ==\n");
    std::printf("frames=%d frame_dt=%.4f sim_dt=%.4f rate=%d expected=%d sim=%d last_tick_dt=%.4f\n",
        frame_count, frame_dt, step_dt, rate_div, expected, sim_steps, last_tick_dt);
}

void run_d6() {
    const double target_fps = 60.0;
    const double tick_dt = 1.0 / 1000.0;
    const int frame_count = 60;

    flecs::world ecs;
    int sim_steps = 0;
    double elapsed = 0.0;
    double last_tick_dt = 0.0;

    ecs.system("FrameAccum")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            elapsed += it.delta_time();
        });

    ecs.system("SimStep")
        .kind(flecs::PreUpdate)
        .interval(tick_dt)
        .run([&](flecs::iter& it) {
            sim_steps++;
            last_tick_dt = it.delta_system_time();
        });

    ecs.app()
        .target_fps(target_fps)
        .frames(frame_count)
        .run();

    int expected = (int)((elapsed / tick_dt) + 1e-6);
    std::printf("== D6: app target_fps + system.interval ==\n");
    std::printf("frames=%d target_fps=%.0f elapsed=%.3f interval=%.4f expected=%d sim=%d last_tick_dt=%.4f\n",
        frame_count, target_fps, elapsed, tick_dt, expected, sim_steps, last_tick_dt);
}

void run_d8() {
    const double sim_fps = 1000.0;
    const double render_fps = 60.0;
    const int render_frames = 60;
    const double sim_dt = 1.0 / sim_fps;
    const double render_dt = 1.0 / render_fps;
    const double elapsed = render_frames * render_dt;
    const int sim_steps_total = (int)((elapsed * sim_fps) + 1e-6);

    flecs::world sim;
    flecs::world render;
    int sim_steps = 0;
    int render_steps = 0;
    double sim_time = 0.0;
    double render_time = 0.0;

    sim.system("SimStep")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            sim_steps++;
            sim_time += it.delta_time();
        });

    render.system("Render")
        .kind(flecs::OnUpdate)
        .run([&](flecs::iter& it) {
            render_steps++;
            render_time += it.delta_time();
        });

    int base = sim_steps_total / render_frames;
    int extra = sim_steps_total % render_frames;
    int min_steps = base;
    int max_steps = base + (extra > 0 ? 1 : 0);

    for (int frame = 0; frame < render_frames; ++frame) {
        int steps = base + (frame < extra ? 1 : 0);
        for (int s = 0; s < steps; ++s) {
            sim.progress(sim_dt);
        }
        render.progress(render_dt);
    }

    std::printf("== D8: dual world fixed ratio (single thread) ==\n");
    std::printf("render_frames=%d render_dt=%.4f sim_dt=%.4f sim_target=%d sim=%d render=%d min_steps=%d max_steps=%d sim_time=%.3f render_time=%.3f\n",
        render_frames, render_dt, sim_dt, sim_steps_total, sim_steps, render_steps, min_steps, max_steps, sim_time, render_time);
}

void run_d9() {
    const double sim_fps = 1000.0;
    const double render_fps = 60.0;
    const double sim_dt = 1.0 / sim_fps;
    const int sim_steps_target = 1000;
    const int render_rate = (int)(sim_fps / render_fps + 0.5);

    flecs::world ecs;
    int sim_steps = 0;
    int render_ticks = 0;

    auto sim_tick = ecs.system("SimTick")
        .interval(sim_dt)
        .run([&](flecs::iter&) {
            sim_steps++;
        });

    ecs.system("RenderTick")
        .tick_source(sim_tick)
        .rate(render_rate)
        .run([&](flecs::iter&) {
            render_ticks++;
        });

    for (int i = 0; i < sim_steps_target; ++i) {
        ecs.progress(sim_dt);
    }

    int expected_render = sim_steps_target / render_rate;
    double render_fps_actual = sim_fps / (double)render_rate;
    std::printf("== D9: render tick from sim tick (nested tick source) ==\n");
    std::printf("sim_fps=%.0f render_fps=%.0f rate=%d sim=%d render=%d expected=%d render_fps_actual=%.1f\n",
        sim_fps, render_fps, render_rate, sim_steps, render_ticks, expected_render, render_fps_actual);
}

}  // namespace

int main() {
    run_d();
    run_d1();
    run_d2();
    run_d3();
    run_d4();
    run_d5();
    run_d6();
    run_d8();
    run_d9();
    return 0;
}
