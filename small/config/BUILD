load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

assets_files = glob(["assets/**"])

filegroup(
    name = "assets",
    srcs = assets_files,
)

cc_binary(
    name = "main",
    srcs = [
        "main.cpp",
    ],
    data = [
        ":assets",
        "//:resources",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
        ],
        "//conditions:default": [],
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
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=16mb",
            "-sINITIAL_HEAP=16mb",
            "-sMAXIMUM_MEMORY=1024mb",
            "-sSINGLE_FILE",
            "-sEXIT_RUNTIME=0",
            "-O2",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','wasmMemory']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize','_flecs_explorer_request']",
            "--embed-file %s/assets@assets" % package_name(),
            "--embed-file resources/fonts/generic.fnt@resources/fonts/generic.fnt",
            "--embed-file resources/fonts/generic.png@resources/fonts/generic.png",
            "--embed-file resources/fonts/tiny.png@resources/fonts/tiny.png",
        ],
        "//conditions:default": [],
    }),
    deps = [
        "@flecs",
        "//modules:graphics2",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":main",
    outputs = [],
    tags = ["wasm"],
    threads = "off",
)
