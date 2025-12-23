load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob([
    
    "*.cpp",
], allow_empty = True)

[
    cc_binary(
        name = source_file_name.replace(".cpp", ""),
        srcs = [source_file_name],
        copts = [
            "-DPLATFORM_DESKTOP",
        ],
        data = [
            "//:resources",
        ],
        includes = [
            "components.hpp",
            "xpbd1.hpp",  # mk.1
        ],
        deps = [
            "//modules:graphics",
            "@eigen",
            "@flecs",
        ],
    )
    for source_file_name in source_file_name_list
]
