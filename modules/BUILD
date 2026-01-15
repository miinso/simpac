load("@rules_cc//cc:cc_library.bzl", "cc_library")

# graphics module - shared many time accross the projects
cc_library(
    name = "graphics2",
    srcs = ["graphics/graphics.cpp"],
    hdrs = [
        "graphics/graphics.h",
        "graphics/camera.h",
        "graphics/lighting.h",
        "graphics/pipelines.h",
        "graphics/resources.h",
    ],
    copts = select({
        "@platforms//os:windows": ["-DPLATFORM_DESKTOP"],
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    strip_include_prefix = "graphics",
    visibility = ["//visibility:public"],
    deps = [
        "@flecs",
        "@raylib-latest",
        "//third_party/par_shapes",
    ] + select({
        "@platforms//os:emscripten": [],
        "//conditions:default": ["@bazel_tools//tools/cpp/runfiles"],
    }),
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

cc_library(
    name = "raygui",
    hdrs = [
        "raygui/raygui.h",
    ],
    strip_include_prefix = "raygui",
    visibility = ["//visibility:public"],
    deps = [
        "@raylib",
        # "@raylib-latest",
    ],
)

