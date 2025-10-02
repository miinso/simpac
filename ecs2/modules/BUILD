load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "core",
    srcs = [
        "engine/core/core_module.cpp",
    ],
    hdrs = [
        "base_module.h",
        "engine/core/components.h",
        "engine/core/core_module.h",
        "engine/core/geometry.h",
    ],
    includes = [".."],
    deps = [
        "@eigen",
        "@flecs",
    ],
)

cc_library(
    name = "rendering",
    srcs = [
        "engine/rendering/rendering_module.cpp",
    ],
    hdrs = [
        "engine/rendering/components.h",
        "engine/rendering/pipelines.h",
        # "engine/rendering/queries.h",
        "engine/rendering/rendering_module.h",
        "engine/rendering/systems.h",
    ],
    includes = ["."],
    deps = [
        ":core",
        # ":gui",
        "@raylib",
    ],
)

cc_library(
    name = "physics",
    srcs = [
        "engine/physics/physics_module.cpp",
    ],
    hdrs = [
        "engine/physics/components.h",
        "engine/physics/physics_module.h",
        "engine/physics/pipelines.h",
        # "engine/physics/queries.h",
        "engine/physics/systems.h",
    ],
    includes = ["."],
    deps = [
        ":core",
        "@raylib",  # NOTE: make this third_party:raylib or such
    ],
)
