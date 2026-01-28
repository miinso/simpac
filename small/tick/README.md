# Tick demo

Minimal demo for 60fps render vs 1000Hz simulation steps.

Files:
- `small/tick/main.cpp`
- `small/tick/BUILD`

Run:

```bash
bazel run //small/tick:main
```

What to look for:
- D: `tick_source` only. Shows that a 1000Hz timer still runs once per frame. (fail)
- D1: accumulator + `progress(step_dt)` to reach ~1000 sim steps. (success)
- D2: manual `sim_step.run(step_dt)` + `progress(frame_dt)` so render stays 60fps. (success)
- D3: `system.interval(0.001)` (still capped by frame rate). (fail)
- D4: `rate(sim_tick, 1)` (rate filter on timer tick source). (fail)
- D5: `tick_source(rate(sim_tick, 2))` (chained rate filter). (fail)
- D6: `app().target_fps(60)` + `system.interval(0.001)` (app loop, still capped). (fail)
- D8: two worlds, fixed ratio schedule per render frame (single thread). (success)
- D9: render tick derived from sim tick using system tick sources + rate (integer rate means render fps is quantized). (kind of?)

TODO: find out what happens if a kernel execution takes longer than the given dt

Result:

```bash
== D: tick_source only (60fps -> 1000Hz) ==
frames=60 frame_dt=0.0167 sim_dt=0.0010 expected=1000 pre=60 on=60 last_tick_dt=0.0167
== D1: accumulator + progress(step_dt) ==
frames=60 frame_dt=0.0167 sim_dt=0.0010 expected=1000 sim=1000 leftover=-0.000000
== D2: manual sim + render progress ==
frames=60 frame_dt=0.0167 sim_dt=0.0010 expected=1000 sim=1000 render=60 leftover=-0.000000
== D3: system.interval(0.001) ==
frames=60 frame_dt=0.0167 interval=0.0010 expected=1000 sim=60 last_tick_dt=0.0167
== D4: rate(sim_tick, 1) ==
frames=60 frame_dt=0.0167 sim_dt=0.0010 expected=1000 sim=60 last_tick_dt=0.0167
== D5: tick_source(rate(sim_tick, 2)) ==
frames=60 frame_dt=0.0167 sim_dt=0.0010 rate=2 expected=1000 sim=30 last_tick_dt=0.0333
== D6: app target_fps + system.interval ==
frames=60 target_fps=60 elapsed=1.016 interval=0.0010 expected=1016 sim=60 last_tick_dt=0.0171
== D8: dual world fixed ratio (single thread) ==
render_frames=60 render_dt=0.0167 sim_dt=0.0010 sim_target=1000 sim=1000 render=60 min_steps=16 max_steps=17 sim_time=1.000 render_time=1.000
== D9: render tick from sim tick (nested tick source) ==
sim_fps=1000 render_fps=60 rate=17 sim=1000 render=58 expected=58 render_fps_actual=58.8
```