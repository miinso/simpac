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
        data = [
            "@com_minseopark_simpac//:resources",
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
            "//modules:graphics",
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
        "kernels.hpp",
        "types.hpp",
    ],
    includes = [
        "kernels.hpp",
        "types.hpp",
    ],
    deps = [
        "@eigen",
        "@flecs",
    ],
)
