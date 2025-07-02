#pragma once

#include <flecs.h>

#include "components.h"

namespace rendering {
    namespace queries {
        inline flecs::query<Renderable> &query_all_renderables() {
            static flecs::query<Renderable> query;
            return query;
        };

        inline flecs::query<Renderable> &query_visible_renderables() {
            static flecs::query<Renderable> query;
            return query;
        };
    } // namespace queries
} // namespace rendering
