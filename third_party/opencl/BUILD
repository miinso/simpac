load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "opencl",
    hdrs = [
        "include/CL/cl.h",
        "include/CL/cl_ext.h",
        "include/CL/cl_platform.h",
        "include/CL/cl_version.h",
    ],
    includes = ["include"],
    linkopts = select({
        "@platforms//os:windows": [
            # import lib from CUDA toolkit. adjust path if needed.
            "/LIBPATH:C:/PROGRA~1/NVIDIA~2/CUDA/v12.6/lib/x64",
            "OpenCL.lib",
        ],
        "//conditions:default": ["-lOpenCL"],
    }),
    visibility = ["//visibility:public"],
)
