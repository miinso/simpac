load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "tmxlite",
    srcs = [
        "tmxlite/src/FreeFuncs.cpp",
        "tmxlite/src/ImageLayer.cpp",
        "tmxlite/src/Map.cpp",
        "tmxlite/src/Object.cpp",
        "tmxlite/src/ObjectGroup.cpp",
        "tmxlite/src/Property.cpp",
        "tmxlite/src/TileLayer.cpp",
        "tmxlite/src/LayerGroup.cpp",
        "tmxlite/src/Tileset.cpp",
        "tmxlite/src/ObjectTypes.cpp",
        # you can use internals if you want
        # "tmxlite/src/miniz.c",
        # "tmxlite/src/detail/pugixml.cpp",
    ],
    hdrs = glob([
        "tmxlite/include/tmxlite/*.hpp",
        "tmxlite/include/tmxlite/*.inl",
        "tmxlite/include/tmxlite/detail/*.hpp",
    ]),
    copts = [
        "-std=c++14",
        "-Wall",
    ] + select({
        "@platforms//os:windows": [
            "/D_CRT_SECURE_NO_WARNINGS",
        ],
        "//conditions:default": [],
    }),
    defines = [
        "TMXLITE_STATIC",
        "USE_EXTLIBS",  # zlib, zstd, pugixml
    ],
    includes = [
        "tmxlite/include",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@pugixml",
        "@zlib",
        "@zstd",
    ],
)
