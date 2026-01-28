load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "leak",
    srcs = ["main.cpp"],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
        ],
        "//conditions:default": [
            "-O2",
        ],
    }),
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
            "-O2",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=2mb",
            "-sINITIAL_MEMORY=16mb",
            "-sMODULARIZE=1",
            "-sASSERTIONS=2",
            "-sEXPORT_NAME=createModule",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','wasmMemory']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_leak_hold','_leak_clear','_leak_held_bytes','_leak_block_count']",
        ],
        "//conditions:default": [],
    }),
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":leak",
    outputs = [
        "leak.js",
        "leak.wasm",
    ],
    tags = ["wasm"],
    threads = "off",
)

cc_binary(
    name = "engine",
    srcs = ["engine.cpp"],
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
            "-O2",
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=16mb",
            "-sINITIAL_MEMORY=32mb",
            # "-sINITIAL_HEAP=16mb",
            "-sMAXIMUM_MEMORY=1024mb",
            "-sMODULARIZE=1",
            "-sSINGLE_FILE",
            "-sEXPORT_NAME=createModule",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','wasmMemory']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize','_leak_hold','_leak_clear','_leak_held_bytes','_leak_block_count','_emscripten_set_main_loop_timing']",
            "--embed-file resources/fonts/generic.fnt@resources/fonts/generic.fnt",
            "--embed-file resources/fonts/generic.png@resources/fonts/generic.png",
            # "-sCROSS_ORIGIN=1",
            #
            # "-sOFFSCREENCANVAS_SUPPORT=1",
            # "-sOFFSCREEN_FRAMEBUFFER=1",
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
    name = "engine_webapp",
    cc_target = ":engine",
    outputs = [
        "engine.js",
        # "engine.wasm",
    ],
    tags = ["wasm"],
    # threads = "emscripten",
    threads = "off",
)
