load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

COMMON_DEFINES = [
    "FCL_HAVE_OCTOMAP=1",
    "FCL_HAVE_EIGEN=1",
]

WIN_DEFINES = COMMON_DEFINES + [
    "FCL_STATIC_DEFINE",
    # "FCL_EXPORT=",  
    # "FCL_NO_EXPORT=",
]

COPTS = select({
    "@platforms//os:windows": [
        "/W3",
        "/DWIN32",
        "/DNDEBUG",
        # "/arch:SSE2",  # Enable SSE2 for Win32
    ],
    "//conditions:default": [
        "-w",
        "-FCL_STATIC_DEFINE",
        "-fvisibility=hidden",
        "-std=c++14",
        "-mfpmath=sse",
        "-msse",
        "-msse2",
        "-msse3",
        "-mssse3",
    ],
}) + select({
    "@platforms//cpu:x86_64": ["-DFCL_USE_X64_SSE"],
    "//conditions:default": [],
})

cc_library(
    name = "fcl",
    srcs = glob([
        "src/**/*.cpp",
    ]),
    hdrs = glob([
        "include/fcl/**/*.h",
        "include/fcl/**/*.hxx",
    ]) + [
        # "include/fcl/config.h",  # We'll provide this
        # "include/fcl/export.h",  # We'll provide this
    ],
    includes = [
        "include",
    ],
    deps = [
        "@libccd//:libccd",
        "@eigen",
        "@octomap//:octomap",
    ],
    copts = COPTS,
    linkstatic = True,
    defines = select({
        "@platforms//os:windows": WIN_DEFINES,
        "//conditions:default": COMMON_DEFINES,
    }),
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-lm"],
    }),
)