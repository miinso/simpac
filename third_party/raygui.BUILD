load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "styles",
    hdrs = [
        "styles/amber/style_amber.h",
        "styles/ashes/style_ashes.h",
        "styles/bluish/style_bluish.h",
        "styles/candy/style_candy.h",
        "styles/cherry/style_cherry.h",
        "styles/cyber/style_cyber.h",
        "styles/dark/style_dark.h",
        "styles/enefete/style_enefete.h",
        "styles/genesis/style_genesis.h",
        "styles/jungle/style_jungle.h",
        "styles/lavanda/style_lavanda.h",
        "styles/rltech/style_rltech.h",
        "styles/sunny/style_sunny.h",
        "styles/terminal/style_terminal.h",
    ],
    include_prefix = "styles",
    strip_include_prefix = "styles",
)

cc_library(
    name = "raygui",
    hdrs = ["src/raygui.h"],
    strip_include_prefix = "src",
    deps = [
        "@raylib-latest",
        ":styles",
    ],
)
