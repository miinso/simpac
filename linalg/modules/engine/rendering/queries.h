#pragma once

#include <flecs.h>

#include "components.h"

namespace rendering::queries {
    inline flecs::query<Renderable> all_renderables{};
    inline flecs::query<Renderable> visible_renderables{};

    // [[nodiscard]] inline flecs::query<Renderable> &all() noexcept { return all_renderables; }

    // [[nodiscard]] inline flecs::query<Renderable> &visible() noexcept {
    //     return visible_renderables;
    // }
} // namespace rendering::queries
