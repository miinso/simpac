// sdf/components.hpp
#pragma once
#include "cubic_lagrange_discrete_grid.hpp"
#include "sdf/types.hpp"

namespace phys {
    namespace components {
        struct SphereSDF {
            Real radius;
        };

        struct BoxSDF {
            Vector3r half_extents;
        };

        struct CapsuleSDF {
            Real radius;
            Real height;
        };

        struct GridSDF {
            Discregrid::CubicLagrangeDiscreteGrid* grid;

            GridSDF()
                : grid(nullptr) { }
            ~GridSDF() {
                if (grid)
                    delete grid;
            }

            // Delete copy operations
            GridSDF(const GridSDF&) = delete;
            GridSDF& operator=(const GridSDF&) = delete;
        };

        struct SDFCollider {
            sdf::SDFType type = sdf::SDFType::None;

            // Common properties
            Real invert_sdf = 1.0f;
            Vector3r scale = Vector3r::Ones();

            union
            {
                SphereSDF sphere;
                BoxSDF box;
                CapsuleSDF capsule;
                GridSDF* grid; // unique_ptr maybe
            };

            SDFCollider()
                : type(sdf::SDFType::None)
                , grid(nullptr) { }

            ~SDFCollider() {
                if (type == sdf::SDFType::Grid && grid) {
                    delete grid;
                }
            }

            // please no copy
            SDFCollider(const SDFCollider&) = delete;
            SDFCollider& operator=(const SDFCollider&) = delete;
        };

    } // namespace components
} // namespace phys