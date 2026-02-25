load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

### wasm
cc_binary(
    name = "main",
    srcs = [
        "main.cpp",
        "components.h",
        "systems.h",
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
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=1000000",
            "-sSINGLE_FILE",
            "-Os",
            "-g0",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            "-sUSE_GLFW=3",
            "-sUSE_WEBGL2=1",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize']",
            "--embed-file resources/shaders/glsl300es/pbr.fs@resources/shaders/glsl300es/pbr.fs",
            "--embed-file resources/shaders/glsl300es/pbr.vs@resources/shaders/glsl300es/pbr.vs",
            "--embed-file resources/shaders/glsl300es/depth.fs@resources/shaders/glsl300es/depth.fs",
            "--embed-file resources/shaders/glsl300es/depth.vs@resources/shaders/glsl300es/depth.vs",
            "--embed-file resources/models/plane.glb@resources/models/plane.glb",
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//modules:graphics2",
        "//modules:raylib-gizmo",
        "@eigen",
        "@flecs",
        "@raylib-latest",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":main",
    # backend = "emscripten",
    # outputs = [
    #     "gizmo1.js",
    #     # "index.wasm",
    #     "gizmo1.data",
    #     # "index.html",
    # ],
    # simd = True,
    # standalone = True, # i don't want this. it's a wasi thing
    tags = ["wasm"],
    threads = "off",
    # threads = ["_default", "emscripten", "off"],
)
