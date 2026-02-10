#pragma once

#include <flecs.h>

namespace graphics {

// rendering phases in order:
// PostUpdate -> PreRender -> OnRender -> PostRender -> OnPresent
inline flecs::entity PreRender;
inline flecs::entity OnRender;
inline flecs::entity PostRender;
inline flecs::entity OnPresent;

} // namespace graphics
