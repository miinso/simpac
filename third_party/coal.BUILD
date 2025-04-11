# coal is a no go.
# hard coded as shared lib 
# + rules_foreign_cc(bazel cmake dep) requires unix bash. powershell is not supported

# load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

# package(default_visibility = ["//visibility:public"])

# filegroup(
#     name = "all_srcs",
#     srcs = glob(["**"]),
# )

# cmake(
#     name = "coal",
#     build_args = [
#         "--",
#         "-j6",
#     ],
#     cache_entries = {
#         "CMAKE_BUILD_TYPE": "Release",
#         "BUILD_PYTHON_INTERFACE": "OFF",
#         "COAL_HAS_QHULL": "OFF",
#         "INSTALL_DOCUMENTATION": "OFF",
#         "CMAKE_CXX_STANDARD": "14",
#         "COAL_ENABLE_LOGGING": "OFF",
#         "COAL_BACKWARD_COMPATIBILITY_WITH_HPP_FCL": "OFF",
#     },
#     generate_args = ["-GNinja"],  # Optional: Use Ninja generator for faster builds
#     lib_source = ":all_srcs",
#     # out_shared_libs = ["libcoal.so"],  # Adjust if the library name is different
#     deps = [
#         # "@assimp",
#         # "@boost//:chrono",
#         # "@boost//:filesystem",
#         # "@boost//:serialization",
#         # "@eigen",
#     ],
# )

load("@rules_cc//cc:defs.bzl", "cc_library")

COAL_SRCS = [
    "src/collision.cpp",
    "src/contact_patch.cpp",
    "src/contact_patch/contact_patch_solver.cpp",
    "src/contact_patch_func_matrix.cpp",
    "src/distance_func_matrix.cpp",
    "src/collision_data.cpp",
    "src/collision_node.cpp",
    "src/collision_object.cpp",
    "src/BV/RSS.cpp",
    "src/BV/AABB.cpp",
    "src/BV/kIOS.cpp",
    "src/BV/kDOP.cpp",
    "src/BV/OBBRSS.cpp",
    "src/BV/OBB.cpp",
    "src/broadphase/default_broadphase_callbacks.cpp",
    "src/broadphase/broadphase_dynamic_AABB_tree.cpp",
    "src/broadphase/broadphase_dynamic_AABB_tree_array.cpp",
    "src/broadphase/broadphase_bruteforce.cpp",
    "src/broadphase/broadphase_collision_manager.cpp",
    "src/broadphase/broadphase_SaP.cpp",
    "src/broadphase/broadphase_SSaP.cpp",
    "src/broadphase/broadphase_interval_tree.cpp",
    "src/broadphase/detail/interval_tree.cpp",
    "src/broadphase/detail/interval_tree_node.cpp",
    "src/broadphase/detail/simple_interval.cpp",
    "src/broadphase/detail/spatial_hash.cpp",
    "src/broadphase/detail/morton.cpp",
    "src/narrowphase/gjk.cpp",
    "src/narrowphase/minkowski_difference.cpp",
    "src/narrowphase/support_functions.cpp",
    "src/shape/convex.cpp",
    "src/shape/geometric_shapes.cpp",
    "src/shape/geometric_shapes_utility.cpp",
] + glob([
    "src/distance/*.cpp",
    "src/math/*.cpp",
    "src/traversal/*.cpp",
    "src/BVH/*.cpp",
    "src/mesh_loader/*.cpp",
])

COAL_PUBLIC_HDRS = glob([
    "include/coal/**/*.h",
    "include/coal/**/*.hpp",
    "include/coal/**/*.hxx",
])

genrule(
    name = "generate_config",
    outs = [
        "include/coal/config.hh",
        "include/coal/deprecated.hh",
        "include/coal/warning.hh",
    ],
    cmd = """
        echo "#pragma once" > $(location include/coal/config.hh)
        echo "#pragma once" > $(location include/coal/deprecated.hh)
        echo "#pragma once" > $(location include/coal/warning.hh)
    """,
)

cc_library(
    name = "coal",
    srcs = COAL_SRCS,
    hdrs = COAL_PUBLIC_HDRS + [
        ":generate_config",
    ],
    includes = [
        "include",
    ],
    defines = select({
        "@platforms//os:windows": [
            "NOMINMAX",
            "_ENABLE_EXTENDED_ALIGNED_STORAGE",  # MSVC 2017 Eigen compatibility
        ],
        "//conditions:default": [],
    }),
    copts = select({
        "@platforms//os:windows": ["/bigobj"],
        "//conditions:default": [
            "-std=c++14",
            "-fPIC",
        ],
    }),
    deps = [
        "@eigen//:eigen",
        "@boost//:serialization",
        "@boost//:chrono",
        "@boost//:filesystem",
        "@assimp//:assimp",
    ] + select({
        "@platforms//os:windows": [
            "@boost//:thread",
            "@boost//:date_time",
        ],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)