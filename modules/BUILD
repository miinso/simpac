load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

# graphics module - shared many time accross the projects
cc_library(
    name = "graphics2",
    srcs = [
        "graphics/fonts.cpp",
        "graphics/graphics.cpp",
    ] + select({
        "@platforms//os:emscripten": ["graphics/graphics_platform_wasm.cpp"],
        "//conditions:default": ["graphics/graphics_platform_native.cpp"],
    }),
    hdrs = [
        "graphics/types.h",
        "graphics/camera.h",
        "graphics/components.h",
        "graphics/fonts.h",
        "graphics/graphics.h",
        "graphics/input.h",
        "graphics/lighting.h",
        "graphics/materials.h",
        "graphics/pipelines.h",
        "graphics/resources.h",
        "graphics/systems.h",
        "graphics/tags.h",
        "graphics/vars.h",
    ],
    copts = select({
        "@platforms//os:windows": [],  # win
        "@platforms//os:emscripten": [],  # browser
        "//conditions:default": [],  # mac, linux
    }),
    defines = select({
        "@platforms//os:emscripten": [
            "PLATFORM_WEB",
        ],
        "//conditions:default": [
            "PLATFORM_DESKTOP",
        ],
    }),
    strip_include_prefix = "graphics",
    visibility = ["//visibility:public"],
    deps = [
        "//third_party/par_shapes",
        "@flecs",
        "@raylib-latest",
        "@eigen",
    ] + select({
        "@platforms//os:emscripten": [],
        "//conditions:default": ["@bazel_tools//tools/cpp/runfiles"],
    }),
)

cc_test(
    name = "graphics_test",
    srcs = ["graphics/graphics_test.cpp"],
    deps = [
        ":graphics2",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "graphics_test_wasm_cc",
    srcs = ["graphics/graphics_test.cpp"],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
            "-DPLATFORM_WEB"
        ],
        "//conditions:default": [],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sENVIRONMENT=node",
            "-sEXIT_RUNTIME=0",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sUSE_PTHREADS=0",
            "-sSINGLE_FILE",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize','_flecs_explorer_request']",
            "-sSTACK_SIZE=64mb",
            "-sINITIAL_MEMORY=512mb",
            "-sMODULARIZE=1",
        ],
        "//conditions:default": [],
    }),
    features = select({
        "@platforms//os:emscripten": ["-use_pthreads"],
        "//conditions:default": [],
    }),
    tags = ["manual"],
    deps = [
        ":graphics2",
        "@googletest//:gtest_main",
    ],
)

wasm_cc_binary(
    name = "graphics_test_wasm",
    cc_target = ":graphics_test_wasm_cc",
    outputs = [
        "graphics_test.js",
        "graphics_test.wasm",
    ],
    tags = ["wasm"],
    threads = "off",
)

py_test(
    name = "graphics_test_wasm_node",
    srcs = ["graphics/graphics_test_node.py"],
    args = ["$(rootpaths :graphics_test_wasm)"],
    data = [
        ":graphics_test_wasm",
    ],
    main = "graphics/graphics_test_node.py",
    tags = ["wasm"],
    deps = [
        "@bazel_tools//tools/python/runfiles",
    ],
)

cc_library(
    name = "pbd",
    srcs = [
        "pbd/BoundingSphereHierarchy.cpp",
        "pbd/IndexedFaceMesh.cpp",
        "pbd/VolumeIntegration.cpp",
    ],
    hdrs = [
        "pbd/AABB.h",
        "pbd/BoundingSphere.h",
        "pbd/BoundingSphereHierarchy.h",
        "pbd/Common.h",
        "pbd/IndexedFaceMesh.h",
        "pbd/ParticleData.h",
        "pbd/VolumeIntegration.h",
        "pbd/kdTree.h",
    ],
    strip_include_prefix = "pbd",
    visibility = ["//visibility:public"],
    deps = [
        "@eigen",
    ],
)

cc_library(
    name = "raylib-gizmo",
    srcs = [
        "raygizmo/raygizmo.c",
    ],
    hdrs = [
        "raygizmo/raygizmo.h",
    ],
    strip_include_prefix = "raygizmo",
    visibility = ["//visibility:public"],
    deps = [
        "@raylib-latest",
    ],
)

cc_library(
    name = "rlights",
    hdrs = [
        "rlights/rlights.h",
    ],
    strip_include_prefix = "rlights",
    visibility = ["//visibility:public"],
    deps = [
        # "@raylib",
        "@raylib-latest",
    ],
)
