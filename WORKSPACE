load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

#-------------------------------------------------------------
# cglm
#-------------------------------------------------------------

http_archive(
    name = "cglm",
    build_file = "//:cglm.BUILD",
    # integrity = "sha256-uu+iE0LSKKg8kHCEWadF1aqbDrs4FVXupC2x83/felo=",
    strip_prefix = "cglm-0.9.2",
    urls = ["https://github.com/recp/cglm/archive/v0.9.2.tar.gz"],
    #     build_file_content = """
    # cc_library(
    #     name = "cglm",
    #     hdrs = glob(["include/cglm/**/*.h"]),
    #     includes = ["include"],
    #     visibility = ["//visibility:public"],
    # )
    #     """,
)

#-------------------------------------------------------------
# flecs
#-------------------------------------------------------------

http_archive(
    name = "flecs",
    build_file = "//:flecs.BUILD",
    integrity = "sha256-jrxfbz7Hu7owsK/p0i8VdDeSV3KFfqHG5CAetdMbT+U=",
    strip_prefix = "flecs-3.2.11",
    urls = ["https://github.com/SanderMertens/flecs/archive/refs/tags/v3.2.11.tar.gz"],
)

# local_repository(
#     name = "hedron_compile_commands",
#     path = "../bazel-compile-commands-extractor", # Assuming this tool is cloned next to your project's repo
# )

#-------------------------------------------------------------
# GLFW
#-------------------------------------------------------------

GLFW_VERSION = "3.3.8"

http_archive(
    name = "glfw",
    build_file = "//:glfw.BUILD",
    sha256 = "f30f42e05f11e5fc62483e513b0488d5bceeab7d9c5da0ffe2252ad81816c713",
    strip_prefix = "glfw-{}".format(GLFW_VERSION),
    urls = ["https://github.com/glfw/glfw/archive/{}.tar.gz".format(GLFW_VERSION)],
)

new_local_repository(
    name = "system_libs",
    build_file_content = """cc_library(
    name = "x11",
    srcs = ["libX11.so"],
    visibility = ["//visibility:public"],
)
""",
    # pkg-config --variable=libdir x11
    path = "/usr/lib/x86_64-linux-gnu",
)