load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob([
    "*.c",
    "*.cpp",
])

# defines = ["NDEBUG"] == copts = ["-DNDEBUG"]

[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name],
        # data = ["@com_minseopark_simpac//:resources"],
        includes = [
            "components.hpp",
            "pbd1.hpp",  # mk.1
        ],
        deps = [
            ":graphics",
            ":some-shit",
            "@eigen",
            "@flecs",
            "@raylib-latest",
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
        "@raylib-latest",
    ],
)

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
    data = ["@com_minseopark_simpac//:resources"],
    strip_include_prefix = "module",
    deps = [
    ],
)

cc_binary(
    name = "app1",
    srcs = [
        "main_heavy.cpp",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-DPLATFORM_WEB",
            # "-fexceptions",  # Enable C++ exception handling
            # "-sWASM_EXCEPTIONS=1",  # Enable WebAssembly exception support
            # "-sEXCEPTION_DEBUG=1",  # Enable exception debugging
            "-fno-rtti",
            "-fno-exceptions",
            "-pthread",
            "-Os",
            # "-flto",
            "-g0",
            # "-DNDEBUG"
        ],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    # data = ["@com_minseopark_simpac//:resources"],
    defines = [
        # "LOGGING_ENABLED",
        # "USE_LOGURU",
    ],
    includes = [
        "components.hpp",
        "pbd1.hpp",  # mk.1
    ],
    linkopts = select({
        "@platforms//os:emscripten": [
            # "-sWASM=1",
            "-sALLOW_MEMORY_GROWTH=1",
            # "-sNO_EXIT_RUNTIME=0",
            "-sSTACK_SIZE=1000000",
            "-sSINGLE_FILE",
            "-sASSERTIONS=1",
            "-sDISABLE_EXCEPTION_CATCHING=2",  # Enable exception catching in Emscripten
            "-sERROR_ON_UNDEFINED_SYMBOLS=0",
            "-pthread",  # requires wasm_cc_binary `threads` flag to be raised
            "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
            # "-sPROXY_TO_PTHREAD",
            # "--shell-file resources/index.html",
            "-Os",
            "-sWASM_BIGINT",
            # "-flto",
            "-g0",
            # vue
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
        ":graphics",
        ":some-shit",
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
    threads = "emscripten",
    # threads = ["_default", "emscripten", "off"],
)
