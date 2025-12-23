#pragma once

#include <flecs.h>

#include "components.h"

namespace rendering::queries {
    inline flecs::query<Renderable> all_renderables;
    inline flecs::query<Renderable> visible_renderables;
} // namespace rendering::queries
