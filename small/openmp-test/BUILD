load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:defs.bzl", "cc_binary")

wasm_copts = [
    # "-O2",
    "-pthread",
    "-fopenmp",
    # "-fopenmp=libomp",
]

wasm_linkopts_common = [
    # threading support
    "-sALLOW_MEMORY_GROWTH=0",
    "-sEXIT_RUNTIME=0",
    "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
    "-sUSE_PTHREADS=1",
    "-pthread",
    "-fopenmp",
    # "-fopenmp=libomp",
]

###
# deps (https://github.com/MuTsunTsai/simpleomp)
###
cc_import(
    name = "simpleomp",
    hdrs = ["simpleomp.h"],
    static_library = "libsimpleomp.a",
    target_compatible_with = ["@platforms//os:emscripten"],
    visibility = ["//visibility:public"],
)

###
# targets
###
cc_binary(
    name = "race",
    srcs = [
        "race.cpp",
    ],
    copts = select({
        "@platforms//os:emscripten": wasm_copts,
        "@platforms//os:windows": [
            "/openmp",  # msvc
        ],
        "//conditions:default": ["-fopenmp"],
    }),
    linkopts = select({
        "@platforms//os:emscripten": wasm_linkopts_common + [
            "-sINITIAL_MEMORY=128mb",
        ],
        "//conditions:default": ["-fopenmp"],
    }),
    deps = select({
        "@platforms//os:emscripten": [":simpleomp"],
        "//conditions:default": [],
    }) + ["@onetbb//:tbb"],
)

wasm_cc_binary(
    name = "race_wasm",
    cc_target = ":race",
    tags = ["wasm"],
    threads = "emscripten",
)

cc_binary(
    name = "bench",
    srcs = ["bench.cpp"],
    copts = select({
        "@platforms//os:emscripten": wasm_copts,
        "@platforms//os:windows": [
            "/openmp",
            # "/openmp:experimental",
            # "/arch:AVX2",
        ],
        "//conditions:default": ["-fopenmp"],
    }),
    linkopts = select({
        "@platforms//os:emscripten": wasm_linkopts_common + [
            "-sINITIAL_MEMORY=256mb",  # raise this if you dare
            # "-sINITIAL_HEAP=4mb",
            # "-sSTACK_SIZE=4mb",
        ],
        "@platforms//os:windows": [],
        "//conditions:default": ["-fopenmp"],
    }),
    deps = select({
        "@platforms//os:emscripten": [":simpleomp"],
        "//conditions:default": [],
    }) + [
        "@google_benchmark//:benchmark",
        "@onetbb//:tbb",
    ],
)

wasm_cc_binary(
    name = "bench_wasm",
    cc_target = ":bench",
    tags = ["wasm"],
    threads = "emscripten",
)
