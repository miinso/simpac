cc_library(
    name = "raylib",
    srcs = select({
        "@platforms//os:macos": ["lib/libraylib.a"], # universal
        "@platforms//os:windows": ["lib/raylib.lib"], # "@platforms//cpu:x86_64", TODO: mingw support
        "@platforms//os:linux": ["lib/libraylib.a"], # "@platforms//cpu:x86_64",
        "@platforms//os:emscripten": ["lib/libraylib.a"], # "@platforms//cpu:wasm32",
    }),
    hdrs = glob(["include/*.h"]),
    includes = ["include"],
    linkopts = select({
        "@platforms//os:macos": [
            "-framework", "CoreVideo",
            "-framework", "IOKit",
            "-framework", "Cocoa",
            "-framework", "GLUT",
            "-framework", "OpenGL",
        ],
        "@platforms//os:windows": [
            "-DEFAULTLIB:gdi32.lib",
            "-DEFAULTLIB:opengl32.lib",
            "-DEFAULTLIB:user32.lib",
            "-DEFAULTLIB:winmm.lib",
            "-DEFAULTLIB:shell32.lib",
        ],
        "@platforms//os:linux": [
            "-lGL",
            "-lm",
            "-lpthread",
            "-ldl",
            "-lrt",
            "-lX11",
        ],
        "@platforms//os:emscripten": [
            "-sUSE_GLFW=3",
            "-sFORCE_FILESYSTEM=1",
            # "-sASSERTIONS=1",
            # "--profiling",
        ],
    }),
    visibility = ["//visibility:public"],
)