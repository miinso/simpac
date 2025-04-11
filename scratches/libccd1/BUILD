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
        ],
        data = ["@com_minseopark_simpac//:resources"],
        includes = [
        ],
        deps = [
            "@libccd",
        ],
    )
    for source_file_name in source_file_name_list
]
