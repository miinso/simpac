load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    copts = select({
        "@platforms//os:windows": ["-DPLATFORM_DESKTOP"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    deps = [
        "//modules:graphics2",
        "//modules/simplicial",
    ],
)
