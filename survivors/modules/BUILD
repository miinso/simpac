load("@rules_cc//cc:cc_library.bzl", "cc_library")

# package(default_visibility = ["//visibility:public"])

cc_library(
    name = "core",
    srcs = [
        "engine/core/core_module.cpp",
    ],
    hdrs = [
        "base_module.h",
        "engine/core/components.h",
        "engine/core/core_module.h",
        "engine/core/queries.h",
        "engine/core/systems.h",
    ],
    includes = [".."],  # so that <modules/base_module.h> works
    deps = [
        "@eigen",
        "@flecs",
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
        "engine/physics/queries.h",
        "engine/physics/systems.h",
    ],
    includes = ["."],
    deps = [
        ":core",
        "//:raylib",  # NOTE: make this third_party:raylib or such
    ],
)

cc_library(
    name = "input",
    srcs = [
        "engine/input/input_module.cpp",
    ],
    hdrs = [
        "engine/input/components.h",
        "engine/input/input_module.h",
        "engine/input/systems.h",
    ],
    includes = ["."],
    deps = [
        ":core",
        "//:raylib",  # glfw key events
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
        "engine/rendering/queries.h",
        "engine/rendering/rendering_module.h",
        "engine/rendering/systems.h",
    ],
    includes = ["."],
    deps = [
        ":core",
        ":gui",
        "//:raylib",
    ],
)

cc_library(
    name = "gui",
    srcs = [
        "engine/rendering/gui/gui_module.cpp",
    ],
    hdrs = [
        "engine/rendering/gui/components.h",
        "engine/rendering/gui/gui_module.h",
        "engine/rendering/gui/systems.h",
    ],
    includes = ["."],
    deps = [
        ":core",
        "//:raylib",
        "//modules:raygui",  # TODO: make it external repo and handle resources too
    ],
)

cc_library(
    name = "engine",
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":input",
        ":physics",
        ":rendering",
    ],
)

##
cc_library(
    name = "player",
    srcs = [
        "player/player_module.cpp",
    ],
    hdrs = [
        "player/components.h",
        "player/player_module.h",
        "player/systems.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":input",
        ":physics",
    ],
)

cc_library(
    name = "ai",
    srcs = [
        "ai/ai_module.cpp",
    ],
    hdrs = [
        "ai/ai_module.h",
        "ai/components.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":physics",
    ],
)

cc_library(
    name = "gameplay",
    srcs = [
        "gameplay/gameplay_module.cpp",
    ],
    hdrs = [
        "gameplay/components.h",
        "gameplay/gameplay_module.h",
        "gameplay/pipelines.h",
        "gameplay/queries.h",
        "gameplay/systems.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":physics",
        ":rendering",
    ],
)

cc_library(
    name = "debug",
    srcs = [
        "debug/debug_module.cpp",
    ],
    hdrs = [
        "debug/components.h",
        "debug/debug_module.h",
        "debug/systems.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":physics",
        ":rendering",
    ],
)

cc_library(
    name = "tilemap",
    srcs = [
        "tilemap/tilemap_module.cpp",
    ],
    hdrs = [
        "tilemap/systems.h",
        "tilemap/tilemap_module.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":core",
        ":physics",
        ":rendering",
        "@tmxlite",
    ],
)
