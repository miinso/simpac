#pragma once

#include <flecs.h>

#include "components.h"

namespace rendering {
    namespace queries {
        inline flecs::query<Renderable> query_all_renderables;
        inline flecs::query<Renderable> query_visible_renderables;
    } // namespace queries
} // namespace rendering
