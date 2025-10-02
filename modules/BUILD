load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "graphics_module",
    srcs = [
        "graphics_module.cpp",
        "include/graphics_module.h",
        "include/rlights.h",
    ],
    hdrs = [
        "include/graphics_module.h",
        "include/rlights.h",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["@com_minseopark_simpac//:resources"],
    strip_include_prefix = "include",
    # includes = ["include"],
    # linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "@raylib",
        "@eigen",
        "@flecs",
    ],
)

cc_library(
    name = "graphics",
    srcs = [
        "graphics.cpp",
        # "include/graphics_module2.h",
        # "include/rlights.h",
    ],
    hdrs = [
        "include/graphics.h",
        "include/lights.hpp",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["@com_minseopark_simpac//:resources"],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [
        "@raylib",
        "@eigen",
        "@flecs",
        "@raylib-cpp",
    ],
)

cc_library(
    name = "model_module",
    srcs = [
        "include/model_module.h",
        "model_module.cpp",
    ],
    hdrs = [
        "include/model_module.h",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [
        # "@raylib",
        "@eigen",
        "@flecs",
    ],
)

cc_library(
    name = "hw2aux",
    srcs = [
        "hw2aux.cpp",
        "include/hw2aux.h",
    ],
    hdrs = [
        "include/hw2aux.h",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [
        "@raylib",
        "@eigen",
        "@flecs",
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

