load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = [
        "main.cpp",
        "ui.h",
    ],
    data = ["//:resources"],
    deps = [
        "//modules:graphics2",
        "@raygui",
        "@flecs",
    ],
)

cc_binary(
    name = "main2",
    srcs = [
        "main2.cpp",
        "ui.h",
    ],
    data = ["//:resources"],
    deps = [
        "//modules:graphics2",
        "@raygui",
        "@flecs",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":main",
    tags = ["wasm"],
    threads = "off",
)
