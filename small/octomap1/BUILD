load("@rules_cc//cc:defs.bzl", "cc_binary")

source_file_name_list = glob(["*.cpp"], allow_empty = True)

[
    cc_binary(
        name = source_file_name.replace(".cpp", "").replace(".c", ""),
        srcs = [source_file_name],
        copts = [
        ],
        data = ["//:resources"],
        includes = [
        ],
        deps = [
            "@octomap",
        ],
    )
    for source_file_name in source_file_name_list
]
