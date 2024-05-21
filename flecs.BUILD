cc_library(
    name = "flecs",
    srcs = ["flecs.c"],
    hdrs = ["flecs.h"],
    includes = ["."],
    copts = select({
        # the manual says to build it as c, not cpp
        "@platforms//os:windows": ["/TC"],
        "//conditions:default": ["-std=gnu99", "-x", "c"],
    }),
    linkopts = select({
        "@platforms//os:windows": ["/DEFAULTLIB:Ws2_32.lib"], # winsocket
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)
