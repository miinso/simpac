load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

cpp_file_names = glob(["*.cpp"])

[
    cc_binary(
        name = cpp_file_name.replace(".cpp", ""),
        srcs = [cpp_file_name],
        data = [
            "@com_minseopark_simpac//:all_resources",
        ],
        deps = [
            "//:raylib",
            "@flecs",
            "@eigen",
            # modules
            "//modules:graphics_module",
            "//modules:model_module",
        ],
    )
    for cpp_file_name in cpp_file_names
]

c_file_names = glob(["*.c"])

[
    cc_binary(
        name = c_file_name.replace(".c", ""),
        srcs = [c_file_name],
        copts = [
            "-DPLATFORM_DESKTOP",
        ],
        data = ["@com_minseopark_simpac//:all_resources"],
        includes = [
            "rlights.h",
        ],
        deps = [
            "//:raylib",
            "@flecs",
        ],
    )
    for c_file_name in c_file_names
]

cc_binary(
    name = "window-test",
    srcs = [
        "test_window.cpp",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sWASM=1",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sNO_EXIT_RUNTIME=0",
            # "-sASSERTIONS=1",
            "-sSTACK_SIZE=1000000",
            "-sSINGLE_FILE",
            "-Os",
            # "-fno-rtti",
            # "-fno-exceptions",
            # "-flto",
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//:raylib",
        "@eigen",
        "@flecs",
        # modules
        "//modules:graphics_module",
        "//modules:model_module",
    ],
)

wasm_cc_binary(
    name = "wasm-window-test",
    cc_target = ":window-test",
    outputs = [
        "index.js",
        # "index.wasm",
    ],
    simd = False,
    standalone = False,
    threads = "off",
)
