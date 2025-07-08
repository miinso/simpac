#pragma once

#include <flecs.h>

#include "components.h"

namespace rendering {
    // namespace queries {
    //     inline flecs::query<Renderable> query_all_renderables;
    //     inline flecs::query<Renderable> query_visible_renderables;
    // } // namespace queries

    // namespace queries {
    //     extern flecs::query<Renderable> query_all_renderables;
    //     extern flecs::query<Renderable> query_visible_renderables;
    // } // namespace queries

    struct queries {
        static flecs::query<Renderable> query_all_renderables;
        static flecs::query<Renderable> query_visible_renderables;
    };
} // namespace rendering
