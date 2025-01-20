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
            "components.hpp",
            "rigid_body.hpp",
            "rigid_body2.hpp",
            "geometry.hpp",
            "collision.hpp",
            "solve.hpp",
        ],
        deps = [
            "//:raylib",
            "@flecs",
            "@eigen",
            "@Discregrid",
            # modules
            "//modules:pbd",
            "//modules:graphics",
        ],
    )
    for source_file_name in source_file_name_list
]
