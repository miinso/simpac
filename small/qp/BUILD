load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    deps = ["@osqp-eigen"],
)

wasm_cc_binary(
    name = "main_wasm",
    cc_target = ":main",
    outputs = [
        "main.js",
        "main.wasm",
    ],
    threads = "off",
    tags = ["wasm"],
)

sh_test(
    name = "main_wasm_test",
    srcs = ["run_wasm.sh"],
    args = ["$(rootpaths :main_wasm)"],
    data = [":main_wasm"],
    tags = ["wasm"],
)
