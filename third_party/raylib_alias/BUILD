alias(
    name = "raylib",
    actual = select({
        "@platforms//os:windows": "@raylib_windows//:raylib",
        "@platforms//os:macos": "@raylib_macos//:raylib",
        "@platforms//os:linux": "@raylib_linux//:raylib",
        "@platforms//os:emscripten": "@raylib_emscripten//:raylib",
    }),
    visibility = ["//visibility:public"],
)
