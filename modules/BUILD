# include_prefix

# String; default is ""
# The prefix to add to the paths of the headers of this rule.
# When set, the headers in the hdrs attribute of this rule are accessible at is the value of this attribute prepended to their repository-relative path.
# The prefix in the strip_include_prefix attribute is removed before this prefix is added.
# This attribute is only legal under third_party.

# strip_include_prefix

# String; default is ""
# The prefix to strip from the paths of the headers of this rule.
# When set, the headers in the hdrs attribute of this rule are accessible at their path with this prefix cut off.
# If it's a relative path, it's taken as a package-relative one. If it's an absolute one, it's understood as a repository-relative path.
# The prefix in the include_prefix attribute is added after this prefix is stripped.
# This attribute is only legal under third_party.

# https://groups.google.com/g/bazel-discuss/c/5DD7ZIBt3Ss

cc_library(
    name = "graphics_module",
    srcs = [
        "graphics_module.cpp",
        "include/graphics_module.h",
        "include/rlights.h",
    ],
    hdrs = [
        "include/graphics_module.h",
        "include/rlights.h",
    ],
    copts = select({
        "@platforms//os:emscripten": ["-DPLATFORM_WEB"],
        "//conditions:default": ["-DPLATFORM_DESKTOP"],
    }),
    data = ["@com_minseopark_simpac//:resources"],
    strip_include_prefix = "include",
    # includes = ["include"],
    # linkstatic = True,
    visibility = ["//visibility:public"],
    deps = [
        "//:raylib",
        "@eigen",
        "@flecs",
    ],
)

cc_library(
    name = "model_module",
    srcs = [
        "include/model_module.h",
        "model_module.cpp",
    ],
    hdrs = [
        "include/model_module.h",
    ],
    strip_include_prefix = "include",
    visibility = ["//visibility:public"],
    deps = [
        # "//:raylib",
        "@eigen",
        "@flecs",
    ],
)
