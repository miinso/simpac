# Shared embed/preload files required by graphics2 module for WASM builds
# Usage in BUILD files:
#   load("//modules:graphics2.bzl", "GRAPHICS2_WASM_EMBEDS")
#   linkopts = select({
#       "@platforms//os:emscripten": [...] + GRAPHICS2_WASM_EMBEDS,
#       ...
#   })

# GRAPHICS2_WASM_EMBEDS = [
#     "--embed-file resources/fonts/Px437_IBM_VGA_8x16.ttf@resources/fonts/Px437_IBM_VGA_8x16.ttf",
# ]

# Alternative: use --preload-file for async loading (creates .data file)
# GRAPHICS2_WASM_PRELOADS = [
#     "--preload-file resources/fonts/Px437_IBM_VGA_8x16.ttf@resources/fonts/Px437_IBM_VGA_8x16.ttf",
# ]
