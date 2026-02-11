load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")

# per-file embed opts keep wasm link cache invalidation working on asset edits.
assets_files = glob(["assets/**"])
assets_embed_linkopts = [
    opt
    for f in assets_files
    for opt in ["--embed-file", "$(location %s)@%s" % (f, f)]
]

filegroup(
    name = "assets",
    srcs = assets_files,
)

cc_library(
    name = "sim",
    srcs = [
        "components.h",
        "components/base.h",
        "components/core.h",
        "components/render.h",
        "components/solver.h",
        "physics.h",
        "physics/integration.h",
        "physics/spring.h",
        "queries.h",
        "vars.h",
        "systems.h",
        "systems/render/module.h",
        "systems/render/draw.h",
        "systems/render/gpu.h",
        "systems/render/interaction.h",
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
    defines = select({
        "@platforms//os:emscripten": [
            "PLATFORM_WEB",
        ],
        "//conditions:default": [
            "PLATFORM_DESKTOP",
            # "EIGEN_DONT_PARALLELIZE",
        ],
    }),
    deps = [
        "//modules:graphics2",
        "@eigen",
        "@flecs",
        "@raylib-latest",
    ],
)

### wasm
cc_binary(
    name = "main",
    srcs = [
        "main_implicit.cpp",
    ],
    data = [
        ":assets",  # per-directory assets
        "//:resources",  # root assets
    ] + assets_files,
    additional_linker_inputs = ["//:resources"] + assets_files,
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
            "--embed-file resources/fonts/tiny.png@resources/fonts/tiny.png",
            # "--embed-file small/cloth/scenes/default.flecs@small/cloth/scenes/default.flecs",
        ] + assets_embed_linkopts,
        "//conditions:default": [],
    }),
    deps = [
        ":sim",
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
