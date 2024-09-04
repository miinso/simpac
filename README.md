# simpac

A meek peon, simulation package of mine.


## How to build wasm binary?

1. Compile flecs as wasm static lib

2. Compile raylib as wasm static lib

3. Compile demo as wasm linking flecs and raylib

TODO:
- [ ] test `-sMODULARIZE=1`
- [ ] test resources dir with wasm build
- [ ] test simd
- [ ] test threads

## Loops

1. main.cpp while loop
2. emscripten rAF loop (browser only)
3. ecs.progress()
4. ecs.system<T>("integrator").kind(flecs::OnUpdate).each(integrate_particle)

Where does the renderer.draw() go? 

1. Put it in the ecs system and call the ecs.progress() in the loop (while or rAF)
> it's possible for native while loop but not browser rAF, because rAF has
control.