load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "app",
    srcs = ["app.cpp"],
    hdrs = ["app.h"],
    deps = [
        "//ecs2/modules:core",
        "//ecs2/modules:physics",
        "//ecs2/modules:rendering",
    ],
)

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    deps = [
        ":app",
    ],
)

cc_binary(
    name = "test",
    srcs = ["test.cpp"],
    deps = [
        ":app",
    ],
)
