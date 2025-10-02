load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

cpp_file_names = glob(["*.cpp"])

[
    cc_binary(
        name = cpp_file_name.replace(".cpp", ""),
        srcs = [cpp_file_name],
        copts = [
            "-DPLATFORM_DESKTOP",
        ],
        data = [
            "@com_minseopark_simpac//:resources",
        ],
        includes = [
            "raygui.h",
            # "hw2aux.h",
            # "rlights.h",
        ],
        deps = [
            "@raylib",
            "@flecs",
            "@eigen",
            # modules
            "//modules:graphics_module",
            "//modules:graphics",
            "//modules:model_module",
            "//modules:hw2aux",
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
        data = ["@com_minseopark_simpac//:resources"],
        includes = [
            "rlights.h",
        ],
        deps = [
            "@raylib",
            "@flecs",
        ],
    )
    for c_file_name in c_file_names
]

cc_binary(
    name = "window-test",
    srcs = [
        # "asset_loading.cpp",
        "hw3b.cpp",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["@com_minseopark_simpac//:resources"],
    includes = [
        "raygui.h",
        # "rlights.h",
    ],
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sWASM=1",
            "-sALLOW_MEMORY_GROWTH=0",
            "-sNO_EXIT_RUNTIME=0",
            "-sSTACK_SIZE=1000000",
            "-sSINGLE_FILE",
            # "-pthread", # requires wasm_cc_binary `threads` flag to be raised
            # "-Os",

            # "--preload-file examples/index.html@resources/index.html",
            "--preload-file resources/shaders/glsl100/lighting.fs@resources/shaders/glsl100/lighting.fs",
            "--preload-file resources/shaders/glsl100/lighting.vs@resources/shaders/glsl100/lighting.vs",
            "--preload-file resources/shaders/glsl120/pbr.fs@resources/shaders/glsl120/pbr.fs",
            "--preload-file resources/shaders/glsl120/pbr.vs@resources/shaders/glsl120/pbr.vs",
            # "--preload-file resources/note1.ogg@resources/note1.ogg",
            # "--preload-file resources/note2.ogg@resources/note2.ogg",
            # "--preload-file resources/note3.ogg@resources/note3.ogg",
            # "--preload-file resources/note4.ogg@resources/note4.ogg",
            # "--preload-file resources/note0.ogg@resources/note0.ogg",
        ],
        "//conditions:default": [],
    }),
    deps = [
        "@raylib",
        "@eigen",
        "@flecs",
        # modules
        "//modules:graphics_module",
        "//modules:model_module",
        "//modules:hw2aux",
    ],
)

# wasm_cc_binary(
#     name = "wasm-window-test",
#     cc_target = ":window-test",
#     outputs = [
#         "hw3.js",
#         # "index.wasm",
#         "hw3.data",
#     ],
#     simd = False,
#     standalone = False,
#     threads = "off",
# )

# https://github.com/google-ai-edge/mediapipe/issues/4919

