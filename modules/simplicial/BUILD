load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "simplicial",
    srcs = [
        "simplicial.cpp",
        "topology.cpp",
        "raylib_mesh.cpp",
    ],
    hdrs = [
        "simplicial.h",
        "topology.h",
        "mechanical_state.h",
        "raylib_mesh.h",
    ],
    include_prefix = "",
    strip_include_prefix = "",
    visibility = ["//visibility:public"],
    deps = [
        "@eigen",
        "@flecs",
        # "@raylib-latest",
        "@raylib",
    ],
)
