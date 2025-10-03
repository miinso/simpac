load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

filegroup(
    name = "web_shell",
    srcs = ["web_shell.html"],
)

cc_library(
    name = "app",
    srcs = ["app.cpp"],
    hdrs = ["app.h"],
    deps = [
        "//linalg/modules:engine",
        "@eigen",
        "@flecs",
    ],
)

cc_binary(
    name = "2d_laplace",
    srcs = ["2d_laplace.cpp"],
    deps = [
        "@eigen",
        # "@raylib",
        "@raylib-latest",
    ],
)

cc_binary(
    name = "2d_laplace_interactive",
    srcs = ["2d_laplace_interactive.cpp"],
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
