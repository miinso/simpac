load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Shared sources that are compiled on every platform
_COMMON_SRCS = [
    "src/context.c",
    "src/egl_context.c",
    "src/init.c",
    "src/input.c",
    "src/monitor.c",
    "src/platform.c",
    "src/vulkan.c",
    "src/window.c",
    "src/osmesa_context.c",
    "src/null_init.c",
    "src/null_joystick.c",
    "src/null_monitor.c",
    "src/null_window.c",
]

# Headers that are used internally while building GLFW
_COMMON_INTERNAL_HDRS = [
    "src/internal.h",
    "src/mappings.h",
    "src/platform.h",
    "src/null_platform.h",
    "src/null_joystick.h",
]

# Public headers exposed to dependents
_PUBLIC_HDRS = [
    "include/GLFW/glfw3.h",
    "include/GLFW/glfw3native.h",
]

# Platform specific additions -------------------------------------------------

_WINDOWS_SRCS = [
    "src/win32_init.c",
    "src/win32_joystick.c",
    "src/win32_module.c",
    "src/win32_monitor.c",
    "src/win32_thread.c",
    "src/win32_time.c",
    "src/win32_window.c",
    "src/wgl_context.c",
]

_WINDOWS_HDRS = [
    "src/win32_platform.h",
    "src/win32_joystick.h",
    "src/win32_time.h",
    "src/win32_thread.h",
]

_LINUX_SRCS = [
    "src/glx_context.c",
    "src/linux_joystick.c",
    "src/posix_module.c",
    "src/posix_poll.c",
    "src/posix_thread.c",
    "src/posix_time.c",
    "src/x11_init.c",
    "src/x11_monitor.c",
    "src/x11_window.c",
    "src/xkb_unicode.c",
]

_LINUX_HDRS = [
    "src/linux_joystick.h",
    "src/posix_poll.h",
    "src/posix_thread.h",
    "src/posix_time.h",
    "src/x11_platform.h",
    "src/xkb_unicode.h",
]

_MACOS_SRCS = [
    "src/cocoa_init.m",
    "src/cocoa_joystick.m",
    "src/cocoa_monitor.m",
    "src/cocoa_window.m",
    "src/nsgl_context.m",
    "src/cocoa_time.c",
    "src/posix_module.c",
    "src/posix_thread.c",
]

_MACOS_HDRS = [
    "src/cocoa_platform.h",
    "src/cocoa_joystick.h",
    "src/cocoa_time.h",
    "src/posix_thread.h",
]

_COMMON_EXTERNAL_HDRS = glob(
    [
        "src/external/**/*.h",
        "src/external/**/*.hpp",
        "src/external/**/*.inl",
        "src/external/**/*.inc",
    ],
    allow_empty = True,
)

# Native (C/C++) build used on Windows and Linux
cc_library(
    name = "glfw2_native",
    srcs = _COMMON_SRCS + select({
        "@platforms//os:windows": _WINDOWS_SRCS,
        "@platforms//os:linux": _LINUX_SRCS,
        "//conditions:default": [],
    }),
    hdrs = _PUBLIC_HDRS + _COMMON_INTERNAL_HDRS + select({
        "@platforms//os:windows": _WINDOWS_HDRS,
        "@platforms//os:linux": _LINUX_HDRS,
        "//conditions:default": [],
    }) + _COMMON_EXTERNAL_HDRS,
    copts = select({
        "@platforms//os:windows": ["/DWIN32_LEAN_AND_MEAN"],
        "//conditions:default": [],
    }),
    defines = select({
        "@platforms//os:windows": ["_GLFW_WIN32"],
        "@platforms//os:linux": ["_GLFW_X11"],
        "//conditions:default": [],
    }),
    includes = ["src"],
    visibility = ["//visibility:private"],
)

# Objective-C build for macOS
# NOTE: objc_library requires Apple toolchain, use cc_library instead
cc_library(
    name = "glfw2_cocoa",
    srcs = _COMMON_SRCS + _MACOS_SRCS,
    hdrs = _PUBLIC_HDRS + _COMMON_INTERNAL_HDRS + _MACOS_HDRS + _COMMON_EXTERNAL_HDRS,
    copts = [
        "-fno-objc-arc",
        "-DGL_SILENCE_DEPRECATION",
    ],
    defines = ["_GLFW_COCOA"],
    includes = ["src"],
    target_compatible_with = ["@platforms//os:macos"],
    visibility = ["//visibility:private"],
)

# Public target that matches the style of third_party/raylib-latest.BUILD
cc_library(
    name = "glfw2",
    hdrs = _PUBLIC_HDRS,
    includes = ["include"],
    linkopts = select({
        "@platforms//os:windows": [
            "/DEFAULTLIB:user32.lib",
            "/DEFAULTLIB:gdi32.lib",
            "/DEFAULTLIB:shell32.lib",
            "/DEFAULTLIB:ole32.lib",
            "/DEFAULTLIB:advapi32.lib",
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
            "-framework",
            "Cocoa",
            "-framework",
            "OpenGL",
            "-framework",
            "IOKit",
            "-framework",
            "CoreVideo",
        ],
        "//conditions:default": [],
    }),
    strip_include_prefix = "include",
    deps = select({
        "@platforms//os:macos": [":glfw2_cocoa"],
        "@platforms//os:windows": [":glfw2_native"],
        "@platforms//os:linux": [":glfw2_native"],
        "//conditions:default": [],
    }),
)
