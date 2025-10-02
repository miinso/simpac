load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "app",
    srcs = ["app.cpp"],
    hdrs = ["app.h"],
    deps = [
        # "//ecs2/modules:core",
        # "//ecs2/modules:physics",
        "//ecs2/modules:rendering",
        "@eigen",
        "@flecs",
    ],
)

cc_binary(
    name = "2d_laplace",
    srcs = ["2d_laplace.cpp"],
    deps = [
        "@eigen",
        "@raylib",
    ],
)

cc_binary(
    name = "2d_laplace_interactive",
    srcs = ["2d_laplace_interactive.cpp"],
    deps = [
        "@eigen",
        "@raylib",
    ],
)
