# Leak Playground

Minimal C++ + WASM target for testing module cleanup and memory growth.

## Build (WASM)

```powershell
bazel build //small/leak:webapp --config=wasm
bazel build //small/leak:engine_webapp --config=wasm
```

Outputs land in `bazel-bin/small/leak/` as:

- `leak.js` / `leak.wasm`
- `engine.js` / `engine.wasm`

## Run the HTML

Each HTML file expects its matching `.js`/`.wasm` in the same folder. For a
quick local test:

1. Copy `small/leak/index.html`, `small/leak/ff.html`,
   `small/leak/worker.html`, `small/leak/engine.html`, and
   `small/leak/engine-worker.html` into `bazel-bin/small/leak/`.
2. Copy `small/leak/simpac_worker_bridge.js` into `bazel-bin/small/leak/`
   (required by `engine-worker.html`).
3. Serve the folder and open the page.

```powershell
Set-Location bazel-bin/small/leak
// python -m http.server
serve
```

Open `http://localhost:8000/` in your browser.

## HTML files and observations

### index.html (main thread)

Purpose: minimal main-thread module lifecycle (create/destroy/hold/clear).

Observed:

- Firefox: destroy did not release memory; each create added ~16 MiB.
- Chrome/Brave: destroy released memory even when holds existed.

### ff.html (main thread + worker + iframe)

Purpose: compare isolation strategies.

Observed:

- Firefox: main thread + iframe did not release memory.
- Firefox: DevTools profiler did not show worker memory growth, but OS task
  manager did. Destroying the worker released memory even when holds existed.
- Chrome/Brave: main thread released memory; iframe did not.
- Worker startup initially failed due to strict MIME checks and invalid wasm
  URL when loading from a blob URL; updated `ff.html` fetches `leak.js` and
  `leak.wasm` explicitly (re-test after update).

### worker.html (manual worker)

Purpose: run `createModule()` inside a Worker (manual setup).

Observed:

- Manual worker works in both browsers (task manager indicates worker teardown
  releases memory). Browser profiler did not show worker memory growth.

Note: recent Emscripten releases no longer support `--proxy-to-worker`, so this
page uses a manual worker harness instead.

### engine.html (template-based engine) (we need to either fix the engine, since we know from index.html that chromiums do release memory)

Purpose: minimal raylib engine test (copied from `small/template` code).

Observed:

- Chromium variant (we hoped this to work) and ff (we knew this wouldn't work): memory did not release after destroy; keep it as minimal reproduction
  for engine behavior.

### engine-worker.html (engine in worker) (or to figure out how to present our wasm app in worker env) (pita expected..)

Purpose: run the template-based engine inside a Worker using OffscreenCanvas.

Observed:

- Both: all worked as expected. actually, the original engine.cpp manifested some choppy, broken drawing, so i tried pure raylib sample and it just worked.
only after lifting this "if (!is_render_thread()) return;" guard from graphics module, things worked. now, need to figure out how to pass mouse/keyboard event.

- Note: on brave, it.delta consitently showed 16ms but ff, fps varied like 45 fps ~ 50, and delta was alternating between 16ms to 33ms.
setting "moduleInstance._emscripten_set_main_loop_timing(0, 16);" didn't help ff to stall.

Initially, `graphics::begin` required an update to `graphics::shim::update()` to sync input state. This is now handled in the engine.
Framerate consistency varies by browser (Firefox vs Chrome) but functionality is stable.


## Firefox notes

Firefox often keeps `WebAssembly.Memory` backing store reserved even after a
module is GC eligible, so repeated create/destroy cycles can look like "leaks"
in process memory. Chrome/Brave tend to release or reuse the backing store
more aggressively, so memory appears to drop immediately.

If you need deterministic cleanup on Firefox, use one of these isolation
patterns and terminate the whole realm:

- Run the module inside a Worker and call `worker.terminate()`.
- Run the module inside an iframe and remove the iframe node.

`ff.html` demonstrates both approaches. Copy it next to `leak.js` the same way
as `index.html` and try the worker/iframe buttons.

Note: `ff.html` loads `leak.js` via `fetch()` and wraps it in a blob for the
Worker to avoid strict MIME checks, then fetches `leak.wasm` directly. Serve the
folder over HTTP (not `file://`) and keep `leak.wasm` next to `leak.js`.

For inspection, Firefox `about:memory` and manual GC can help confirm whether
memory is just reserved or actually retained.

## Exports

- `leak_hold(bytes)` allocate and hold memory
- `leak_clear()` free all held blocks
- `leak_held_bytes()` total bytes held
- `leak_block_count()` total blocks held
- `emscripten_shutdown()` clear held blocks before teardown
