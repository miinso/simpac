// src/test/core_test.cpp
#include "core/components.hpp"
#include "core/tags.hpp"
#include "core/types.hpp"
#include <gtest/gtest.h>

using namespace phys;
using namespace phys::components;

TEST(CoreTypesTest, VectorOperations) {
    Vector3r v1(1.0f, 2.0f, 3.0f);
    Vector3r v2(2.0f, 3.0f, 4.0f);

    // vector addition
    Vector3r sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum.x(), 3.0f);
    EXPECT_FLOAT_EQ(sum.y(), 5.0f);
    EXPECT_FLOAT_EQ(sum.z(), 7.0f);

    // dot product
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 20.0f); // 1*2 + 2*3 + 3*4 = 20

    // cross product
    Vector3r cross = v1.cross(v2);
    EXPECT_FLOAT_EQ(cross.x(), 2.0f * 4.0f - 3.0f * 3.0f);
    EXPECT_FLOAT_EQ(cross.y(), 3.0f * 2.0f - 1.0f * 4.0f);
    EXPECT_FLOAT_EQ(cross.z(), 1.0f * 3.0f - 2.0f * 2.0f);
}

TEST(CoreTypesTest, QuaternionOperations) {
    Quaternionr q = Quaternionr::Identity();
    EXPECT_FLOAT_EQ(q.w(), 1.0f);
    EXPECT_FLOAT_EQ(q.x(), 0.0f);
    EXPECT_FLOAT_EQ(q.y(), 0.0f);
    EXPECT_FLOAT_EQ(q.z(), 0.0f);

    // rotation around z-axis by 90 degrees
    float angle = constants::PI / 2.0f;
    Vector3r axis(0.0f, 0.0f, 1.0f);
    Quaternionr q_rot(Eigen::AngleAxisf(angle, axis));

    // apply rotation
    Vector3r v(1.0f, 0.0f, 0.0f);
    Vector3r rotated = q_rot * v;

    // 90 degree rotation transforms (1,0,0) to (0,1,0)
    EXPECT_NEAR(rotated.x(), 0.0f, constants::EPSILON);
    EXPECT_NEAR(rotated.y(), 1.0f, constants::EPSILON);
    EXPECT_NEAR(rotated.z(), 0.0f, constants::EPSILON);
}

TEST(CoreComponentsTest, MassComponents) {
    // test mass component
    Mass mass = Mass::create(2.0f);
    EXPECT_FLOAT_EQ(mass.value, 2.0f);

    // calculate inverse mass
    InverseMass inv_mass = InverseMass::from_mass(2.0f);
    EXPECT_FLOAT_EQ(inv_mass.value, 0.5f);

    // test zero mass (static object)
    InverseMass static_inv_mass = InverseMass::from_mass(0.0f);
    EXPECT_FLOAT_EQ(static_inv_mass.value, 0.0f);
}

TEST(CoreComponentsTest, TransformComponents) {
    Position pos{Vector3r(1.0f, 2.0f, 3.0f)};
    Orientation ori{Quaternionr::Identity()};
    Scale scale{Vector3r(2.0f, 2.0f, 2.0f)};

    EXPECT_FLOAT_EQ(pos.value.x(), 1.0f);
    EXPECT_FLOAT_EQ(pos.value.y(), 2.0f);
    EXPECT_FLOAT_EQ(pos.value.z(), 3.0f);

    EXPECT_FLOAT_EQ(ori.value.w(), 1.0f);
    EXPECT_FLOAT_EQ(ori.value.x(), 0.0f);
    EXPECT_FLOAT_EQ(ori.value.y(), 0.0f);
    EXPECT_FLOAT_EQ(ori.value.z(), 0.0f);

    EXPECT_FLOAT_EQ(scale.value.x(), 2.0f);
    EXPECT_FLOAT_EQ(scale.value.y(), 2.0f);
    EXPECT_FLOAT_EQ(scale.value.z(), 2.0f);
}

TEST(CoreTagsTest, TagTypes) {
    // compile-time check if tag types are properly defined as empty
    static_assert(std::is_empty_v<tags::PreUpdate>, "PreUpdate should be empty");
    static_assert(std::is_empty_v<tags::OnUpdate>, "OnUpdate should be empty");
    static_assert(std::is_empty_v<tags::PostUpdate>, "PostUpdate should be empty");
    static_assert(std::is_empty_v<tags::OnValidate>, "OnValidate should be empty");

    static_assert(std::is_empty_v<tags::RigidBody>, "RigidBody should be empty");
    static_assert(std::is_empty_v<tags::Constraint>, "Constraint should be empty");
    static_assert(std::is_empty_v<tags::Collider>, "Collider should be empty");
    static_assert(std::is_empty_v<tags::Joint>, "Joint should be empty");
}
