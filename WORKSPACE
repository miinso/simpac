workspace(name = "com_minseopark_simpac")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

##
#  flecs
##
http_archive(
    name = "flecs",
    build_file = "//third_party:flecs.BUILD",
    # integrity = "sha256-2IkoIms6bn68fIGNtQsvtYKAIe07zSBsTio7BAZHLSs=",
    strip_prefix = "flecs-4.0.2",
    urls = ["https://github.com/SanderMertens/flecs/archive/refs/tags/v4.0.2.tar.gz"],
)

git_repository(
    name = "emsdk",
    remote = "https://github.com/emscripten-core/emsdk.git",
    strip_prefix = "bazel",
    tag = "3.1.64",
)

##
#  emcc
##
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

##
#  raylib
##
http_archive(
    name = "raylib_macos",
    build_file = "//third_party:raylib.BUILD",
    strip_prefix = "raylib-5.0_macos",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_macos.tar.gz"],
)

http_archive(
    name = "raylib_windows",
    build_file = "//third_party:raylib.BUILD",
    strip_prefix = "raylib-5.0_win64_msvc16",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_msvc16.zip"],
)

http_archive(
    name = "raylib_linux",
    build_file = "//third_party:raylib.BUILD",
    strip_prefix = "raylib-5.0_linux_amd64",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_linux_amd64.tar.gz"],
)

http_archive(
    name = "raylib_emscripten",
    build_file = "//third_party:raylib.BUILD",
    strip_prefix = "raylib-5.0_webassembly",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_webassembly.zip"],
)
