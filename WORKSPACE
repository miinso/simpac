workspace(name = "com_minseopark_simpac")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

##
#  flecs
##
http_archive(
    name = "flecs",
    build_file = "//third_party:flecs.BUILD",
    strip_prefix = "flecs-4.0.5",
    urls = ["https://github.com/SanderMertens/flecs/archive/refs/tags/v4.0.5.tar.gz"],
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
# "configuration": ["--lto"],
# "targets": [
#     "crtbegin",
#     "libprintf_long_double-debug",
#     "libstubs-debug",
#     "libnoexit",
#     "libc-debug",
#     "libdlmalloc",
#     "libcompiler_rt",
#     "libc++-noexcept",
#     "libc++abi-debug-noexcept",
#     "libsockets"
# ]
# })

##
# GLFW2
##
GLFW2_VERSION = "3.4"

http_archive(
    name = "glfw2",
    build_file = "//third_party/glfw2:BUILD",
    strip_prefix = "glfw-{}".format(GLFW2_VERSION),
    urls = ["https://github.com/glfw/glfw/archive/{}.tar.gz".format(GLFW2_VERSION)],
)

##
#  raylib (prebuilt)
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
# raylib (head)
##
git_repository(
    name = "raylib-head",
    # branch = "master",
    build_file = "//third_party:raylib-head.BUILD",
    commit = "cd9206956caa774c3ed9df0957b6b3167e5000c7",
    # depth = 1,
    remote = "https://github.com/raysan5/raylib.git",
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

##
# assimp
##
http_archive(
    name = "assimp",
    build_file = "//third_party:assimp.BUILD",
    strip_prefix = "assimp-5.3.1",
    urls = ["https://github.com/assimp/assimp/archive/refs/tags/v5.3.1.tar.gz"],
)

##
#  coal
##
# http_archive(
#     name = "coal",
#     build_file = "//third_party:coal.BUILD",
#     strip_prefix = "coal-3.0.0",
#     urls = ["https://github.com/coal-library/coal/archive/refs/tags/v3.0.0.tar.gz"],
# )

##
#  libccd
##
http_archive(
    name = "libccd",
    build_file = "//third_party:libccd.BUILD",
    patch_args = ["-p1"],
    patches = ["//third_party/patches:libccd_config.patch"],
    strip_prefix = "libccd-2.1",
    urls = ["https://github.com/danfis/libccd/archive/refs/tags/v2.1.tar.gz"],
)

##
#  octomap
##
http_archive(
    name = "octomap",
    build_file = "//third_party:octomap.BUILD",
    strip_prefix = "octomap-1.10.0",
    urls = ["https://github.com/OctoMap/octomap/archive/refs/tags/v1.10.0.tar.gz"],
)

##
#  fcl
##
http_archive(
    name = "fcl",
    build_file = "//third_party:fcl.BUILD",
    patch_args = ["-p1"],
    patches = ["//third_party/patches:fcl_config.patch"],
    strip_prefix = "fcl-0.7.0",
    urls = ["https://github.com/flexible-collision-library/fcl/archive/refs/tags/0.7.0.tar.gz"],
)
