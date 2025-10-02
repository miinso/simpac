load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob([
    "*.c",
    "*.cpp",
])

[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name],
        copts = [
            "-DPLATFORM_DESKTOP",
        ],
        data = ["@com_minseopark_simpac//:resources"],
        includes = [
            "components.hpp",
            # "graphics.h",
        ],
        deps = [
            ":graphics",
            "@raylib",
            "@assimp//:assimp",
            "@eigen",
            "@flecs",
        ],
    )
    for source_file_name in source_file_name_list
]

cc_library(
    name = "graphics",
    srcs = [
        "module/graphics.cpp",
    ],
    hdrs = [
        "module/graphics.h",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    # data = ["@com_minseopark_simpac//:resources"],
    strip_include_prefix = "module",
    deps = [
        "@raylib",
        "@eigen",
        "@flecs",
    ],
)
