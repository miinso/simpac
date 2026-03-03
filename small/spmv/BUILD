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
