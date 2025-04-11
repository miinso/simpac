load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob([
    "*.c",
    "*.cpp",
])

[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name],
        copts = [
            "-DPLATFORM_DESKTOP",
        ],
        data = [
            "@com_minseopark_simpac//:resources",
        ],
        includes = [
        ],
        deps = [
            ":graphics",
            "//modules:raylib-gizmo",
            "//modules:rlights",
            "@eigen",
            "@flecs",
            "@raylib-head",
        ],
    )
    for source_file_name in source_file_name_list
]

cc_library(
    name = "graphics",
    srcs = [
        "graphics/graphics.cpp",
    ],
    hdrs = [
        "graphics/graphics.h",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["@com_minseopark_simpac//:resources"],
    includes = ["."],
    strip_include_prefix = "graphics",
    deps = [
        "@eigen",
        "@flecs",
        "@raylib-head",
    ],
)
### wasm

cc_binary(
    name = "fs1",
    srcs = [
        "main_shader.cpp",
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
    data = ["@com_minseopark_simpac//:resources"],
    defines = [
    ],
    includes = [
    ],
    linkopts = select({
        "@platforms//os:emscripten": [
            # "-sWASM=1",
            "-sALLOW_MEMORY_GROWTH=1",
            # "-sNO_EXIT_RUNTIME=0",
            "-sSTACK_SIZE=1000000",
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
            # vue
            # "-sMODULARIZE=1",
            # "-sEXPORT_NAME=simpac",
            "-sUSE_GLFW=3",
            "-sUSE_WEBGL2=1",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            # "-sEXPORT_ES6=1",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown']",
            # "--preload-file resources/shaders/glsl300es/pbr.fs@resources/shaders/glsl300es/pbr.fs",
            # "--preload-file resources/shaders/glsl300es/pbr.vs@resources/shaders/glsl300es/pbr.vs",
            # "--preload-file resources/models/plane.glb@resources/models/plane.glb",
            "--embed-file resources/shaders/glsl300es/pbr.fs@resources/shaders/glsl300es/pbr.fs",
            "--embed-file resources/shaders/glsl300es/pbr.vs@resources/shaders/glsl300es/pbr.vs",
            "--embed-file resources/shaders/glsl300es/reload.fs@resources/shaders/glsl300es/reload.fs",
            "--embed-file resources/models/plane.glb@resources/models/plane.glb",
            "--shell-file resources/wasm/index.html",

            # filesystem
            # "-lidbfs.js",
        ],
        "//conditions:default": [],
    }),
    deps = [
        ":graphics",
        "//modules:raylib-gizmo",
        "//modules:rlights",
        "@eigen",
        "@flecs",
        "@raylib-head",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":fs1",
    # backend = "emscripten",
    # outputs = [
    #     "gizmo1.js",
    #     # "index.wasm",
    #     "gizmo1.data",
    #     # "index.html",
    # ],
    # simd = True,
    # standalone = True, # i don't want this. it's a wasi thing
    threads = "off",
    # threads = ["_default", "emscripten", "off"],
)
