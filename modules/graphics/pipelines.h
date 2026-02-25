#pragma once

#include <flecs.h>

namespace graphics {

// rendering phases in order:
// PostUpdate -> PreRender -> OnShadowPass -> OnRender -> OnRenderOverlay -> PostRender -> OnPresent
//
// OnShadowPass:    depth-only pass from light POV (shadow map generation)
// OnRender:        lit geometry (PBR shader active if LightRenderer present)
// OnRenderOverlay: unlit 3D draws — gizmos, debug shapes, wireframes
inline flecs::entity PreRender;
inline flecs::entity OnShadowPass;
inline flecs::entity OnRender;
inline flecs::entity OnRenderOverlay;
inline flecs::entity PostRender;
inline flecs::entity OnPresent;

} // namespace graphics
