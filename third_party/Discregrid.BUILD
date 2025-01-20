# BUILD file
cc_library(
    name = "Discregrid",
    srcs = [
        "discregrid/src/cubic_lagrange_discrete_grid.cpp",
        "discregrid/src/discrete_grid.cpp",
        "discregrid/src/mesh/entity_containers.cpp",
        "discregrid/src/mesh/entity_iterators.cpp",
        "discregrid/src/mesh/triangle_mesh.cpp",
        "discregrid/src/utility/timing.cpp",
    ],
    hdrs = [
        "discregrid/include/Discregrid/All",  # this `All` breaks things
        "discregrid/include/Discregrid/cubic_lagrange_discrete_grid.hpp",
        "discregrid/include/Discregrid/discrete_grid.hpp",
        "discregrid/include/Discregrid/geometry/TriangleMeshDistance.h",
        "discregrid/include/Discregrid/mesh/entity_containers.hpp",
        "discregrid/include/Discregrid/mesh/entity_iterators.hpp",
        "discregrid/include/Discregrid/mesh/halfedge.hpp",
        "discregrid/include/Discregrid/mesh/triangle_mesh.hpp",
        "discregrid/include/Discregrid/utility/lru_cache.hpp",
        "discregrid/include/Discregrid/utility/serialize.hpp",
        "discregrid/src/data/z_sort_table.hpp",
        "discregrid/src/utility/spinlock.hpp",
        "discregrid/src/utility/timing.hpp",
        "extern/cxxopts/cxxopts.hpp",
    ],
    copts = select({
        "@platforms//os:windows": [
            "/MP",
            "/openmp",  # TODO: use tbb
            "/wd4250",
            "-D_SCL_SECURE_NO_WARNINGS",
            "-D_CRT_SECURE_NO_WARNINGS",
            "-DNOMINMAX",
            "-D_USE_MATH_DEFINES",
        ],
        "@platforms//os:emscripten": [],
        "//conditions:default": [
            "-std=c++11",
        ],
    }),
    includes = ["discregrid/include/Discregrid"],
    linkopts = select({
        "@platforms//os:windows": [],
        "//conditions:default": [
            # "-lpthread",
            # "-ldl",
        ],
    }),
    visibility = ["//visibility:public"],
    deps = ["@eigen"],
)

cc_binary(
    name = "cmd",
    srcs = glob(["cmd/**/*.cpp"]),
    deps = [":Discregrid"],
)

# config_setting(
#     name = "windows",
#     values = {"cpu": "x64_windows"},
#     visibility = ["//visibility:public"],
# )
