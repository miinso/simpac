cc_library(
    name = "flecs",
    srcs = ["flecs.c"],
    hdrs = ["flecs.h"],
    copts = select({
        # the manual says to build it as c, not cpp
        "@platforms//os:windows": ["/TC"],
        "//conditions:default": [
            "-std=gnu99",
            "-x",
            "c",
        ],
    }),
    includes = ["."],
    linkopts = select({
        "@platforms//os:windows": ["/DEFAULTLIB:Ws2_32.lib"],  # winsocket
        "@platforms//os:emscripten": [
            # "-sALLOW_MEMORY_GROWTH=1",
            # "-sSTACK_SIZE=1mb",
            "-sEXPORTED_RUNTIME_METHODS=cwrap ",
            # "-sMODULARIZE=1",
            # "-sEXPORT_NAME=\"my_app\"",
        ],  # + @platforms//cpu:wasm32
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
