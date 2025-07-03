load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(
    default_visibility = ["//visibility:public"],
)

cc_library(
    name = "library1",
    srcs = ["game.cpp"],
    hdrs = ["game.h"],
    deps = [
        "//survivors/modules:engine",  # holds all the internal (sub)modules
        # user defined moduels below
        "//survivors/modules:player",
        "//survivors/modules:ai",
        "//survivors/modules:gameplay",
        "//survivors/modules:debug",
        # third_party below
        "//:raylib",
        "@flecs",
    ],
)

cc_binary(
    name = "main1",
    srcs = ["main1.cpp"],
    copts = select({
        "@platforms//os:emscripten": [
            "-DPLATFORM_WEB",
            # "-Os",
            # "-g0",
            "-DNDEBUG",
        ],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sWASM=1",
            "-sALLOW_MEMORY_GROWTH=1",
            # "-sNO_EXIT_RUNTIME=0",
            "-sSTACK_SIZE=1000000",
            "-sSINGLE_FILE=0",
            # "-sASSERTIONS=1",
            # "-sDISABLE_EXCEPTION_CATCHING=2",  # Enable exception catching in Emscripten
            # "-sERROR_ON_UNDEFINED_SYMBOLS=0",
            # "-pthread",  # requires wasm_cc_binary `threads` flag to be raised
            # "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
            # "-sPROXY_TO_PTHREAD",
            # "--shell-file resources/index.html",
            # "-Os",
            # "-flto",
            "-sGL_ENABLE_GET_PROC_ADDRESS=1",
            # "-g0",
            "-sUSE_GLFW=3",
            "-sUSE_WEBGL2=1",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            # "-sENVIRONMENT=web,worker",
            "-sENVIRONMENT=web",
            # "-sEXPORT_ES6=1",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main']",
            "--emit-html",
            "--html-title='huh?'",
            "--emrun",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":library1",
        # "//:raylib",
        # "@flecs",
    ],
)

cc_binary(
    name = "tmx",
    srcs = ["tmx.cpp"],
    data = ["//:resources"],
    deps = [
        "@tmxlite",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":main1",
    # backend = "emscripten",
    outputs = [
        # "mass-spring.js",
        # "index.wasm",
        # "app1.data",
        # "index.html",
        # "hello.js",
        # "hello.wasm",
    ],
    # simd = True,
    # standalone = True, # i don't want this. it's a wasi thing
    threads = "off",
    # threads = ["_default", "emscripten", "off"],
)
