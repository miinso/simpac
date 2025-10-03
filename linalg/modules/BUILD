load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

_CXX17_COPTS = select({
    "@platforms//os:windows": ["/std:c++17"],
    "//conditions:default": ["-std=c++17"],
})

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
    copts = _CXX17_COPTS,
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
    copts = _CXX17_COPTS,
    visibility = ["//linalg/modules:__pkg__"],
    deps = [
        ":core",
        # ":gui",
        "@raylib",
    ],
)

cc_library(
    name = "engine",
    copts = _CXX17_COPTS,
    deps = [
        ":core",
        ":rendering",
    ],
    visibility = ["//visibility:public"],
)
