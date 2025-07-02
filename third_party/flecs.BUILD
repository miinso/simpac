load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "flecs",
    srcs = ["distr/flecs.c"],
    hdrs = ["distr/flecs.h"],
    copts = select({
        # the manual says to build it as c, not cpp
        "@platforms//os:windows": ["/TC"],
        "//conditions:default": [
            "-std=gnu99",
            "-x",
            "c",
        ],
    }),
    # defines = [
    #     "NDEBUG",
    # ],
    includes = ["distr"],
    linkopts = select({
        "@platforms//os:windows": ["/DEFAULTLIB:Ws2_32.lib"],  # winsocket
        "@platforms//os:emscripten": [
            # "-sALLOW_MEMORY_GROWTH=1",
            # "-sSTACK_SIZE=1mb",
            "-sEXPORTED_RUNTIME_METHODS=cwrap",  # TODO: what about ccall
            # "-sMODULARIZE=1",
        ],  # + @platforms//cpu:wasm32
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
