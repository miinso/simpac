load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "explorer_demo",
    srcs = ["main.cpp"],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
        ],
        "//conditions:default": [
            "-O2",
        ],
    }),
    data = [
        "//:resources",
        "pre_canvas.js",
    ],
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
            "--pre-js",
            "$(location pre_canvas.js)",
            "-sSTACK_SIZE=16mb",
            "-sINITIAL_HEAP=16mb",
            "-sMAXIMUM_MEMORY=1024mb",
            "-sMODULARIZE=1",
            "-sSINGLE_FILE",
            "-sEXPORT_NAME=createModule",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize','_flecs_explorer_request']",
            "-sASSERTIONS=2",
            # "-sSAFE_HEAP=1",
            # "-sSTACK_OVERFLOW_CHECK=2",
            "--embed-file resources/generic.fnt@resources/generic.fnt",
            "--embed-file resources/generic.png@resources/generic.png",
            # "--extern-pre-js modules/graphics/simpac_worker_bridge.js"
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
    cc_target = ":explorer_demo",
    outputs = [
        "explorer_demo.js",
    ],
    tags = ["wasm"],
    threads = "off",
)
