#pragma once

#include <flecs.h>

#include "components.h"

namespace core {
    namespace queries {
        // inline flecs::query<Position, Tag> &position_and_tag() { // cpp workaround 3
        //     static flecs::query<Position, Tag> query;
        //     return query;
        // };
        extern flecs::query<Position, Tag> position_and_tag;
    } // namespace queries
} // namespace core
