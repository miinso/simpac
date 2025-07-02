#pragma once

#include <flecs.h>

#include "components.h"

namespace core {
    namespace systems {
        inline void destroy_entity_after_frame(flecs::iter &it, size_t i) {
            it.entity(i).destruct();
        }

        inline void destroy_entity_after_time(flecs::iter &it, size_t i, DestroyAfterTime &time) {
            time.time -= it.delta_time();
            if (time.time <= 0.0f) {
                it.entity(i).add<DestroyAfterFrame>();
            }
        }

        inline void remove_empty_tables(const flecs::world &world) {
            ecs_delete_empty_tables_desc_t desc;
            desc.delete_generation = true;
            ecs_delete_empty_tables(world.c_ptr(), &desc);
        }
    } // namespace systems
} // namespace core
