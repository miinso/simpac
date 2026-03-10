load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_cuda//cuda:defs.bzl", "cuda_binary", "cuda_library")

cc_library(
    name = "utils",
    hdrs = ["timer.h", "gen.h"],
    deps = ["@eigen"],
    visibility = ["//visibility:public"],
)

##
# coo
##
cuda_library(
    name = "coo_kernel",
    srcs = ["coo/kernel.cu"],
    hdrs = ["coo/common.h"],
    deps = [":utils"],
)

cuda_binary(
    name = "coo",
    srcs = ["coo/main.cu"],
    deps = [":coo_kernel", ":utils"],
)

##
# csr
##
cuda_library(
    name = "csr_kernel",
    srcs = ["csr/kernel.cu"],
    hdrs = ["csr/common.h"],
    deps = [":utils"],
)

cuda_binary(
    name = "csr",
    srcs = ["csr/main.cu"],
    deps = [":csr_kernel", ":utils"],
)

##
# ell
##
cuda_library(
    name = "ell_kernel",
    srcs = ["ell/kernel.cu"],
    hdrs = ["ell/common.h"],
    deps = [":utils"],
)

cuda_binary(
    name = "ell",
    srcs = ["ell/main.cu"],
    deps = [":ell_kernel", ":utils"],
)

cuda_binary(
    name = "bench",
    srcs = ["bench.cu"],
    deps = [
        ":coo_kernel",
        ":csr_kernel",
        ":ell_kernel",
        ":utils",
        "@google_benchmark//:benchmark_main",
    ],
)

# rules_cuda offers:
# @cuda//:cublas
# @cuda//:cusparse
# @cuda//:cusolver
# @cuda//:cufft
# @cuda//:curand
# @cuda//:cudart
# @cuda//:cupti
