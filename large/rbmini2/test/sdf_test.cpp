#include "sdf/analytic.hpp"
#include "sdf/components.hpp"
#include "sdf/discrete.hpp"
#include "sdf/operations.hpp"
#include <gtest/gtest.h>

using namespace phys;

class SDFTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Common setup code if needed
    }

    // Helper function to verify contact point is on surface
    void verify_surface_point(const components::SDFCollider& collider,
                              const Vector3r& point,
                              const Real tolerance = 1e-4f) {
        Real dist = static_cast<Real>(sdf::distance(collider, point, 0.0f));
        EXPECT_NEAR(dist, 0.0f, tolerance) << "Point " << point.transpose() << " is not on surface";
    }

    // Helper to verify normal is unit length
    void verify_normal(const Vector3r& normal, const Real tolerance = 1e-6f) {
        EXPECT_NEAR(normal.norm(), 1.0f, tolerance)
            << "Normal " << normal.transpose() << " is not normalized";
    }
};

// Box SDF Tests
TEST_F(SDFTest, BoxSDF) {
    components::SDFCollider box;
    box.type = sdf::SDFType::Box;
    box.box.half_extents = Vector3r(1.0f, 1.0f, 1.0f);

    // Distance field tests
    {
        // Interior point
        Vector3r interior(0.5f, 0.5f, 0.5f);
        EXPECT_LT(sdf::distance(box, interior, 0.0f), 0.0f);

        // Surface point
        Vector3r surface(1.0f, 0.0f, 0.0f);
        EXPECT_NEAR(sdf::distance(box, surface, 0.0f), 0.0f, 1e-6f);

        // Exterior point
        Vector3r exterior(2.0f, 0.0f, 0.0f);
        EXPECT_GT(sdf::distance(box, exterior, 0.0f), 0.0f);
    }

    // Collision tests
    {
        Vector3r point(2.0f, 0.0f, 0.0f);
        Vector3r cp, normal;
        Real dist;

        EXPECT_TRUE(sdf::collision_test(box, point, 0.0f, cp, normal, dist, 2.0f));
        verify_surface_point(box, cp);
        verify_normal(normal);
        EXPECT_NEAR(normal.x(), 1.0f, 1e-6f) << "Incorrect normal direction";
    }

    // Scale tests
    {
        box.scale = Vector3r(2.0f, 1.0f, 1.0f);
        Vector3r point(2.0f, 0.0f, 0.0f);
        EXPECT_NEAR(sdf::distance(box, point, 0.0f), 0.0f, 1e-6f)
            << "Scaled box surface point test failed";
    }

    // Normal inversion tests
    {
        box.scale = Vector3r::Ones();
        box.invert_sdf = -1.0f;
        Vector3r point(2.0f, 0.0f, 0.0f);
        Vector3r cp, normal;
        Real dist;

        EXPECT_TRUE(sdf::collision_test(box, point, 0.0f, cp, normal, dist, 2.0f));
        EXPECT_NEAR(normal.x(), -1.0f, 1e-6f) << "Inverted normal incorrect";
    }
}

// Sphere SDF Tests
TEST_F(SDFTest, SphereSDF) {
    components::SDFCollider sphere;
    sphere.type = sdf::SDFType::Sphere;
    sphere.sphere.radius = 1.0f;

    // Distance field tests
    {
        // Interior
        Vector3r interior(0.5f, 0.0f, 0.0f);
        EXPECT_LT(sdf::distance(sphere, interior, 0.0f), 0.0f);

        // Surface
        Vector3r surface(1.0f, 0.0f, 0.0f);
        EXPECT_NEAR(sdf::distance(sphere, surface, 0.0f), 0.0f, 1e-6f);

        // Exterior
        Vector3r exterior(2.0f, 0.0f, 0.0f);
        EXPECT_GT(sdf::distance(sphere, exterior, 0.0f), 0.0f);
    }

    // Normal computation test
    {
        Vector3r point(1.0f, 0.0f, 0.0f);
        Vector3r normal = sdf::approximate_normal(sphere, point, 0.0f);
        verify_normal(normal);
        EXPECT_NEAR(normal.x(), 1.0f, 1e-6f);
        EXPECT_NEAR(normal.y(), 0.0f, 1e-6f);
        EXPECT_NEAR(normal.z(), 0.0f, 1e-6f);
    }
}

// Grid SDF Tests
// Grid SDF Tests
TEST_F(SDFTest, GridSDF) {
    // 2x2x2 box mesh for grid SDF (-1 to 1 in each dimension)
    std::vector<Vector3r> vertices = {
        Vector3r(-1.0f, -1.0f, -1.0f),
        Vector3r(1.0f, -1.0f, -1.0f),
        Vector3r(1.0f, 1.0f, -1.0f),
        Vector3r(-1.0f, 1.0f, -1.0f),
        Vector3r(-1.0f, -1.0f, 1.0f),
        Vector3r(1.0f, -1.0f, 1.0f),
        Vector3r(1.0f, 1.0f, 1.0f),
        Vector3r(-1.0f, 1.0f, 1.0f)};

    std::vector<unsigned int> indices = {
        0, 2, 1, 0, 3, 2, // front
        1, 2, 6, 1, 6, 5, // right
        5, 6, 7, 5, 7, 4, // back
        4, 7, 3, 4, 3, 0, // left
        3, 7, 6, 3, 6, 2, // top
        4, 0, 1, 4, 1, 5 // bottom
    };

    components::SDFCollider collider;
    collider.type = sdf::SDFType::Grid;
    collider.grid = sdf::generate_grid_sdf(vertices, indices, Vector3i(32, 32, 32));
    ASSERT_TRUE(collider.grid != nullptr);
    ASSERT_TRUE(collider.grid->grid != nullptr);

    // Get actual grid bounds for debug
    const auto& domain = collider.grid->grid->domain();
    std::cout << "Grid domain:\n"
              << "Min: " << domain.min().transpose() << "\n"
              << "Max: " << domain.max().transpose() << "\n";

    // Distance tests
    {
        // Interior point (halfway from center to surface)
        Vector3r interior(0.5f, 0.0f, 0.0f);
        double dist_interior = sdf::distance(collider, interior, 0.0f);
        EXPECT_LT(dist_interior, 0.0f);

        // Surface point (borderline surface)
        Vector3r surface(0.9f, 0.0f, 0.0f);
        double dist_surface = sdf::distance(collider, surface, 0.0f);
        EXPECT_NEAR(dist_surface, -0.1f, 1e-6);

        // Exterior point (in domain)
        Vector3r exterior(1.05f, 0.0f, 0.0f);
        double dist_exterior = sdf::distance(collider, exterior, 0.0f);
        EXPECT_GT(dist_exterior, 0.0f);

        // Far exterior point (out of domain)
        Vector3r far_exterior(1.5f, 0.0f, 0.0f);
        double dist_far = sdf::distance(collider, far_exterior, 0.0f);
        EXPECT_EQ(dist_far, std::numeric_limits<double>::max());
    }

    // Collision tests
    {
        Vector3r point(1.05f, 0.0f, 0.0f); // in domain
        Vector3r cp, normal;
        Real dist;

        bool collision = sdf::collision_test(collider, point, 0.0f, cp, normal, dist, 0.5f);
        EXPECT_TRUE(collision);
        verify_normal(normal);
        EXPECT_NEAR(normal.x(), 1.0f, 0.1f);
        verify_surface_point(collider, cp, 0.1f);
    }

    // Scale tests
    {
        collider.scale = Vector3r(2.0f, 1.0f, 1.0f);
        Vector3r point(1.8f, 0.0f, 0.0f); // Scaled box near surface
        Vector3r cp, normal;
        Real dist;

        bool collision = sdf::collision_test(collider, point, 0.0f, cp, normal, dist, 0.5f);
        std::cout << "Scaled collision test results:\n"
                  << "Point: " << point.transpose() << "\n"
                  << "Distance: " << dist << "\n"
                  << "Normal: " << normal.transpose() << "\n"
                  << "Contact point: " << cp.transpose() << "\n";

        EXPECT_TRUE(collision);
        verify_normal(normal);
        EXPECT_NEAR(normal.x(), 1.0f, 0.1f);
    }

    // SDF inversion test
    {
        collider.scale = Vector3r::Ones();
        collider.invert_sdf = -1.0f;
        Vector3r point(1.05f, 0.0f, 0.0f); // inward domain
        Vector3r cp, normal;
        Real dist;

        bool collision = sdf::collision_test(collider, point, 0.0f, cp, normal, dist, 0.2f);
        std::cout << "Inverted normal test results:\n"
                  << "Distance: " << dist << "\n"
                  << "Normal: " << normal.transpose() << "\n";

        EXPECT_TRUE(collision);
        verify_normal(normal); // normal should be unit length
        EXPECT_NEAR(normal.x(), -1.0f, 0.1f) << "Inverted normal should point in -x direction";
    }
}

// Edge Cases and Error Handling
TEST_F(SDFTest, EdgeCases) {
    components::SDFCollider collider;
    Vector3r point(1.0f, 0.0f, 0.0f);
    Vector3r cp, normal;
    Real dist;

    // Uninitialized collider
    EXPECT_FALSE(sdf::collision_test(collider, point, 0.0f, cp, normal, dist));
    EXPECT_EQ(sdf::distance(collider, point, 0.0f), std::numeric_limits<double>::max());

    // Zero scale test
    // collider.type = sdf::SDFType::Box;
    // collider.box.half_extents = Vector3r(1.0f, 1.0f, 1.0f);
    // collider.scale = Vector3r::Zero();

    // normal = sdf::approximate_normal(collider, point, 0.0f);
    // EXPECT_TRUE(normal.isZero()) << "Normal should be zero for zero scale";
}

// Tolerance Tests
TEST_F(SDFTest, ToleranceTests) {
    components::SDFCollider sphere;
    sphere.type = sdf::SDFType::Sphere;
    sphere.sphere.radius = 1.0f;

    Vector3r point(1.1f, 0.0f, 0.0f);
    Vector3r cp, normal;
    Real dist;

    // Without tolerance - should not detect collision
    EXPECT_FALSE(sdf::collision_test(sphere, point, 0.0f, cp, normal, dist, 0.05f));

    // With tolerance - should detect collision
    EXPECT_TRUE(sdf::collision_test(sphere, point, 0.2f, cp, normal, dist, 0.05f));
}