#pragma once

// Flecs rendering phase tags
// Use these with .kind<Phase>() when registering systems

namespace graphics {

// Rendering phases in order of execution:
// PostUpdate -> PreRender -> OnRender -> PostRender -> OnPresent

struct PreRender {};   // BeginDrawing, clear, shader setup
struct OnRender {};    // BeginMode3D, main 3D rendering
struct PostRender {};  // EndMode3D, overlays (grid, FPS)
struct OnPresent {};   // EndDrawing, frame presentation

} // namespace graphics
