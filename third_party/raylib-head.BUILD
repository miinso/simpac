# third_party/raylib.BUILD
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

RAYLIB_SRCS = [
    "src/rcore.c",
    "src/rshapes.c",
    "src/rtextures.c",
    "src/rtext.c",
    "src/utils.c",
    "src/rmodels.c",
    # "src/raudio.c",
]

RAYLIB_HDRS = [
    "src/raylib.h",
    "src/rlgl.h",
    "src/raymath.h",
    "src/rcamera.h",
    "src/rgestures.h",
    "src/utils.h",
]

# platform specific source files
GLFW_SRCS = ["src/rglfw.c"]

# common compile flags
COMMON_COPTS = [
    "-D_GNU_SOURCE",
    "-fno-strict-aliasing",
    "-Wno-missing-braces",
    "-Werror=pointer-arith",
]

# windows specific flags for msvc
WINDOWS_MSVC_COPTS = [
]

# platform specific compile flags
PLATFORM_COPTS = select({
    "@platforms//os:windows": [
        "/D_GNU_SOURCE",
        "/DPLATFORM_DESKTOP",
        "/DGRAPHICS_API_OPENGL_33",
        "/wd4056",  # overflow in floating-point constant arithmetic
        "/wd4244",  # conversion from 'type1' to 'type2', possible loss of data
        "/wd4305",  # truncation from 'double' to 'float'
    ],
    "@platforms//os:linux": COMMON_COPTS + [
        "-std=c99",
        "-DPLATFORM_DESKTOP",
        "-DGRAPHICS_API_OPENGL_33",
        "-fPIC",
    ],
    "@platforms//os:macos": COMMON_COPTS + [
        "-std=c99",
        "-DPLATFORM_DESKTOP",
        "-DGRAPHICS_API_OPENGL_33",
        "-fPIC",
    ],
    "@platforms//os:emscripten": COMMON_COPTS + [
        "-std=gnu99",
        "-DPLATFORM_WEB",
        "-DGRAPHICS_API_OPENGL_ES3",
        "-DRLGL_SHOW_GL_DETAILS_INFO",
    ],
    # default to desktop build instead of web
    "//conditions:default": COMMON_COPTS + [
        "-std=c99",
        "-DPLATFORM_DESKTOP",
        "-DGRAPHICS_API_OPENGL_33",
    ],
})

# platform specific link options
PLATFORM_LINKOPTS = select({
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
    "@platforms//os:macos": [
        "-framework",
        "CoreVideo",
        "-framework",
        "IOKit",
        "-framework",
        "Cocoa",
        "-framework",
        "GLUT",
        "-framework",
        "OpenGL",
    ],
    "@platforms//os:emscripten": [
        "-sUSE_GLFW=3",
        # "-sFORCE_FILESYSTEM=1",
        "-sUSE_WEBGL2=1",
        "-sMIN_WEBGL_VERSION=2",
        "-sMAX_WEBGL_VERSION=2",
        # "-sGL_SUPPORT_AUTOMATIC_ENABLE_EXTENSIONS=0",
        # "-sGL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS=0",
        # "-lidbfs.js", # https://emscripten.org/docs/api_reference/Filesystem-API.html#filesystem-api-idbfs
    ],  # emscripten handles this
    "//conditions:default": [],
})

# select platform specific sources
PLATFORM_SRCS = select({
    "@platforms//os:emscripten": [],  # web platform doesn't use rglfw
    "//conditions:default": GLFW_SRCS,  # use embedded glfw by default
})

# raylib library target
cc_library(
    name = "raylib-head",
    srcs = RAYLIB_SRCS + PLATFORM_SRCS,
    hdrs = RAYLIB_HDRS,
    copts = PLATFORM_COPTS,
    includes = ["src"],
    linkopts = PLATFORM_LINKOPTS,
    deps = select({
        "@platforms//os:emscripten": [],  # no glfw for web
        "//conditions:default": ["@glfw2"],  # use glfw for desktop
    }),
)

# headers-only target
cc_library(
    name = "raylib_headers",
    hdrs = RAYLIB_HDRS,
    includes = ["src"],
    visibility = ["//visibility:public"],
)
