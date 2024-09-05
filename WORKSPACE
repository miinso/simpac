workspace(name = "com_minseopark_simpac")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

#-------------------------------------------------------------
# cglm
#-------------------------------------------------------------

# http_archive(
#     name = "cglm",
#     build_file = "//:cglm.BUILD",
#     # integrity = "sha256-uu+iE0LSKKg8kHCEWadF1aqbDrs4FVXupC2x83/felo=",
#     strip_prefix = "cglm-0.9.2",
#     urls = ["https://github.com/recp/cglm/archive/v0.9.2.tar.gz"],
#     #     build_file_content = """
#     # cc_library(
#     #     name = "cglm",
#     #     hdrs = glob(["include/cglm/**/*.h"]),
#     #     includes = ["include"],
#     #     visibility = ["//visibility:public"],
#     # )
#     #     """,
# )

#-------------------------------------------------------------
# flecs
#-------------------------------------------------------------

http_archive(
    name = "flecs",
    build_file = "//:flecs.BUILD",
    integrity = "sha256-2IkoIms6bn68fIGNtQsvtYKAIe07zSBsTio7BAZHLSs=",
    strip_prefix = "flecs-4.0.1",
    urls = ["https://github.com/SanderMertens/flecs/archive/refs/tags/v4.0.1.tar.gz"],
)

#-------------------------------------------------------------
# GLFW
#-------------------------------------------------------------

# GLFW_VERSION = "3.3.8"

# http_archive(
#     name = "glfw",
#     build_file = "//:glfw.BUILD",
#     sha256 = "f30f42e05f11e5fc62483e513b0488d5bceeab7d9c5da0ffe2252ad81816c713",
#     strip_prefix = "glfw-{}".format(GLFW_VERSION),
#     urls = ["https://github.com/glfw/glfw/archive/{}.tar.gz".format(GLFW_VERSION)],
# )

# new_local_repository(
#     name = "system_libs",
#     build_file_content = """cc_library(
#     name = "x11",
#     srcs = ["libX11.so"],
#     visibility = ["//visibility:public"],
# )
# """,
#     # pkg-config --variable=libdir x11
#     path = "/usr/lib/x86_64-linux-gnu",
# )

#-------------------------------------------------------------
# Emscripten
#-------------------------------------------------------------

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
git_repository(
    name = "emsdk",
    remote = "https://github.com/emscripten-core/emsdk.git",
    tag = "3.1.64",
    strip_prefix = "bazel",
)

load("@emsdk//:deps.bzl", emsdk_deps = "deps")
emsdk_deps()

load("@emsdk//:emscripten_deps.bzl", emsdk_emscripten_deps = "emscripten_deps")
emsdk_emscripten_deps(emscripten_version = "3.1.64")

load("@emsdk//:toolchains.bzl", "register_emscripten_toolchains")
register_emscripten_toolchains()
# register_emscripten_toolchains(cache = {
#     "configuration": ["--lto"],
#     "targets": [
#         "crtbegin",
#         "libprintf_long_double-debug",
#         "libstubs-debug",
#         "libnoexit",
#         "libc-debug",
#         "libdlmalloc",
#         "libcompiler_rt",
#         "libc++-noexcept",
#         "libc++abi-debug-noexcept",
#         "libsockets"
#     ]
# })

#-------------------------------------------------------------
# raylib
#-------------------------------------------------------------

http_archive(
    name = "raylib_macos",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_macos.tar.gz"],
    strip_prefix = "raylib-5.0_macos",
    build_file = "//third_party:raylib.BUILD",
)

http_archive(
    name = "raylib_windows",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_msvc16.zip"],
    strip_prefix = "raylib-5.0_win64_msvc16",
    build_file = "//third_party:raylib.BUILD",
)

http_archive(
    name = "raylib_linux",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_linux_amd64.tar.gz"],
    strip_prefix = "raylib-5.0_linux_amd64",
    build_file = "//third_party:raylib.BUILD",
)

http_archive(
    name = "raylib_emscripten",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_webassembly.zip"],
    strip_prefix = "raylib-5.0_webassembly",
    build_file = "//third_party:raylib.BUILD",
)
