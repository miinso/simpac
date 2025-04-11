load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

COPTS = select({
    "@platforms//os:windows": [
        "/W3",
        "/DCCD_STATIC_DEFINE",
        "/DCCD_SINGLE",  # default to single precision
        "/wd4005",
    ],
    "//conditions:default": [
        "-w",
        "-DCCD_STATIC_DEFINE",
        "-DCCD_SINGLE",  # default to single precision
        "-fvisibility=hidden",
    ],
})

cc_library(
    name = "libccd",
    srcs = [
        "src/alloc.h",
        "src/ccd.c",
        "src/ccd/config.h",
        "src/dbg.h",
        "src/list.h",
        "src/mpr.c",
        "src/polytope.c",
        "src/polytope.h",
        "src/simplex.h",
        "src/support.c",
        "src/support.h",
        "src/vec3.c",
    ],
    hdrs = [
        "src/ccd/ccd.h",
        "src/ccd/ccd_export.h",
        "src/ccd/compiler.h",
        "src/ccd/config.h",
        "src/ccd/quat.h",
        "src/ccd/vec3.h",
    ],
    copts = COPTS,
    defines = ["CCD_SINGLE"],
    includes = ["src"],
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-lm"],
    }),
)
