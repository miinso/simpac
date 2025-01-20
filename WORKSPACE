workspace(name = "com_minseopark_simpac")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

##
#  flecs
##
http_archive(
    name = "flecs",
    build_file = "//third_party:flecs.BUILD",
    strip_prefix = "flecs-4.0.4",
    urls = ["https://github.com/SanderMertens/flecs/archive/refs/tags/v4.0.4.tar.gz"],
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
    strip_prefix = "raylib-5.5_macos",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_macos.tar.gz"],
)

http_archive(
    name = "raylib_windows",
    build_file = "//third_party:raylib.BUILD",
    strip_prefix = "raylib-5.5_win64_msvc16",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_msvc16.zip"],
)

http_archive(
    name = "raylib_linux",
    build_file = "//third_party:raylib.BUILD",
    strip_prefix = "raylib-5.5_linux_amd64",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz"],
)

http_archive(
    name = "raylib_emscripten",
    build_file = "//third_party:raylib.BUILD",
    strip_prefix = "raylib-5.5_webassembly",
    urls = ["https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_webassembly.zip"],
)

##
#  raylib-cpp
##
git_repository(
    name = "raylib-cpp",
    build_file = "//third_party:raylib-cpp.BUILD",
    commit = "c283b731b0c1bd423db45546867af969d91b49a1",  # Use the specific commit or tag
    remote = "https://github.com/RobLoach/raylib-cpp.git",
)

##
#  discregrid
##
git_repository(
    name = "Discregrid",
    build_file = "//third_party:Discregrid.BUILD",
    commit = "4c27e1cc88be828c6ac5b8a05759ac7e01cf79e9",
    remote = "https://github.com/InteractiveComputerGraphics/Discregrid.git",
)

##
#  loguru
##
git_repository(
    name = "loguru",
    build_file = "//third_party:loguru.BUILD",
    commit = "4adaa185883e3c04da25913579c451d3c32cfac1",
    remote = "https://github.com/emilk/loguru.git",
)
