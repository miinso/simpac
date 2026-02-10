#pragma once

#include <flecs.h>

#include "components.h"

namespace graphics {

namespace props {
inline flecs::entity shows_statistics;
inline flecs::entity show_grid;
inline flecs::entity grid_slices;
inline flecs::entity grid_spacing;
inline flecs::entity background_color;

inline void seed(flecs::world& world) {
    props::shows_statistics = world.entity("Config::graphics::showsStatistics")
        .set<bool>(true)
        .add<Configurable>();
    props::show_grid = world.entity("Config::graphics::debugOptions::showGrid")
        .set<bool>(true)
        .add<Configurable>();
    props::grid_slices = world.entity("Config::graphics::debugOptions::gridSlices")
        .set<int>(12)
        .add<Configurable>();
    props::grid_spacing = world.entity("Config::graphics::debugOptions::gridSpacing")
        .set<float>(10.0f / 12.0f)
        .add<Configurable>();
    props::background_color = world.entity("Config::graphics::backgroundColor")
        .set<ColorRGBA>({1.0f, 1.0f, 1.0f, 1.0f})
        .add<Configurable>();
}
} // namespace props

} // namespace graphics
