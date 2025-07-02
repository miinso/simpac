workspace(name = "com_minseopark_simpac")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

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

# ##
# #  raylib-cpp
# ##
# git_repository(
#     name = "raylib-cpp",
#     build_file = "//third_party:raylib-cpp.BUILD",
#     commit = "c283b731b0c1bd423db45546867af969d91b49a1",  # Use the specific commit or tag
#     remote = "https://github.com/RobLoach/raylib-cpp.git",
# )

##
#  coal
##
# http_archive(
#     name = "coal",
#     build_file = "//third_party:coal.BUILD",
#     strip_prefix = "coal-3.0.0",
#     urls = ["https://github.com/coal-library/coal/archive/refs/tags/v3.0.0.tar.gz"],
# )
