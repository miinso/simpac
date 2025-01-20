// rigid_body.hpp
// for pbd
#pragma once

#include <Eigen/Dense>
#include <flecs.h>
// #include <iostream>
// #include "graphics.h"
// #include "geometry.hpp"
#include "components.hpp"


// static const Eigen::Vector3f gravity(0.0f, -9.87f, 0.0f);
static const Eigen::Vector3f gravity(0.0f, 0.0f, 0.0f);


void rb_integrate_linear(flecs::iter& it, size_t,
                        Position& pos, LinearVelocity& vel,
                        Acceleration& acc, OldPosition& old_pos) {
    const float dt = it.delta_time();
    vel.value += acc.value * dt;
    old_pos.value = pos.value;
    pos.value += vel.value * dt;
}

void rb_integrate_angular(flecs::iter& it, size_t,
                         Orientation& orient, AngularVelocity& omega,
                         AngularForce& torque, WorldInverseInertia& I_inv,
                         WorldInertia& I, OldOrientation& old_orient) {
    const float dt = it.delta_time();
    
    Eigen::Vector3f alpha = I_inv.value * 
        (torque.value - omega.value.cross(I.value * omega.value));
    
    omega.value += alpha * dt;
    old_orient.value = orient.value;
    
    Eigen::Quaternionf w_quat(0, omega.value.x(), 
                                omega.value.y(), 
                                omega.value.z());
    auto q_dot = Eigen::Quaternionf(0.5f * (orient.value * w_quat).coeffs());
    
    orient.value.coeffs() += q_dot.coeffs() * dt;
    orient.value.normalize();
}

void rb_update_world_inertia(Rotation& R, WorldInertia& I, WorldInverseInertia& I_inv,
                            const LocalInertia& I_local, const Orientation& orient) {
    // Eigen::Matrix3f R = orient.value.toRotationMatrix();
    R.value = orient.value.matrix();
    I.value = R.value * I_local.value * R.value.transpose();
    I_inv.value = I.value.inverse();
}

void rb_reset_forces(Acceleration& a, LinearForce& force, AngularForce& torque) {
    a.value.setZero();
    force.value.setZero();
    torque.value.setZero();
    // lambda.value.setZero();
}

// pbd specific
void rb_update_velocities(flecs::iter& it, size_t,
                         LinearVelocity& vel, const Position& pos, 
                         const OldPosition& old_pos, AngularVelocity& omega,
                         const Orientation& orient, const OldOrientation& old_orient) {
    const float dt = it.delta_time();
    
    vel.value = (pos.value - old_pos.value) / dt;
    
    auto q_diff = (orient.value * old_orient.value.inverse());

    Eigen::Vector3f new_omega = 2.0f * q_diff.vec() / dt;
    // if (q_diff.w() < 0) new_omega = -new_omega;
    
    omega.value = new_omega;
}

void rb_apply_gravity(Acceleration& a, const Mass& mass) {
    a.value += gravity;
}

// debug text rendering system
void draw_rb_debug_text(
    const Position& pos,
    const Orientation& orient,
    const LinearVelocity& vel,
    const AngularVelocity& ang_vel,
    const Mass& mass,
    const InverseMass& inv_mass,
    const LocalInertia& local_I,
    const WorldInertia& world_I
) {
    const int font_size = 20;
    const int line_spacing = 20;
    int y = 50;  // starting y position
    const int x = 10;  // left margin
    const auto text_color = rl::BLUE;

    // helper for vector3 formatting
    auto vec3_to_string = [](const Eigen::Vector3f& v) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.3f, %.3f, %.3f", v.x(), v.y(), v.z());
        return std::string(buf);
    };

    // helper for quaternion formatting
    auto quat_to_string = [](const Eigen::Quaternionf& q) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.3f | %.3f, %.3f, %.3f", 
                q.w(), q.x(), q.y(), q.z());
        return std::string(buf);
    };

    // helper for matrix3 formatting
    auto mat3_to_string = [](const Eigen::Matrix3f& m) {
        std::string result;
        for(int i = 0; i < 3; i++) {
            char buf[64];
            snprintf(buf, sizeof(buf), "| %.3f, %.3f, %.3f |", 
                    m(i,0), m(i,1), m(i,2));
            result += std::string(buf) + "\n";
        }
        return result;
    };

    // draw each property
    rl::DrawText("Rigid Body Properties:", x, y, font_size, rl::BLUE);
    y += line_spacing * 2;

    // position
    rl::DrawText(("Position: " + vec3_to_string(pos.value)).c_str(), 
                 x, y, font_size, text_color);
    y += line_spacing;

    // orientation
    rl::DrawText(("Orientation: " + quat_to_string(orient.value)).c_str(), 
                 x, y, font_size, text_color);
    y += line_spacing;

    // linear velocity
    rl::DrawText(("Linear Velocity: " + vec3_to_string(vel.value)).c_str(), 
                 x, y, font_size, text_color);
    y += line_spacing;

    // angular velocity
    rl::DrawText(("Angular Velocity: " + vec3_to_string(ang_vel.value)).c_str(), 
                 x, y, font_size, text_color);
    y += line_spacing;

    // mass
    char mass_buf[64];
    snprintf(mass_buf, sizeof(mass_buf), "Mass: %.3f (inv: %.3f)", 
             mass.value, inv_mass.value);
    rl::DrawText(mass_buf, x, y, font_size, text_color);
    y += line_spacing * 2;

    // inertia tensors
    rl::DrawText("Local Inertia Tensor:", x, y, font_size, text_color);
    y += line_spacing;
    std::string local_I_str = mat3_to_string(local_I.value);
    std::istringstream local_I_stream(local_I_str);
    std::string line;
    while (std::getline(local_I_stream, line)) {
        rl::DrawText(line.c_str(), x + 10, y, font_size, text_color);
        y += line_spacing;
    }
    y += line_spacing;

    rl::DrawText("World Inertia Tensor:", x, y, font_size, text_color);
    y += line_spacing;
    std::string world_I_str = mat3_to_string(world_I.value);
    std::istringstream world_I_stream(world_I_str);
    while (std::getline(world_I_stream, line)) {
        rl::DrawText(line.c_str(), x + 10, y, font_size, text_color);
        y += line_spacing;
    }

    // std::cout << vec3_to_string(pos.value) << std::endl;
}

flecs::entity rb_add_rigid_body(flecs::world& ecs, float density) {
    auto e = ecs.entity();
    
    // Because it's a rigid body
    e.add<RigidBody>();

    // Add all required components
    e.add<Position>()
     .add<Orientation>()
     .add<Rotation>()
     .add<OldPosition>()
     .add<OldOrientation>()

     .add<LinearVelocity>()
     .add<AngularVelocity>()
     
     .add<Acceleration>()
     .add<LinearForce>()
     .add<AngularForce>()
     
     .add<Density>()
     .add<Mass>()
     .add<InverseMass>()
     
     .add<LocalInertia>()
     .add<LocalInverseInertia>()
     .add<WorldInertia>()
     .add<WorldInverseInertia>()

     .add<Restitution>()
     .add<Friction>();
    //  .add<Geometry>();

    // Initialize components
    e.set<Position>({Eigen::Vector3f::Zero()});
    e.set<Orientation>({Eigen::Quaternionf::Identity()});
    e.set<OldPosition>({Eigen::Vector3f::Zero()});
    e.set<OldOrientation>({Eigen::Quaternionf::Identity()});
    e.set<Rotation>({Eigen::Matrix3f::Identity()});
    
    e.set<LinearVelocity>({Eigen::Vector3f::Zero()});
    e.set<AngularVelocity>({Eigen::Vector3f::Zero()});
    
    e.set<Acceleration>({Eigen::Vector3f::Zero()});
    e.set<LinearForce>({Eigen::Vector3f::Zero()});
    e.set<AngularForce>({Eigen::Vector3f::Zero()});

    e.set<Density>({density});
    float mass = 1.0f;
    e.set<Mass>({mass});
    e.set<InverseMass>({1.0f / mass});
    
    Eigen::Matrix3f inertia = Eigen::Matrix3f::Identity();
    e.set<LocalInertia>({inertia});
    e.set<LocalInverseInertia>({inertia.inverse()});
    e.set<WorldInertia>({inertia});
    e.set<WorldInverseInertia>({inertia.inverse()});

    e.set<Restitution>({0.6f});
    e.set<Friction>({0.2f});
    
    return e;
}

// Register systems
void register_rb_systems(flecs::world& ecs) {
    // Reset forces
    ecs.system<Acceleration, LinearForce, AngularForce>("RbResetForces")
        .with<RigidBody>()
        .each(rb_reset_forces);
    
    // Apply gravity
    ecs.system<Acceleration, const Mass>("RbApplyGravity")
        .with<RigidBody>()
        .without<IsPinned>()
        .each(rb_apply_gravity);

    // Linear integration
    ecs.system<Position, LinearVelocity, Acceleration, OldPosition>("RbIntegrateLinear")
        .with<RigidBody>()
        .without<IsPinned>()
        .each(rb_integrate_linear);

    // Angular integration
    ecs.system<Orientation, AngularVelocity, AngularForce, 
              WorldInverseInertia, WorldInertia, OldOrientation>("RbIntegrateAngular")
        .with<RigidBody>()
        .without<IsPinned>()
        .each(rb_integrate_angular);

    // World inertia update
    ecs.system<Rotation, WorldInertia, WorldInverseInertia, 
              const LocalInertia, const Orientation>("RbUpdateInertia")
        .with<RigidBody>()
        .each(rb_update_world_inertia);

    // Update velocities
    // ecs.system<LinearVelocity, const Position, const OldPosition,
    //           AngularVelocity, const Orientation, const OldOrientation>("RbUpdateVelocities")
    //     .with<RigidBody>()
    //     .without<IsPinned>()
    //     .each(rb_update_velocities);


    // // Ground contact
    // ecs.system<Position, LinearVelocity>("RbGroundContact")
    //     .with<RigidBody>()
    //     .without<IsPinned>()
    //     .each(rb_ground_contact);
}