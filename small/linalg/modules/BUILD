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
    ],
    includes = [".."],
    visibility = ["//linalg/modules:__pkg__"],
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
        "engine/rendering/queries.h",
        "engine/rendering/rendering_module.h",
        "engine/rendering/systems.h",
    ],
    includes = ["."],
    visibility = ["//linalg/modules:__pkg__"],
    deps = [
        ":core",
        # ":gui",
        "@raylib",
    ],
)

cc_library(
    name = "engine",
    deps = [
        ":core",
        ":rendering",
    ],
    visibility = ["//visibility:public"],
)
