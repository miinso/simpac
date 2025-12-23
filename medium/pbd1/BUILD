load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob([
    "*.cpp",
], allow_empty = True)

[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name],
        copts = select({
            "@platforms//os:windows": ["-DPLATFORM_DESKTOP"],
            "//conditions:default": ["-DPLATFORM_DESKTOP"],
        }),
        data = [
            "//:resources",
        ],
        includes = [
            # "types.hpp",
            # "kernels.hpp",
            # "pbd1.hpp", # mk.1
            # "components.hpp",
            # "rigid_body.hpp",
            # "rigid_body2.hpp",
            # "geometry.hpp",
            # "solve.hpp",
        ],
        deps = [
            "//modules:graphics2",
            "@raylib",
            "@flecs",
            "@eigen",
            # "@Discregrid",
            # modules
            # "//modules:pbd",
            ":pbd",
        ],
    )
    for source_file_name in source_file_name_list
]

cc_library(
    name = "pbd",
    hdrs = [
        "components.hpp",
        "kernels.hpp",
        "pbd1.hpp",
        "types.hpp",
    ],
    includes = [
        ".",
    ],
    deps = [
        "@eigen",
        "@flecs",
    ],
)
