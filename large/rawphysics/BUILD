load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
# load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob(
    [
        # only include main_*.cpp files (excluding library cpp files)
        "main_*.cpp",
    ],
    exclude = [
        # particle-based files that use incompatible types.hpp from mass-spring1
        "main_spring1.cpp",
        "main_prefab.cpp",
    ],
)

[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name],
        copts = [
            "-DPLATFORM_DESKTOP",
        ],
        data = [
            # "//:resources",
            "//:resources"
        ],
        includes = [
        ],
        deps = [
            "//modules:graphics2",
            ":pbd",
            ":collision",
            "@raylib",
            "@eigen",
            "@flecs",
        ],
    )
    for source_file_name in source_file_name_list
]

cc_library(
    name = "collision",
    srcs = [
        "broad.cpp",
        "clipping.cpp",
        "collider.cpp",
        "epa.cpp",
        "gjk.cpp",
        "support.cpp",
    ],
    hdrs = [
        "broad.h",
        "clipping.h",
        "collider.h",
        "epa.h",
        "gjk.h",
        "support.h",
        "types.h",
    ],
    includes = [
        "broad.h",
        "clipping.h",
        "collider.h",
        "epa.h",
        "gjk.h",
        "support.h",
    ],
    deps = [
        "@raylib",
        "@eigen",
        "@flecs",
    ],
)

cc_library(
    name = "pbd",
    srcs = [
        "constraints.cpp",
    ],
    hdrs = [
        # common
        "types.h",

        # pbd
        "constraints.h",
        "entity.hpp",
        "kernels.hpp",
    ],
    includes = [
        # common
        "types.h",

        # pbd
        "constraints.h",
        "entity.hpp",
        "kernels.hpp",
    ],
    deps = [
        ":collision",
        "@raylib",
        "@eigen",
        "@flecs",
    ],
)

### wasm

cc_binary(
    name = "app1",
    srcs = [
        "main_orientation1.cpp",
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
        ],
        "//conditions:default": [],
    }),
    deps = [
        "//modules:graphics2",
        ":pbd",
        ":collision",
        "@raylib",
        "@eigen",
        "@flecs",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":app1",
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
