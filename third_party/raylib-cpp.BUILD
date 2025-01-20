cc_library(
    name = "raylib-cpp",
    hdrs = glob(["include/*.hpp"]),
    copts = select({
        "@platforms//os:windows": [
            "/wd5051",  # ignore maybe_unused attribute warning"
        ],
        "//conditions:default": [],
    }),
    includes = ["include"],
    visibility = ["//visibility:public"],
)
