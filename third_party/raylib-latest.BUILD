load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "raylib-latest",
    srcs = [
        "src/raudio.c",
        "src/rcore.c",
        "src/rmodels.c",
        "src/rshapes.c",
        "src/rtext.c",
        "src/rtextures.c",
        "src/utils.c",
    ],
    hdrs = glob([
        "src/*.h",
    ]),
    textual_hdrs = glob([
        "src/platforms/**/*.c",
        "src/platforms/**/*.h",
        "src/external/**/*.h",
        "src/external/**/*.hpp",
        "src/external/**/*.inl",
        "src/external/**/*.inc",
        "src/external/**/*.c",
    ], allow_empty = True),
    includes = ["src"],
    defines = select({
        "@platforms//os:windows": [
            "PLATFORM_DESKTOP",
            # "PLATFORM_DESKTOP_GLFW", # ANGEL introduces perf loss, suppress for now
            "PLATFORM_WINDOWS",
            "GRAPHICS_API_OPENGL_33"
            # "GRAPHICS_API_OPENGL_ES2",
            # "GLFW_INCLUDE_NONE",
        ],
        "@platforms//os:linux": [
            "PLATFORM_DESKTOP",
            "PLATFORM_LINUX",
            "GRAPHICS_API_OPENGL_33",
            "_GNU_SOURCE",
        ],
        "@platforms//os:macos": [
            "PLATFORM_DESKTOP",
            "PLATFORM_DESKTOP_GLFW", # for ANGLE, this is a must!
            "PLATFORM_OSX",
            "GRAPHICS_API_OPENGL_ES2", # should use ES3
            "GLFW_INCLUDE_NONE",
            "GL_SILENCE_DEPRECATION",
        ],
        "@platforms//os:emscripten": [
            "PLATFORM_WEB",
            # "GRAPHICS_API_OPENGL_ES2",
            "GRAPHICS_API_OPENGL_ES3", # if you're going for WebGL2
            "_GNU_SOURCE",
            "_DEFAULT_SOURCE",
        ],
        "//conditions:default": [
            "PLATFORM_DESKTOP",
            "GRAPHICS_API_OPENGL_33",
        ],
    }),
    copts = select({
        "@platforms//os:windows": [
            "/std:c11",
            "/D_CRT_SECURE_NO_WARNINGS",
            "/permissive-",
            "/Zc:preprocessor",
            "/W3",
        ],
        "@platforms//os:linux": [
            "-std=gnu99",
            "-fno-strict-aliasing",
            "-Werror=pointer-arith",
            "-Werror=implicit-function-declaration",
            "-Wno-missing-braces",
        ],
        "@platforms//os:macos": [
            "-std=gnu99",
            "-fno-strict-aliasing",
            "-Werror=pointer-arith",
            "-Werror=implicit-function-declaration",
            "-Wno-missing-braces",
        ],
        "@platforms//os:emscripten": [
            "-std=gnu99",
            "-fno-strict-aliasing",
            "-Werror=pointer-arith",
            "-Werror=implicit-function-declaration",
            "-Wno-missing-braces",
            "-Iexternal/glfw/include",
        ],
        "//conditions:default": [
            "-std=gnu99",
            "-fno-strict-aliasing",
            "-Wno-missing-braces",
        ],
    }),
    linkopts = select({
        "@platforms//os:windows": [
            "/DEFAULTLIB:user32.lib",
            "/DEFAULTLIB:gdi32.lib",
            "/DEFAULTLIB:opengl32.lib",
            "/DEFAULTLIB:winmm.lib",
            "/DEFAULTLIB:imm32.lib",
            "/DEFAULTLIB:ole32.lib",
            "/DEFAULTLIB:shell32.lib",
            "/DEFAULTLIB:advapi32.lib",
            "/DEFAULTLIB:version.lib",
            "/DEFAULTLIB:uuid.lib",
        ],
        "@platforms//os:linux": [
            "-lm",
            "-lpthread",
            "-ldl",
            "-lrt",
            "-lGL",
            "-lX11",
            "-lXrandr",
            "-lXinerama",
            "-lXi",
            "-lXcursor",
        ],
        "@platforms//os:macos": [
            "-framework", "Cocoa",
            "-framework", "OpenGL",
            "-framework", "IOKit",
            "-framework", "CoreVideo",
        ],
        "@platforms//os:emscripten": [
            "-sUSE_GLFW=3",
            # "-sEXPORTED_RUNTIME_METHODS=ccall",
            # if you're going for WebGL2
            "-sUSE_WEBGL2=1",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
        ],
        "//conditions:default": [],
    }),
    deps = select({
        "@platforms//os:windows": [
            "@glfw2//:glfw2",
            # "@simpac//third_party/angle",
            # "@simpac//third_party/glad2"
        ],
        "@platforms//os:linux": ["@glfw2//:glfw2"],
        "@platforms//os:macos": [
            "@glfw2//:glfw2", 
            "@simpac//third_party/angle",
        ],
        "@platforms//os:emscripten": [], # no need
        "//conditions:default": [],
    }),
)
