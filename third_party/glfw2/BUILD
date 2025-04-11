load("@rules_cc//cc:defs.bzl", "cc_library", "objc_library")

# ===== win32 =====

WIN32_DEFINES = [
    "_GLFW_WIN32",  # enabled win32 backend support
]

WIN32_HDRS = [
    "src/win32_joystick.h",
    "src/win32_platform.h",
    "src/win32_time.h",
    "src/win32_thread.h",
]

WIN32_SRCS = [
    "src/win32_init.c",
    "src/win32_joystick.c",
    "src/win32_module.c",
    "src/win32_monitor.c",
    "src/win32_time.c",
    "src/win32_thread.c",
    "src/win32_window.c",
    "src/wgl_context.c",
]

WIN32_LINKOPTS = [
    "-DEFAULTLIB:user32.lib",
    "-DEFAULTLIB:gdi32.lib", 
    "-DEFAULTLIB:shell32.lib",
]

# ===== linux =====

LINUX_DEFINES = [
    "_GLFW_X11",  # enabled x11 backend support
    # "_GLFW_WAYLAND",  # disabled wayland backend support for now
]

LINUX_HDRS = [
    # "src/glx_context.h",
    "src/linux_joystick.h",
    "src/posix_thread.h",
    "src/posix_time.h",
    "src/x11_platform.h",
    "src/posix_poll.h",
    "src/xkb_unicode.h",
]

LINUX_SRCS = [
    "src/glx_context.c",
    "src/linux_joystick.c",
    "src/posix_module.c",
    "src/posix_thread.c",
    "src/posix_time.c",
    "src/posix_poll.c",
    "src/x11_init.c",
    "src/x11_monitor.c", 
    "src/x11_window.c",
    "src/xkb_unicode.c",
]

LINUX_LINKOPTS = []

# ===== darwin =====

DARWIN_DEFINES = [
    "_GLFW_COCOA",  # enabled cocoa backend support
]

DARWIN_HDRS = [
    "src/cocoa_joystick.h",
    "src/cocoa_platform.h",
    "src/cocoa_time.h",
    "src/posix_thread.h",
]

DARWIN_SRCS = [
    "src/cocoa_init.m",
    "src/cocoa_joystick.m",
    "src/cocoa_monitor.m", 
    "src/cocoa_time.c",
    "src/cocoa_window.m",
    "src/nsgl_context.m",
    "src/posix_module.c",
    "src/posix_thread.c",
]

DARWIN_LINKOPTS = [
    "-framework OpenGL",
    "-framework Cocoa",
    "-framework IOKit",
    "-framework CoreFoundation",
]

# ===== common =====

COMMON_HDRS = [
    "include/GLFW/glfw3.h",
    "include/GLFW/glfw3native.h",
    "src/internal.h",
    "src/mappings.h",
    "src/platform.h",
    "src/null_platform.h",
    "src/null_joystick.h",
]

COMMON_SRCS = [
    "src/context.c",
    "src/egl_context.c",
    "src/init.c",
    "src/input.c", 
    "src/monitor.c",
    "src/platform.c",
    "src/vulkan.c",
    "src/window.c",
    "src/osmesa_context.c",
    # null platform is always available in 3.4
    "src/null_init.c",
    "src/null_monitor.c",
    "src/null_window.c",
    "src/null_joystick.c",
]

cc_library(
    name = "glfw_src",
    srcs = COMMON_SRCS + select({
        "@bazel_tools//src/conditions:windows": WIN32_SRCS,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_SRCS,
    }),
    hdrs = COMMON_HDRS + select({
        "@bazel_tools//src/conditions:windows": WIN32_HDRS,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_HDRS,
    }),
    defines = select({
        "@bazel_tools//src/conditions:windows": WIN32_DEFINES,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_DEFINES,
    }),
    includes = ["src"],
    copts = select({
        "@bazel_tools//src/conditions:windows": ["/DWIN32_LEAN_AND_MEAN"],
        "//conditions:default": [],
    }),
)

objc_library(
    name = "glfw_src_darwin",
    srcs = COMMON_SRCS + DARWIN_SRCS,
    hdrs = COMMON_HDRS + DARWIN_HDRS,
    defines = DARWIN_DEFINES,
    includes = ["src"],
    copts = [
        "-fno-objc-arc",
        "-DGL_SILENCE_DEPRECATION",
    ],
)

cc_library(
    name = "glfw2",
    hdrs = [
        "include/GLFW/glfw3.h",
        "include/GLFW/glfw3native.h",
    ],
    linkopts = select({
        "@bazel_tools//src/conditions:windows": WIN32_LINKOPTS,
        "@bazel_tools//src/conditions:linux_x86_64": LINUX_LINKOPTS,
        "@bazel_tools//src/conditions:darwin": DARWIN_LINKOPTS,
    }),
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = select({
        "@bazel_tools//src/conditions:windows": [":glfw_src"],
        "@bazel_tools//src/conditions:linux_x86_64": [":glfw_src"],
        "@bazel_tools//src/conditions:darwin": [":glfw_src_darwin"],
    }),
)