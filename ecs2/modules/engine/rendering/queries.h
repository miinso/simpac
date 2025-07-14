#pragma once

#include <flecs.h>

#include "components.h"

namespace rendering {
    // namespace queries { cpp17 inline variables
    //     inline flecs::query<Renderable> query_all_renderables;
    //     inline flecs::query<Renderable> query_visible_renderables;
    // } // namespace queries

    // namespace queries { cpp11 workaround 1
    //     extern flecs::query<Renderable> query_all_renderables;
    //     extern flecs::query<Renderable> query_visible_renderables;
    // } // namespace queries

    struct queries { // cpp11 workaround 2
        static flecs::query<Renderable> query_all_renderables;
        static flecs::query<Renderable> query_visible_renderables;
    };

    // namespace queries { // cpp11 workaround 3
    //     inline flecs::query<Renderable> &query_all_renderables() {
    //         static flecs::query<Renderable> query;
    //         return query;
    //     };
    //     inline flecs::query<Renderable> &query_visible_renderables() {
    //         static flecs::query<Renderable> query;
    //         return query;
    //     };
    // }; // namespace queries

} // namespace rendering
