load("@emsdk//emscripten_toolchain:wasm_rules.bzl", "wasm_cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_binary")

assets_files = glob(["assets/**"])
# 2) local glob list (pkg assets)
assets_embed_files = [f for f in assets_files if f != "assets/myfile.txt"]
# 3) local per-file list (exclude explicit embed below)
assets_embed_linkopts = [
    opt
    for f in assets_embed_files
    for opt in ["--embed-file", "$(location %s)@%s" % (f, f)]
]

filegroup(
    name = "assets",
    srcs = assets_files,
)

cc_binary(
    name = "main",
    srcs = ["main.cpp"],
    copts = select({
        "@platforms//os:emscripten": [
            "-O2",
        ],
        "//conditions:default": [
            "-O2",
        ],
    }),
    # 1) root resources via runfiles
    # 2) local glob via runfiles
    # 3) local per-file via runfiles (for $(location ...))
    data = [
        "//:resources",
        ":assets",
    ] + assets_files,
    defines = select({
        "@platforms//os:emscripten": [
            "PLATFORM_WEB",
        ],
        "//conditions:default": [
            "PLATFORM_DESKTOP",
        ],
    }),
    linkopts = select({
        "@platforms//os:emscripten": [
            "-sALLOW_MEMORY_GROWTH=1",
            "-sSTACK_SIZE=16mb",
            "-sINITIAL_HEAP=16mb",
            "-sMAXIMUM_MEMORY=1024mb",
            "-sMODULARIZE=1",
            "-sEXPORT_NAME=createModule",
            "-sMIN_WEBGL_VERSION=2",
            "-sMAX_WEBGL_VERSION=2",
            "-sENVIRONMENT=web,worker",
            "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS']", # exposing 'FS'
            "-sEXPORTED_FUNCTIONS=['_main','_emscripten_shutdown','_emscripten_resize']",
            # wasm embed (path remap to assets/..)
            "--embed-file resources/generic.fnt@resources/generic.fnt", # 1) root per-file
            "--embed-file resources/generic.png@resources/generic.png", # 1) root per-file
            "--embed-file %s/assets/myfile.txt@assets/myfile.txt" % package_name(), # 3) local per-file added like this
        ] + assets_embed_linkopts, # 2) local glob added like this
        "//conditions:default": [],
    }),
    deps = [
        "//modules:graphics2",
        "@flecs",
        "@raylib-latest",
    ],
)

wasm_cc_binary(
    name = "webapp",
    cc_target = ":main",
    tags = ["wasm"],
    threads = "off",
)
