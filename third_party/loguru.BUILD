cc_library(
    name = "loguru",
    srcs = [
        "loguru.cpp",
    ],
    hdrs = [
        "loguru.hpp",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-fexceptions",  # Enable C++ exception handling
        ],
        "@platforms//os:windows": [
            "/std:c++17",
            "/wd5051",  # Disable specific Visual Studio warning (C5051)
        ],
        "//conditions:default": [
            "-std=c++11",  # Use C++11 standard for UNIX-like
        ],
    }),
    includes = ["."],
    linkopts = select({
        "@platforms//os:windows": [
        ],
        "//conditions:default": [
            "-lpthread",
            "-ldl",
        ],
    }),
    visibility = ["//visibility:public"],
)
