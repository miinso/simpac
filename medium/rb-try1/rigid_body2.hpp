// rigid_body.hpp
// for pbd
#pragma once

#include <Eigen/Dense>
#include <flecs.h>

#include "components.hpp"

static const Eigen::Vector3f gravity(0.0f, -1.87f, 0.0f);
// static const Eigen::Vector3f gravity(0.0f, 0.0f, 0.0f);

// dynamic kinetic static

void rb_integrate(
    flecs::iter& it,
    size_t i,
    LinearVelocity& v,
    AngularVelocity& omega,
    Mass& mass,
    LinearForce& f,
    ConstraintForce& f_c,
    AngularForce& tau,
    ConstraintTorque& tau_c,
    Position& x,
    Orientation& q,
    WorldInertia& I,
    WorldInverseInertia& I_inv) {
    const float dt = it.delta_time();

    if (it.entity(i).has<IsPinned>() == false) {

        // linear
        v.value += dt * (1 / mass.value) * (f.value + f_c.value);

        // angular
        Eigen::Vector3f alpha =
            I_inv.value * (tau.value + tau_c.value - omega.value.cross(I.value * omega.value));
        omega.value += alpha * dt;

        x.value += v.value * dt;
        // quaternion update (q' = q + 1/2 * dt * w * q)
        Eigen::Quaternionf w(0, omega.value.x(), omega.value.y(), omega.value.z());
        q.value.coeffs() += 0.5f * dt * (q.value * w).coeffs();
        q.value.normalize();
    } else {
        // v.value.setZero();
        // omega.value.setZero();

        // kinematic update
        x.value += v.value * dt;

        Eigen::Quaternionf w(0, omega.value.x(), omega.value.y(), omega.value.z());
        q.value.coeffs() += 0.5f * dt * (q.value * w).coeffs();
        q.value.normalize();
    }
}

void rb_integrate3(
    Real dt,
    flecs::entity e,
    Vector3r v,
    Vector3r omega,
    Real mass,
    Vector3r f,
    Vector3r f_c,
    Vector3r tau,
    Vector3r tau_c,
    Vector3r x,
    Eigen::Quaternionf q,
    Eigen::Matrix3f I,
    Eigen::Matrix3f I_inv) {
    if (e.has<IsPinned>() == false) {
        v += dt * (1 / mass) * (f + f_c);
        Vector3r alpha = I_inv * (tau + tau_c - omega.cross(I * omega));
        omega += alpha * dt;

        x += v * dt;
        Eigen::Quaternionf w(0, omega.x(), omega.y(), omega.z());
        q.coeffs() += 0.5f * dt * (q * w).coeffs();
        q.normalize();
    } else {
    }
}

void rb_integrate2(
    flecs::iter& it,
    size_t i,
    LinearVelocity& v,
    AngularVelocity& omega,
    Mass& mass,
    LinearForce& f,
    ConstraintForce& f_c,
    AngularForce& tau,
    ConstraintTorque& tau_c,
    Position& x,
    Orientation& q,
    WorldInertia& I,
    WorldInverseInertia& I_inv) {
    rb_integrate3(
        it.delta_time(),
        it.entity(i),
        v.value,
        omega.value,
        mass.value,
        f.value,
        f_c.value,
        tau.value,
        tau_c.value,
        x.value,
        q.value,
        I.value,
        I_inv.value);
}

void rb_update_world_inertia(Rotation& R,
                             WorldInertia& I,
                             WorldInverseInertia& I_inv,
                             const LocalInertia& I_local,
                             const Orientation& orient) {
    // Eigen::Matrix3f R = orient.value.toRotationMatrix();
    R.value = orient.value.matrix();
    I.value = R.value * I_local.value * R.value.transpose();
    I_inv.value = I.value.inverse();
}

void rb_reset_forces(
    LinearForce& force, AngularForce& torque, ConstraintForce& force_c, ConstraintTorque& tau_c) {
    // a.value.setZero();
    force.value.setZero();
    torque.value.setZero();
    force_c.value.setZero();
    tau_c.value.setZero();
}

void rb_clear_constraint_forces(ConstraintForce& force_c, ConstraintTorque& tau_c) {
    force_c.value.setZero();
    tau_c.value.setZero();
}

void rb_compute_contact_jacobian(Contact& contact) {
    contact.compute_contact_frame();
    contact.compute_jacobian();
}

void rb_apply_gravity(LinearForce& f, const Mass& mass) {
    f.value = mass.value * gravity;
}

// debug text rendering system
void draw_rb_debug_text(const Position& pos,
                        const Orientation& orient,
                        const LinearVelocity& vel,
                        const AngularVelocity& ang_vel,
                        const Mass& mass,
                        const InverseMass& inv_mass,
                        const LocalInertia& local_I,
                        const WorldInertia& world_I) {
    const int font_size = 20;
    const int line_spacing = 20;
    int y = 50; // starting y position
    const int x = 10; // left margin
    const auto text_color = BLUE;

    // helper for vector3 formatting
    auto vec3_to_string = [](const Eigen::Vector3f& v) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.3f, %.3f, %.3f", v.x(), v.y(), v.z());
        return std::string(buf);
    };

    // helper for quaternion formatting
    auto quat_to_string = [](const Eigen::Quaternionf& q) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.3f | %.3f, %.3f, %.3f", q.w(), q.x(), q.y(), q.z());
        return std::string(buf);
    };

    // helper for matrix3 formatting
    auto mat3_to_string = [](const Eigen::Matrix3f& m) {
        std::string result;
        for (int i = 0; i < 3; i++) {
            char buf[64];
            snprintf(buf, sizeof(buf), "| %.3f, %.3f, %.3f |", m(i, 0), m(i, 1), m(i, 2));
            result += std::string(buf) + "\n";
        }
        return result;
    };

    // draw each property
    DrawText("Rigid Body Properties:", x, y, font_size, BLUE);
    y += line_spacing * 2;

    // position
    DrawText(("Position: " + vec3_to_string(pos.value)).c_str(), x, y, font_size, text_color);
    y += line_spacing;

    // orientation
    DrawText(
        ("Orientation: " + quat_to_string(orient.value)).c_str(), x, y, font_size, text_color);
    y += line_spacing;

    // linear velocity
    DrawText(
        ("Linear Velocity: " + vec3_to_string(vel.value)).c_str(), x, y, font_size, text_color);
    y += line_spacing;

    // angular velocity
    DrawText(("Angular Velocity: " + vec3_to_string(ang_vel.value)).c_str(),
                 x,
                 y,
                 font_size,
                 text_color);
    y += line_spacing;

    // mass
    char mass_buf[64];
    snprintf(mass_buf, sizeof(mass_buf), "Mass: %.3f (inv: %.3f)", mass.value, inv_mass.value);
    DrawText(mass_buf, x, y, font_size, text_color);
    y += line_spacing * 2;

    // inertia tensors
    DrawText("Local Inertia Tensor:", x, y, font_size, text_color);
    y += line_spacing;
    std::string local_I_str = mat3_to_string(local_I.value);
    std::istringstream local_I_stream(local_I_str);
    std::string line;
    while (std::getline(local_I_stream, line)) {
        DrawText(line.c_str(), x + 10, y, font_size, text_color);
        y += line_spacing;
    }
    y += line_spacing;

    DrawText("World Inertia Tensor:", x, y, font_size, text_color);
    y += line_spacing;
    std::string world_I_str = mat3_to_string(world_I.value);
    std::istringstream world_I_stream(world_I_str);
    while (std::getline(world_I_stream, line)) {
        DrawText(line.c_str(), x + 10, y, font_size, text_color);
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
        .add<Friction>()

        .add<ConstraintForce>()
        .add<ConstraintTorque>();
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

    e.set<ConstraintForce>({Eigen::Vector3f::Zero()});
    e.set<ConstraintTorque>({Eigen::Vector3f::Zero()});

    return e;
}

// Register systems
void register_rb_systems(flecs::world& ecs) {
    // Reset forces
    ecs.system<LinearForce, AngularForce, ConstraintForce, ConstraintTorque>("RbResetForces")
        .with<RigidBody>()
        .each(rb_reset_forces);

    // Apply gravity
    ecs.system<LinearForce, const Mass>("RbApplyGravity")
        .with<RigidBody>()
        .without<IsPinned>()
        .each(rb_apply_gravity);

    // Linear integration
    // ecs.system<Position, LinearVelocity, Acceleration, OldPosition>("RbIntegrateLinear")
    //     .with<RigidBody>()
    //     .without<IsPinned>()
    //     .each(rb_integrate_linear);

    // // Angular integration
    // ecs.system<Orientation, AngularVelocity, AngularForce, WorldInverseInertia, WorldInertia, OldOrientation>(
    //        "RbIntegrateAngular")
    //     .with<RigidBody>()
    //     .without<IsPinned>()
    //     .each(rb_integrate_angular);

    // World inertia update
    ecs.system<Rotation, WorldInertia, WorldInverseInertia, const LocalInertia, const Orientation>(
           "RbUpdateInertia")
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