#pragma once

#include "../../components/render.h"
#include "../../queries.h"
#include "../../vars.h"
#include "graphics.h"

#include <raylib.h>

#include <algorithm>
#include <limits>
#include <cmath>

namespace systems {
namespace detail {

inline bool ray_plane_intersection(const Ray& ray, const vec3f& plane_point, const vec3f& plane_normal, vec3f& out_hit) {
    Eigen::Vector3r ray_origin((Real)ray.position.x, (Real)ray.position.y, (Real)ray.position.z);
    Eigen::Vector3r ray_dir((Real)ray.direction.x, (Real)ray.direction.y, (Real)ray.direction.z);

    // reject degenerate planes
    Real normal_len2 = plane_normal.map().squaredNorm();
    if (normal_len2 < (Real)1e-12f) return false;
    Eigen::Vector3r n = plane_normal.map() / (Real)std::sqrt(normal_len2);

    // near-zero denominator means ray is parallel to plane
    Real denom = ray_dir.dot(n);
    if (std::fabs(denom) < (Real)1e-6f) return false;

    Real t = (plane_point.map() - ray_origin).dot(n) / denom;
    // ignore hits behind the camera ray origin
    if (t < (Real)0) return false;

    out_hit.map() = ray_origin + t * ray_dir;
    return true;
}

inline void set_velocity_zero(const flecs::entity& e) {
    if (!e.is_alive() || !e.has<Velocity>()) return;
    e.get_mut<Velocity>().map().setZero();
}

inline void apply_virtual_spring(const flecs::entity& e, const Eigen::Vector3r& target, Real dt, Real k, Real d) {
    if (!e.is_alive() || !e.has<Position>() || !e.has<Velocity>() || !e.has<Mass>()) return;
    auto& pos = e.get_mut<Position>();
    auto& vel = e.get_mut<Velocity>();
    Real mass = std::max((Real)e.get<Mass>(), (Real)1e-6f);
    Eigen::Vector3r force = k * (target - pos.map()) - d * vel.map();
    vel.map() += (dt / mass) * force;
}

inline void release_drag_pin(ParticleInteractionState& pick) {
    if (!pick.drag_added_pin) return;
    if (pick.pressed.is_alive()) {
        set_velocity_zero(pick.pressed);
        // remove only the pin we added for kinematic dragging
        if (pick.pressed.has<IsPinned>()) {
            pick.pressed.remove<IsPinned>();
        }
    }
    pick.drag_added_pin = false;
}

} // namespace detail

namespace interaction {

// input/state stage:
// - updates hovered/pressed/selected
// - computes drag plane and offset
// - does not move particles (drag systems do that)
inline void pick_particles(flecs::iter& it) {
    auto& pick = it.world().get_mut<ParticleInteractionState>();
    auto& renderer = it.world().get<ParticleRenderer>();
    graphics::input::capture_mouse_left = false;
    Vector2 mouse = GetMousePosition();
    bool mouse_down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    pick.pointer_pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    pick.pointer_released = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    Ray ray = GetMouseRay(mouse, graphics::get_raylib_camera_const());
    float closest = std::numeric_limits<float>::max();
    flecs::entity hovered = flecs::entity::null();

    queries::particle_pick_query.each([&](flecs::entity e, const Position& pos, const Mass& mass, ParticleState&) {
        Real radius_scale = std::pow(std::max((Real)mass, (Real)1e-12f), (Real)0.0752575f); // 10000x mass -> 2x radius
        float pick_radius = renderer.base_radius * (float)radius_scale * pick.pick_radius_scale;
        RayCollision hit = GetRayCollisionSphere(ray, {(float)pos[0], (float)pos[1], (float)pos[2]}, pick_radius);
        if (hit.hit && hit.distance < closest) {
            closest = hit.distance;
            hovered = e;
        }
    });

    pick.hovered = hovered;

    if (pick.pointer_pressed) {
        // reset any leftover drag state before starting a new gesture
        detail::release_drag_pin(pick);
        pick.pointer_down = true;
        pick.dragging = false;
        pick.drag_added_pin = false;
        pick.pressed = flecs::entity::null();
        pick.press_x = mouse.x;
        pick.press_y = mouse.y;
        pick.last_x = mouse.x;
        pick.last_y = mouse.y;

        // pinned particles are pickable/hoverable but cannot be dragged
        if (hovered.is_alive() && !hovered.has<IsPinned>()) {
            pick.pressed = hovered;

            const auto& pos = hovered.get<Position>();
            pick.drag_plane_point.map() = pos.map();

            vec3f normal = {(Real)ray.direction.x, (Real)ray.direction.y, (Real)ray.direction.z};
            Real normal_len2 = normal.map().squaredNorm();
            if (normal_len2 < (Real)1e-12f) {
                pick.drag_plane_normal = {0.0f, 1.0f, 0.0f};
            } else {
                pick.drag_plane_normal.map() = normal.map() / (Real)std::sqrt(normal_len2);
            }

            vec3f press_hit;
            if (detail::ray_plane_intersection(ray, pick.drag_plane_point, pick.drag_plane_normal, press_hit)) {
                // keep the same grab point relative to the particle center
                pick.drag_offset.map() = pos.map() - press_hit.map();
            } else {
                pick.drag_offset.map().setZero();
            }
        }
    }

    if (pick.pointer_down && mouse_down) {
        float dx = mouse.x - pick.press_x;
        float dy = mouse.y - pick.press_y;
        float threshold2 = pick.drag_threshold_px * pick.drag_threshold_px;
        if (!pick.dragging
            && pick.pressed.is_alive()
            // avoid treating micro jitters as drag start
            && (dx * dx + dy * dy) >= threshold2) {
            pick.dragging = true;
        }

        pick.last_x = mouse.x;
        pick.last_y = mouse.y;
    }

    // input canceled covers focus loss / input glitches where release event is missed
    bool canceled = pick.pointer_down && !mouse_down && !pick.pointer_released;
    if (pick.pointer_released || canceled) {
        if (pick.pointer_released && !pick.dragging) {
            // click toggles selection, drag does not
            flecs::entity click_target = pick.pressed.is_alive() ? pick.pressed : hovered;
            if (click_target.is_alive()) {
                pick.selected = (pick.selected == click_target)
                    ? flecs::entity::null()
                    : click_target;
            } else {
                pick.selected = flecs::entity::null();
            }
        }
        detail::release_drag_pin(pick);
        pick.pointer_down = false;
        pick.dragging = false;
        pick.pressed = flecs::entity::null();
    }

    graphics::input::capture_mouse_left = pick.pointer_down && pick.pressed.is_alive();

    queries::particle_pick_query.each([&](flecs::entity e, const Position&, const Mass&, ParticleState& state) {
        // interaction bits are fully derived each frame from pick state
        constexpr uint32_t kInteractionMask = ParticleState::Hovered | ParticleState::Selected | ParticleState::Dragged;
        uint32_t flags = (uint32_t)(state.flags & ~kInteractionMask);
        if (e == hovered) flags |= ParticleState::Hovered;
        if (pick.selected == e) flags |= ParticleState::Selected;
        if (pick.dragging && pick.pressed == e) flags |= ParticleState::Dragged;
        state.flags = flags;
    });
}

// drag stage (mode: kinematic):
// - temporarily pins dragged particle
// - writes position directly to cursor target
inline void drag_particles_kinematic(flecs::iter& it) {
    auto& pick = it.world().get_mut<ParticleInteractionState>();
    // mode and liveness guards keep this system no-op unless actively dragging
    if (!pick.dragging || !pick.pressed.is_alive()) return;
    if (pick.drag_mode != ParticleInteractionState::Kinematic) return;

    if (!pick.drag_added_pin) {
        pick.pressed.add<IsPinned>();
        pick.drag_added_pin = true;
    }

    Vector2 mouse = GetMousePosition();
    Ray ray = GetMouseRay(mouse, graphics::get_raylib_camera_const());
    vec3f hit;
    // if cursor ray cannot hit drag plane this frame, keep last position
    if (!detail::ray_plane_intersection(ray, pick.drag_plane_point, pick.drag_plane_normal, hit)) return;

    auto& pos = pick.pressed.get_mut<Position>();
    pos.map() = hit.map() + pick.drag_offset.map();
    detail::set_velocity_zero(pick.pressed);
}

// drag stage (mode: virtual spring):
// - applies F = k(x_target - x) - d v
// - keeps particle unpinned
inline void drag_particles_virtual_spring(flecs::iter& it) {
    auto& pick = it.world().get_mut<ParticleInteractionState>();
    // mode and liveness guards keep this system no-op unless actively dragging
    if (!pick.dragging || !pick.pressed.is_alive()) return;
    if (pick.drag_mode != ParticleInteractionState::VirtualSpring) return;

    if (pick.drag_added_pin) {
        detail::release_drag_pin(pick);
    }

    Vector2 mouse = GetMousePosition();
    Ray ray = GetMouseRay(mouse, graphics::get_raylib_camera_const());
    vec3f hit;
    if (!detail::ray_plane_intersection(ray, pick.drag_plane_point, pick.drag_plane_normal, hit)) return;

    detail::apply_virtual_spring(
        pick.pressed,
        hit.map() + pick.drag_offset.map(),
        props::dt.get<Real>(),
        (Real)pick.virtual_spring_k,
        (Real)pick.virtual_spring_d
    );
}

} // namespace interaction

} // namespace systems
