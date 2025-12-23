load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

### wasm
cc_binary(
    name = "main",
    srcs = [
        "components.h",
        "main.cpp",
        "systems.h",
        "utils.h",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-DPLATFORM_WEB",
            "-Os",
            "-g0",
            "-DNDEBUG",
        ],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["//:resources"],
    defines = [
    ],
    includes = [
    ],
    linkopts = select({
        "@platforms//os:emscripten": [
            # "-sWASM=1",
            "-sALLOW_MEMORY_GROWTH=1",
            # "-sNO_EXIT_RUNTIME=0",
            "-sSTACK_SIZE=1mb",
            "-sSINGLE_FILE",
            # "-sASSERTIONS=1",
            # "-sDISABLE_EXCEPTION_CATCHING=2",  # Enable exception catching in Emscripten
            # "-sERROR_ON_UNDEFINED_SYMBOLS=0",
            # "-pthread",  # requires wasm_cc_binary `threads` flag to be raised
            # "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
            # "-sPROXY_TO_PTHREAD",
            # "--shell-file resources/index.html",
            "-Os",
            # "-sWASM_BIGINT",
            # "-flto",
            "-g0",
            # MODULARIZE for SPA reloading
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            # "-sUSE_GLFW=3",
            # "-sUSE_WEBGL2=1",
            # "-sMIN_WEBGL_VERSION=2",
            # "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            # "-sEXPORT_ES6=1",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize']",
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
    cc_target = ":main",
    # backend = "emscripten",
    outputs = [
        # "mass-spring.js",
        # "index.wasm",
        # "app1.data",
        # "index.html",
    ],
    # simd = True,
    # standalone = True, # i don't want this. it's a wasi thing
    tags = ["wasm"],
    threads = "off",
    # threads = ["_default", "emscripten", "off"],
)
