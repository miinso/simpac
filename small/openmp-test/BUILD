load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:defs.bzl", "cc_binary")


###
# deps
###
###### https://github.com/MuTsunTsai/simpleomp
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
    name = "main",
    srcs = [
        "main.cpp",
    ],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
            "-pthread",
            "-fopenmp",
            # "-fopenmp=libomp",
        ],
        "@platforms//os:windows": [
            "/openmp",  # msvc
        ],
        "//conditions:default": ["-fopenmp"],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            # thread support
            "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
            "-sUSE_PTHREADS=1",
            "-pthread",
            "-fopenmp",
            # "-fopenmp=libomp",
        ],
        "//conditions:default": ["-fopenmp"],
    }),
    deps = select({
        "@platforms//os:emscripten": [":simpleomp"],
        "//conditions:default": [],
    }),
)

wasm_cc_binary(
    name = "main_wasm",
    cc_target = ":main",
    tags = ["wasm"],
    threads = "emscripten",
)

cc_binary(
    name = "bench",
    srcs = ["bench.cpp"],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
            "-pthread",
            "-fopenmp",
        ],
        "@platforms//os:windows": ["/openmp"],
        "//conditions:default": ["-fopenmp"],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
            "-sUSE_PTHREADS=1",
            "-pthread",
            "-fopenmp",
        ],
        "//conditions:default": ["-fopenmp"],
    }),
    deps = select({
        "@platforms//os:emscripten": [":simpleomp"],
        "//conditions:default": [],
    }) + ["@google_benchmark//:benchmark"],
)

wasm_cc_binary(
    name = "bench_wasm",
    cc_target = ":bench",
    tags = ["wasm"],
    threads = "emscripten",
)
