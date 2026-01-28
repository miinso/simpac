load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "ref_bench",
    srcs = [
        "bench_ref.cpp",
    ],
    deps = [
        "@flecs",
        "@google_benchmark//:benchmark",
    ],
)
