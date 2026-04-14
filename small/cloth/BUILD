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
        "components/particle.h",
        "components/constraint.h",
        "components/render.h",
        "sim/model.h",
        "sim/state.h",
        "sim/bridge.h",
        "math/spring.h",
        "queries.h",
        "vars.h",
        "systems.h",
        "setup.h",
        "render/systems.h",
        "render/draw.h",
        "render/gpu.h",
        "render/interaction.h",
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

# regression benchmark
cc_binary(
    name = "bench",
    srcs = [
        "bench/bench.cpp",
    ],
    data = [
        "bench/bench.flecs",
    ],
    deps = [
        ":sim",
        "@google_benchmark//:benchmark",
    ],
)

# a/** -- per-entity ecs pattern
[cc_binary(
    name = "a_" + f.replace("a/", "").replace(".cpp", ""),
    srcs = [f],
    data = [":assets", "//:resources"] + assets_files,
    additional_linker_inputs = ["//:resources"] + assets_files,
    deps = [":sim"],
) for f in glob(["a/*.cpp"])]

# b/** -- staging buffer pattern
[cc_binary(
    name = "b_" + f.replace("b/", "").replace(".cpp", ""),
    srcs = [f],
    data = [":assets", "//:resources"] + assets_files,
    additional_linker_inputs = ["//:resources"] + assets_files,
    deps = [":sim"],
) for f in glob(["b/*.cpp"])]

### wasm
cc_binary(
    name = "main",
    srcs = [
        "b/implicit.cpp",
    ],
    data = [
        ":assets",
        "//:resources",
    ] + assets_files,
    additional_linker_inputs = ["//:resources"] + assets_files,
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=16mb",
            "-sINITIAL_HEAP=16mb",
            "-sMAXIMUM_MEMORY=1024mb",
            "-sSINGLE_FILE",
            "-sWASMFS",
            "-sEXIT_RUNTIME=0",
            "-O2",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
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
    outputs = [],
    tags = ["wasm"],
    threads = "off",
)
