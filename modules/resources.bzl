# Resource embed lists for WASM builds
# Use these in linkopts select() for WASM targets

# Font required by graphics2 module
GRAPHICS2_FONT_EMBEDS = [
    "--embed-file resources/fonts/Px437_IBM_VGA_8x16.ttf@resources/fonts/Px437_IBM_VGA_8x16.ttf",
]

# Shader embeds for different GLSL versions
GRAPHICS2_SHADER_EMBEDS_GLSL100 = [
    "--embed-file resources/shaders/glsl100/lighting.vs@resources/shaders/glsl100/lighting.vs",
    "--embed-file resources/shaders/glsl100/lighting.fs@resources/shaders/glsl100/lighting.fs",
]

GRAPHICS2_SHADER_EMBEDS_GLSL300ES = [
    "--embed-file resources/shaders/glsl300es/pbr.vs@resources/shaders/glsl300es/pbr.vs",
    "--embed-file resources/shaders/glsl300es/pbr.fs@resources/shaders/glsl300es/pbr.fs",
]

# Combined base embeds for graphics2 (font only by default)
GRAPHICS2_WASM_EMBEDS = GRAPHICS2_FONT_EMBEDS
