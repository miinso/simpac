#pragma once

#include <flecs.h>

#include "components.h"

namespace core {
    namespace queries {
        inline flecs::query<Position, Tag> &position_and_tag() {
            static flecs::query<Position, Tag> query;
            return query;
        };
    } // namespace queries
} // namespace core
