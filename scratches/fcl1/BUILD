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
            # ":util",
            "//:raylib",
            "@fcl",
            "@eigen",
            "@googletest//:gtest_main",
        ],
    )
    for source_file_name in source_file_name_list
]

# cc_library(
#     name = "util",
#     srcs = [
#         "module/test_fcl_utility.cpp",
#     ],
#     hdrs = [
#         "module/test_fcl_utility.h",
#     ],
#     copts = select({
#         # "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
#         "//conditions:default": ["-DPLATFORM_DESKTOP"],
#     }),
#     # data = ["@com_minseopark_simpac//:resources"],
#     strip_include_prefix = "module",
#     deps = [
#         "@fcl",
#     ],
# )
