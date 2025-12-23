// sdf/types.hpp
#pragma once
#include "core/types.hpp"

namespace phys {
    namespace sdf {
        enum class SDFType : uint8_t { None = 0, Grid = 1, Sphere = 2, Box = 3, Capsule = 4 };

    } // namespace sdf
} // namespace phys
