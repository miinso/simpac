##
#  ANGLE (dynamic/shared lib) (ANGLE 2.1.23876 git hash: fffbc739779a)
##
cc_import(
    name = "libEGL",
    shared_library = select({
        "@platforms//os:macos": "libEGL.dylib",  # universal
        "@platforms//os:windows": "libEGL.dll",  # x86_64
        "@platforms//os:linux": "libEGL.so",  # x86_64
    }),
    visibility = ["//visibility:public"],
)

cc_import(
    name = "libGLESv2",
    shared_library = select({
        "@platforms//os:macos": "libGLESv2.dylib",  # universal
        "@platforms//os:windows": "libGLESv2.dll",  # x86_64
        "@platforms//os:linux": "libGLESv2.so",  # x86_64
    }),
    visibility = ["//visibility:public"],
)

# convenience target that bundles both
cc_library(
    name = "angle",
    hdrs = [
        "include/EGL/egl.h",
        "include/EGL/eglext.h",
        "include/EGL/eglext_angle.h",
    ],
    data = select({
        "@platforms//os:macos": [
            "//third_party/angle:libEGL.dylib",
            "//third_party/angle:libGLESv2.dylib",
        ],
        "@platforms//os:linux": [
            "//third_party/angle:libEGL.so",
            "//third_party/angle:libGLESv2.so",
        ],
        "@platforms//os:windows": [
            # TODO: do we want to provide `d3dcompiler_47.dll` here?
            # "//third_party/angle:d3dcompiler_47.dll"
        ],
        "//conditions:default": [],
    }),
    includes = ["include"],
    visibility = ["//visibility:public"],
    deps = [
        ":libEGL",
        ":libGLESv2",
    ],
)


