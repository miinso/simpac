load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

_CXX17_COPTS = select({
    "@platforms//os:windows": ["/std:c++17"],
    "//conditions:default": ["-std=c++17"],
})

_THREAD_COPTS = select({
    "@platforms//os:linux": ["-pthread"],
    "@platforms//os:macos": ["-pthread"],
    "//conditions:default": [],
})

_THREAD_LINKOPTS = select({
    "@platforms//os:linux": ["-pthread"],
    "@platforms//os:macos": ["-pthread"],
    "//conditions:default": [],
})

filegroup(
    name = "web_shell",
    srcs = ["web_shell.html"],
)

cc_library(
    name = "info",
    srcs = ["info.hpp"],
    hdrs = ["info.hpp"],
    deps = [
        "@cpuinfo",
    ],
)

cc_library(
    name = "app",
    srcs = ["app.cpp"],
    hdrs = ["app.h"],
    copts = _CXX17_COPTS + _THREAD_COPTS,
    data = [":web_shell"],
    linkopts = _THREAD_LINKOPTS,
    deps = [
        "//linalg/modules:engine",
        "@eigen",
        "@flecs",
    ],
)

cc_binary(
    name = "2d_laplace",
    srcs = ["2d_laplace.cpp"],
    copts = _CXX17_COPTS,
    deps = [
        "@eigen",
        # "@raylib",
        "@raylib-latest",
    ],
)

cc_binary(
    name = "2d_laplace_interactive",
    srcs = ["2d_laplace_interactive.cpp"],
    copts = _CXX17_COPTS,
    data = [":web_shell"],
    linkopts = select({
        "@platforms//os:emscripten": [
            # "-sSINGLE_FILE", # put everything into html
            "-sALLOW_MEMORY_GROWTH=1",
            "-sASSERTIONS=1",
            "-sMINIFY_HTML=0",
            "--oformat=html",
            "--shell-file=$(location :web_shell)",
            # more optimizations:
            "-O3",  # Whole-program optimization
            "--OPT_LEVEL=3",  # Emscripten-specific optimizer level
            "--SHRINK_LEVEL=2",  # Aggressively minimize JS glue
            "-sASSERTIONS=0",  # Remove runtime assertions
            "-sSAFE_HEAP=0",  # Disable safety checks on heap accesses
            "-sABORTING_MALLOC=0",  # Keep running on OOM
            # "-sALLOW_MEMORY_GROWTH=0", # Freeze heap size for determinism
            # "--closure=1",           # Run Closure Compiler on JS glue (requires Java)
            # "--minify 1",            # Disable JS minify if debugging size issues
            # "-sFILESYSTEM=0",        # Strip POSIX FS stubs when not needed
            "-sENVIRONMENT=web",  # Restrict to web environment only
            "-sLEGACY_GL_EMULATION=0",  # Ensure legacy GL shims are off
            "-sSTANDALONE_WASM=0",  # Skip standalone mode when targeting browsers
        ],
        "//conditions:default": [],
    }),
    deps = [
        "@eigen",
        "@raylib-latest",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":2d_laplace_interactive",
    outputs = [
        "app1.html",
        "app1.js",
        "app1.wasm",
    ],
    threads = "off",
)

cc_binary(
    name = "mt",
    srcs = ["mt.cpp"],
    copts = select({
        "@platforms//os:linux": ["-pthread"],
        "@platforms//os:macos": ["-pthread"],
        "//conditions:default": [],
    }),
    data = [":web_shell"],
    defines = select({
        "@platforms//os:emscripten": [
            "PLATFORM_WEB",
        ],
        "//conditions:default": ["PLATFORM_DESKTOP"],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            "-pthread",
            "-sUSE_PTHREADS=1",
            "-sSTACK_SIZE=1000000",
            "--oformat=html",
            "-sMINIFY_HTML=0",
            "--shell-file=$(location :web_shell)",
            # "-sPROXY_TO_PTHREAD=1",
            "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
            # "-sUSE_GLFW=3",
            # "-sOFFSCREENCANVAS_SUPPORT=1",
            # "-sOFFSCREEN_FRAMEBUFFER=1", # https://groups.google.com/g/emscripten-discuss/c/b9u2Mt_9lJw/m/NunM9h-GAAAJ
            # "-sWASM_EXCEPTIONS=1",
            # "-sNO_DISABLE_EXCEPTION_CATCHING",
            # "-sEXCEPTION_CATCHING_ALLOWED",
            # "-sASSERTIONS=1",
            # "-fexceptions",
            # "-fwasm-exceptions",
            # "-sUSE_WEBGL2=1",
            # "-sMIN_WEBGL_VERSION=2",
            # "-sMAX_WEBGL_VERSION=2",
            # "-sALLOW_MEMORY_GROWTH=1",
            # "-sMALLOC=mimalloc",
            # "-sEXIT_RUNTIME=1",
            # "-Wl,--no-undefined",
            # "-ldl",
        ],
        "@platforms//os:linux": ["-pthread"],
        "@platforms//os:macos": ["-pthread"],
        "//conditions:default": [],
    }),
    deps = [
        ":app",
        ":info",
        "@onetbb//:tbb",
    ],
)

wasm_cc_binary(
    name = "mt_web",
    cc_target = ":mt",
    outputs = [
        "mt.html",
        "mt.js",
        "mt.wasm",
    ],
    threads = "emscripten",
)
