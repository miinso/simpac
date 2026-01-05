load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")

cc_binary(
    name = "asset_loading",
    srcs = ["asset_loading.cpp"],
    data = ["//:resources"],
)

cc_library(
    name = "common",
    srcs = ["common/geometry.cpp"],
    hdrs = [
        "common/components.h",
        "common/conversions.h",
        "common/geometry.h",
        "common/random.h",
        "common/spatial_hash.h",
    ],
    include_prefix = "common",
    strip_include_prefix = "common",
    deps = [
        "@eigen",
        "@flecs",
        "@raylib",
    ],
)

cc_binary(
    name = "boids",
    srcs = [
        "boids/boids.cpp",
        "boids/components.h",
        "boids/systems.h",
        "boids/utils.h",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-DPLATFORM_WEB",
            "-DNDEBUG",
            "-msse4",
        ],
        "@platforms//os:windows": ["-DPLATFORM_DESKTOP"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=1mb",
            "-sSINGLE_FILE",
            "-sASSERTIONS=0",
            "-pthread",  # requires wasm_cc_binary `threads` flag to be raised
            # "-Os",
            "-g0",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize']",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":common",
        "//modules:graphics2",
        "@eigen",
        "@flecs",
        "@raylib",
    ],
)

cc_binary(
    name = "boids_original",
    srcs = [
        "boids/boids_original.cpp",
        "boids/components.h",
        "boids/systems.h",
        "boids/utils.h",
    ],
    copts = select({
        "@platforms//os:windows": ["-DPLATFORM_DESKTOP"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    deps = [
        ":common",
        "//modules:graphics2",
        "@eigen",
        "@flecs",
        "@raylib",
    ],
)

cc_binary(
    name = "particle_mesh",
    srcs = [
        "particle_mesh/components.h",
        "particle_mesh/particle_mesh.cpp",
        "particle_mesh/systems.h",
        "particle_mesh/utils.h",
    ],
    copts = select({
        "@platforms//os:windows": ["-DPLATFORM_DESKTOP"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    deps = [
        ":common",
        "//modules:graphics2",
        "@eigen",
        "@flecs",
        "@raylib",
    ],
)

cc_binary(
    name = "particle_plane",
    srcs = [
        "particle_plane/components.h",
        "particle_plane/particle_plane.cpp",
        "particle_plane/systems.h",
        "particle_plane/utils.h",
    ],
    copts = select({
        "@platforms//os:windows": ["-DPLATFORM_DESKTOP"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    deps = [
        ":common",
        "//modules:graphics2",
        "//modules:raygui",
        "@eigen",
        "@flecs",
        "@raylib",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":boids",
    outputs = [
    ],
    simd = True,
    tags = ["wasm"],
    threads = "emscripten",
)
