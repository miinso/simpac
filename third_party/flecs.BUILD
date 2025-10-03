load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "flecs",
    srcs = ["distr/flecs.c"],
    hdrs = ["distr/flecs.h"],
    copts = select({
        "@platforms//os:windows": [
            "/TC",  # treat as C
            # "/O2",      # optimize for speed
        ],
        "@platforms//os:emscripten": [
            "-std=gnu99",
            "-x",
            "c",
            # "-O3",
            "-fno-strict-aliasing",
            "-pthread",
        ],
        "//conditions:default": [
            "-std=gnu99",
            "-x",
            "c",
            # "-O3",
            "-fno-strict-aliasing",
            "-pthread",
        ],
    }),
    defines = ["NDEBUG"],
    includes = ["distr"],
    linkopts = select({
        "@platforms//os:windows": [
            "/DEFAULTLIB:Ws2_32.lib",
        ],
        "@platforms//os:emscripten": [
            "-sALLOW_MEMORY_GROWTH=1",
            "-sEXPORTED_RUNTIME_METHODS=cwrap",
            "-sUSE_PTHREADS=1",
            "-pthread",
        ],
        "//conditions:default": [
            "-pthread",
        ],
    }),
    visibility = ["//visibility:public"],
)
