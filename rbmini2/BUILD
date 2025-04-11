# rbmini2/BUILD
load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
    name = "core",
    hdrs = [
        "core/components.hpp",
        "core/tags.hpp",
        "core/types.hpp",
        "util/logging.hpp",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        "//modules:pbd",
        "@eigen",
        "@loguru",
    ],
)

cc_library(
    name = "geometry",
    # srcs = [], internals go here
    hdrs = [
        "geometry/components.hpp",
        "geometry/operations.hpp",
        "geometry/types.hpp",
        "systems/geometry.hpp",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        "//:raylib",
        "//modules:pbd",
        "@eigen",
        "@flecs",
    ],
)

cc_library(
    name = "dynamics",
    hdrs = [
        "dynamics/forces.hpp",
        "dynamics/integration.hpp",
        "dynamics/types.hpp",
        "systems/dynamics.hpp",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        "//modules:pbd",
        "@eigen",
    ],
)

cc_library(
    name = "sdf",
    hdrs = [
        "sdf/analytic.hpp",
        "sdf/components.hpp",
        "sdf/discrete.hpp",
        "sdf/operations.hpp",
        "sdf/types.hpp",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        "//modules:pbd",
        "@Discregrid",
        "@eigen",
        "@flecs",
    ],
)

cc_library(
    name = "collision",
    hdrs = [
        "collision/broadphase.hpp",
        "collision/components.hpp",
        "collision/factory.hpp",
        "collision/narrowphase.hpp",
        "collision/resolution.hpp",
        "collision/types.hpp",
        "systems/collision.hpp",
    ],
    includes = ["."],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":geometry",
        ":sdf",
        "@Discregrid",
        "@eigen",
        "@flecs",
    ],
)

# cc_library(
#     name = "constraints",
#     hdrs = [
#         "constraints/types.hpp",
#         "constraints/components.hpp",
#         "constraints/solvers.hpp",
#         "systems/constraints.hpp",
#     ],
#     deps = [
#         ":core",
#         "@eigen",
#         "@flecs",
#     ],
#     visibility = ["//visibility:public"],
# )

# cc_library(
#     name = "debug",
#     hdrs = [
#         "debug/types.hpp",
#         "debug/components.hpp",
#         "debug/primitives.hpp",
#         "debug/physics_visualization.hpp",
#         "debug/simulation_visualization.hpp",
#         "systems/debug.hpp",
#         "systems/debug_physics.hpp",
#         "systems/debug_simulation.hpp",
#     ],
#     deps = [
#         ":core",
#         ":geometry",
#         ":collision",
#         ":constraints",
#         "//:raylib",
#         "@eigen",
#         "@flecs",
#     ],
#     visibility = ["//visibility:public"],
# )

# cc_library(
#     name = "profiling",
#     hdrs = [
#         "profiling/types.hpp",
#         "profiling/components.hpp",
#         "profiling/monitoring.hpp",
#         "systems/profiling.hpp",
#     ],
#     deps = [
#         ":core",
#         "//:raylib",
#         "@eigen",
#         "@flecs",
#     ],
#     visibility = ["//visibility:public"],
# )

# cc_library(
#     name = "util",
#     hdrs = [
#         "util/logging.hpp",
#     ],
#     visibility = ["//visibility:public"],
#     deps = [
#         "@loguru",
#     ],
# )

cc_library(
    name = "graphics",
    srcs = [
        "graphics/graphics.cpp",
    ],
    hdrs = [
        "graphics/graphics.h",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["@com_minseopark_simpac//:resources"],
    includes = ["."],
    deps = [
        "//:raylib",
        "@eigen",
        "@flecs",
    ],
)

########## engine above

# main apps
[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name],
        copts = [
            "-DPLATFORM_DESKTOP",
        ],
        data = [
            "@com_minseopark_simpac//:resources",
        ],
        defines = [
            "LOGGING_ENABLED",
            "USE_LOGURU",
        ],
        deps = [
            ":core",
            ":geometry",
            ":dynamics",
            ":collision",
            # ":constraints",
            # ":debug",
            # ":profiling",
            "//:raylib",
            "@flecs",
            "@eigen",
            # "@Discregrid",
            "//modules:pbd",
            ":graphics",
        ],
    )
    for source_file_name in glob([
        "*.cpp",
        "*.c",
    ])
]

cc_binary(
    name = "app1",
    srcs = [
        "main1.cpp",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-DPLATFORM_WEB",
            # "-fexceptions",  # Enable C++ exception handling
            # "-sWASM_EXCEPTIONS=1",  # Enable WebAssembly exception support
            # "-sEXCEPTION_DEBUG=1",  # Enable exception debugging
        ],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["@com_minseopark_simpac//:resources"],
    defines = [
        "LOGGING_ENABLED",
        # "USE_LOGURU",
    ],
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sWASM=1",
            "-sALLOW_MEMORY_GROWTH=0",
            "-sDISABLE_EXCEPTION_CATCHING=0",  # Enable exception catching in Emscripten
            "-sNO_EXIT_RUNTIME=0",
            "-sSTACK_SIZE=1000000",
            "-sSINGLE_FILE",
            # "-fexceptions",
            # "-fno-exceptions",
            # "-pthread", # requires wasm_cc_binary `threads` flag to be raised
            # "-Os",
            "--preload-file resources/shaders/glsl100/lighting.fs@resources/shaders/glsl100/lighting.fs",
            "--preload-file resources/shaders/glsl100/lighting.vs@resources/shaders/glsl100/lighting.vs",
            "--preload-file resources/shaders/glsl120/pbr.fs@resources/shaders/glsl120/pbr.fs",
            "--preload-file resources/shaders/glsl120/pbr.vs@resources/shaders/glsl120/pbr.vs",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":collision",
        ":core",
        ":dynamics",
        ":geometry",
        ":graphics",
        "//:raylib",
        "//modules:pbd",
        "@eigen",
        "@flecs",
    ],
)

wasm_cc_binary(
    name = "app1-wasm",
    cc_target = ":app1",
    outputs = [
        "app1.js",
        # "index.wasm",
        "app1.data",
    ],
    simd = True,
    # standalone = False,
    # threads = "off",
)

########## tests below

cc_test(
    name = "core_test",
    srcs = ["test/core_test.cpp"],
    deps = [
        ":core",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "geometry_test",
    srcs = ["test/geometry_test.cpp"],
    deps = [
        ":geometry",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "sdf_test",
    srcs = ["test/sdf_test.cpp"],
    deps = [
        ":sdf",
        "@googletest//:gtest_main",
    ],
)
