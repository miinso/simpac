load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob(["*.cpp"], allow_empty = True)

# defines = ["NDEBUG"] == copts = ["-DNDEBUG"]

[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name] + [
            "components.h",
            "systems.h",
        ],
        deps = [
            "//modules:graphics2",
            "@eigen",
            "@flecs",
            "@raylib-latest",
        ],
    )
    for source_file_name in source_file_name_list
]

cc_library(
    name = "some-shit",
    srcs = [
        "module/some-shit.cpp",
    ],
    hdrs = [
        "module/some-shit.h",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    strip_include_prefix = "module",
    deps = [
    ],
)

cc_binary(
    name = "app1",
    srcs = [
        "main_heavy.cpp",
        "components.h",
        "systems.h",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-DPLATFORM_WEB",
            "-fno-rtti",
            "-fno-exceptions",
            "-pthread",
            "-Os",
            "-g0",
        ],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=1000000",
            "-sSINGLE_FILE",
            "-sASSERTIONS=1",
            "-sDISABLE_EXCEPTION_CATCHING=2",
            "-sERROR_ON_UNDEFINED_SYMBOLS=0",
            "-pthread",
            "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
            "-Os",
            "-sWASM_BIGINT",
            "-g0",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=simpac",
            "-sUSE_GLFW=3",
            "-sUSE_WEBGL2=1",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            "-sEXPORT_ES6=1",
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//modules:graphics2",
        "@eigen",
        "@flecs",
        "@raylib-latest",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":app1",
    # backend = "emscripten",
    # outputs = [
    # "app.js",
    # "index.wasm",
    # "app1.data",
    # "index.html",
    # ],
    # simd = True,
    # standalone = True, # i don't want this
    tags = ["wasm"],
    threads = "emscripten",
    # threads = ["_default", "emscripten", "off"],
)
