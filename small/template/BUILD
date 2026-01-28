load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
        ],
        "//conditions:default": [
            "-O2",
        ],
    }),
    data = ["//:resources"],
    defines = select({
        "@platforms//os:emscripten": [
            "PLATFORM_WEB",
        ],
        "//conditions:default": [
            "PLATFORM_DESKTOP",
        ],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=16mb",
            "-sINITIAL_HEAP=16mb",
            "-sMAXIMUM_MEMORY=1024mb",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize']",
            "--embed-file resources/fonts/generic.fnt@resources/fonts/generic.fnt",
            "--embed-file resources/fonts/generic.png@resources/fonts/generic.png",
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//modules:graphics2",
        "@flecs",
        "@raylib-latest",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":main",
    tags = ["wasm"],
    threads = "off",
)
