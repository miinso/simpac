load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

# Platform specific compiler options
COPTS = select({
    "@platforms//os:windows": [
        "/w",
        "/DWIN32",
        "/DNDEBUG",
        "/openmp",  # MSVC OpenMP flag
    ],
    "//conditions:default": [
        "-w",
        "-fPIC",
        "-std=c++14",
        "-fopenmp",  # GCC/Clang OpenMP flag
    ],
})

# Platform specific linker options
LINKOPTS = select({
    "@platforms//os:windows": [],
    "@platforms//os:linux": [
        "-lgomp",  # Link with GNU OpenMP library on Linux
    ],
    "@platforms//os:osx": [
        "-lomp",  # Link with LLVM OpenMP library on macOS
    ],
    "//conditions:default": [],
})

# octomath library
cc_library(
    name = "octomath",
    srcs = [
        "octomap/src/math/Pose6D.cpp",
        "octomap/src/math/Quaternion.cpp",
        "octomap/src/math/Vector3.cpp",
    ],
    hdrs = glob([
        "octomap/include/octomap/math/*.h",
    ]),
    copts = COPTS,
    includes = ["octomap/include"],
    linkopts = LINKOPTS,
    linkstatic = True,
)

# Main octomap library
cc_library(
    name = "octomap",
    srcs = [
        "octomap/src/AbstractOcTree.cpp",
        "octomap/src/AbstractOccupancyOcTree.cpp",
        "octomap/src/ColorOcTree.cpp",
        "octomap/src/CountingOcTree.cpp",
        "octomap/src/OcTree.cpp",
        "octomap/src/OcTreeNode.cpp",
        "octomap/src/OcTreeStamped.cpp",
        "octomap/src/Pointcloud.cpp",
        "octomap/src/ScanGraph.cpp",
    ],
    hdrs = glob([
        "octomap/include/octomap/*.h",
        "octomap/include/octomap/*.hxx",
    ]),
    copts = COPTS,
    includes = ["octomap/include"],
    linkopts = LINKOPTS,
    linkstatic = True,
    deps = [":octomath"],
)

# # Optional: shared library versions if needed
# cc_library(
#     name = "octomath_shared",
#     srcs = [
#         "src/math/Vector3.cpp",
#         "src/math/Quaternion.cpp",
#         "src/math/Pose6D.cpp",
#     ],
#     hdrs = glob([
#         "include/octomap/math/*.h",
#     ]),
#     includes = ["include"],
#     copts = COPTS,
#     linkstatic = False,
# )

# cc_library(
#     name = "octomap_shared",
#     srcs = [
#         "src/AbstractOcTree.cpp",
#         "src/AbstractOccupancyOcTree.cpp",
#         "src/Pointcloud.cpp",
#         "src/ScanGraph.cpp",
#         "src/CountingOcTree.cpp",
#         "src/OcTree.cpp",
#         "src/OcTreeNode.cpp",
#         "src/OcTreeStamped.cpp",
#         "src/ColorOcTree.cpp",
#     ],
#     hdrs = glob([
#         "include/octomap/*.h",
#         "include/octomap/*.hxx",
#     ]),
#     includes = ["include"],
#     copts = COPTS,
#     deps = [":octomath_shared"],
#     linkstatic = False,
# )
