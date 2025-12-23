load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")

cc_test(
    name = "topology_test",
    srcs = ["topology_test.cpp"],
    deps = [
        "//modules/simplicial",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "mechanical_state_test",
    srcs = ["mechanical_state_test.cpp"],
    deps = [
        "//modules/simplicial",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "factory_test",
    srcs = ["factory_test.cpp"],
    deps = [
        "//modules/simplicial",
        "@flecs",
        "@googletest//:gtest_main",
    ],
)

# Manual integration test (requires display)
# Run with: bazel run //modules/simplicial/tests:raylib_integration_test
cc_binary(
    name = "raylib_integration_test",
    srcs = ["raylib_integration_test.cpp"],
    deps = [
        "//modules/simplicial",
        "@flecs",
        "@raylib",
    ],
)
