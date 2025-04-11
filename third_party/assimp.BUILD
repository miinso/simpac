load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

# you might want to check:
# https://bazel-contrib.github.io/rules_foreign_cc/cmake.html

cmake(
    name = "assimp",
    cache_entries = {
        # always release build
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_CXX_FLAGS_RELEASE": "-O3 -DNDEBUG -fno-exceptions -fno-rtti",

        # defaults
        "BUILD_SHARED_LIBS": "OFF",
        "ASSIMP_BUILD_TESTS": "OFF",
        "ASSIMP_BUILD_ASSIMP_TOOLS": "OFF",
        "ASSIMP_BUILD_ZLIB": "ON",

        # min build
        "ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT": "OFF",
        "ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT": "OFF",
        "ASSIMP_NO_EXPORT": "ON",

        # nono
        "ASSIMP_BUILD_DOCS": "OFF",
        "ASSIMP_BUILD_SAMPLES": "OFF",
        "ASSIMP_BUILD_PYASSIMP": "OFF",
        "ASSIMP_BUILD_ASSIMP_VIEW": "OFF",
        "ASSIMP_BUILD_DEBUGGER": "OFF",
        "ASSIMP_INSTALL": "OFF",

        # no debug
        "ASSIMP_BUILD_DEBUG_LOGGING": "OFF",
        "CMAKE_INSTALL_DEBUG_LIBRARIES": "OFF",
        "ASSIMP_BUILD_OBJ_IMPORTER": "ON",
    },
    lib_source = ":all_srcs",
    # out_static_libs = ["libassimp.a"],
    out_static_libs = [
        "assimp-vc142-mt.lib",
        "zlibstatic.lib",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)
