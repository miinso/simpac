load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

assets_files = glob(["assets/**"])

filegroup(
    name = "assets",
    srcs = assets_files,
)

### wasm
cc_binary(
    name = "main",
    srcs = [
        "cloth.h",
        "components.h",
        "main.cpp",
        "systems.h",
        "systems/physics.h",
        "systems/rendering.h",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
            # "-g0",
            # "-DNDEBUG",
        ],
        "@platforms//os:windows": [
            # "/O3",
            # "/arch:AVX2",
            # "/openmp:experimental",
        ],
        "//conditions:default": [
            # "-fopenmp",
            # "-march=native",
            # "-O3",
        ],
    }),
    data = [
        ":assets",  # per-directory assets
        "//:resources",  # root assets
    ],
    defines = select({
        "@platforms//os:emscripten": [
            "PLATFORM_WEB",
        ],
        "//conditions:default": [
            "PLATFORM_DESKTOP",
            # "EIGEN_DONT_PARALLELIZE",
        ],
    }),
    includes = [
    ],
    linkopts = select({
        "@platforms//os:emscripten": [
            # with known memory usage and expected size
            # "-sALLOW_MEMORY_GROWTH=0",  # sometimes it's a good idea to clamp it
            # "-sSTACK_SIZE=16mb",
            # "-sINITIAL_MEMORY=100mb",
            # "-sINITIAL_HEAP=4mb",
            # with no picture of memory footprint
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=16mb",
            "-sINITIAL_HEAP=16mb",
            "-sMAXIMUM_MEMORY=1024mb", # paired with allow_memory_growth
            # ---
            "-sSINGLE_FILE",
            # "-sASSERTIONS=2",
            "-sEXIT_RUNTIME=0",  # include shutdown code for proper cleanup
            # "-pthread", # this introduces the SharedArrayBuffer
            "-O2",
            # "-g0",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            # "-sFULL_ES3=1",
            # "-sUSE_GLFW=3",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','wasmMemory']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize','_flecs_explorer_request']",
            "--embed-file resources/shaders/glsl300es/spring.fs@resources/shaders/glsl300es/spring.fs",
            "--embed-file resources/shaders/glsl300es/spring.vs@resources/shaders/glsl300es/spring.vs",
            "--embed-file resources/shaders/glsl300es/particle.fs@resources/shaders/glsl300es/particle.fs",
            "--embed-file resources/shaders/glsl300es/particle.vs@resources/shaders/glsl300es/particle.vs",
            "--embed-file resources/fonts/generic.fnt@resources/fonts/generic.fnt",
            "--embed-file resources/fonts/generic.png@resources/fonts/generic.png",
            "--embed-file resources/fonts/font.png@resources/fonts/font.png",
            # "--embed-file small/implicit-euler/scenes/default.flecs@small/implicit-euler/scenes/default.flecs",
            "--embed-file %s/assets@assets" % package_name(), # embed all under ./assets
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//modules:graphics2",
        "@eigen",
        "@flecs",
        "@raylib-latest",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":main",
    # backend = "emscripten",
    outputs = [
        # "mass-spring.js",
        # "index.wasm",
        # "app1.data",
        # "index.html",
    ],
    # simd = True,
    # standalone = True, # i don't want this. it's a wasi thing
    tags = ["wasm"],
    threads = "off",
    # threads = ["_default", "emscripten", "off"],
)
