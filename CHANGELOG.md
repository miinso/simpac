### r12 (_March 10, 2026_)

- lab(spmv): add COO, CSR, ELL kernels with benchmarks
- deps: add rules_cuda, opencl
- feat(graphics): DOM polyfill for worker input, touch support
- chore: make repo public, clean git history

### r11 (_February 25, 2026_)

- feat(graphics): add material, lighting, shadow support
- feat(vue): add engine helper libs, repl component, emoji shortcodes
- build(cloth): enable wasmfs for web build
- bench(cloth): add benchmark target
- refactor(cloth): split core/physics/render layers, drag interaction
- refactor(graphics): modularize runtime setup

### r10 (_January 29, 2026_)

- fix(cloth): warm-starting recovery bug, fresh topology per frame
- feat(cloth): add more scenes, particle drag, ECS setup extraction
- feat(graphics): tiny font, resource path fallbacks
- feat(remote): add filesystem support, fix encoding
- lab: locality bench, tick source, entity.get vs ref.get bench

### r9 (_January 21, 2026_)

- feat(graphics): worker + offscreen canvas, reflections, flecs lifecycle
- feat(cloth): particle picker, particle renderer, spring instancing
- lab(thread): bench serial/omp/tbb x native/wasm
- lab: flecs remote api, wasm leak test, vitepress test bed

### r8 (_January 5, 2026_)

- feat: implicit euler cloth solver with hessian assembly
- feat: ANGLE support (windows + macOS), spring renderer
- feat(graphics): camera component
- feat: wallclock + simulation time separation
- deps: add glad2, ANGLE, par_shapes

### r7 (_December 12, 2025_)

- modules: add unified graphics2 module
- lab: add 2d laplace examples
- lab: try simd ops on desktop/wasm targets, tbb with emcc
- build: opt in for cpp17, rework glfw2 and raylib-latest deps
- deps: bump eigen 5.0.1, add cpuinfo, tmxlite
- refactor: migrate lab/rb-try1, lab/pbd1, raw to Flecs 4.x API

### r6 (_April 11, 2025_)

- lab: pbd rigid body sim
- lab: try bvh implementation
- lab: ecs base with components and systems (solve2d)

### r5 (_December 13, 2024_)

- chore: bump deps

### r4 (_September 11, 2024_)

- lab: add animation assignment projects
- feat: redo engine loop
- build: drop legacy deps and targets

### r3 (_August 27, 2024_)

- feat: implement graphics module main loop
- build: add Emscripten build target

### r2 (_May 21, 2024_)

- feat: implement osx graphics module

### r1 (_May 21, 2024_)

- chore: so help me god

[comment]: # "fix|feat|perf|lab|chore|refactor|type|build|docs"
