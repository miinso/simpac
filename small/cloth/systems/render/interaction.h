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
    const vec3f ray_origin = vec3f(ray.position);
    const vec3f ray_dir = vec3f(ray.direction);

    // reject degenerate planes
    const float normal_len2 = plane_normal.map().squaredNorm();
    if (normal_len2 < 1e-12f) return false;
    const Eigen::Vector3f n = plane_normal.map() / std::sqrt(normal_len2);

    // near-zero denominator means ray is parallel to plane
    const float denom = ray_dir.map().dot(n);
    if (std::fabs(denom) < 1e-6f) return false;

    const float t = (plane_point.map() - ray_origin.map()).dot(n) / denom;
    // ignore hits behind the camera ray origin
    if (t < 0.0f) return false;

    out_hit.map() = ray_origin.map() + t * ray_dir.map();
    return true;
}

inline void set_velocity_zero(const flecs::entity& e) {
    if (!e.is_alive() || !e.has<Velocity>()) return;
    e.get_mut<Velocity>().map().setZero();
}

inline void apply_virtual_spring(const flecs::entity& e, const Eigen::Vector3f& target, float dt, float k, float d) {
    if (!e.is_alive() || !e.has<Position>() || !e.has<Velocity>() || !e.has<Mass>()) return;
    Position& pos = e.get_mut<Position>();
    Velocity& vel = e.get_mut<Velocity>();
    const float mass = std::max((float)e.get<Mass>(), 1e-6f);
    const Eigen::Vector3f force = k * (target - pos.map()) - d * vel.map();
    vel.map() += (dt / mass) * force;
}

inline bool update_drag_plane_normal_from_camera(ParticleInteractionState& pick, const Camera3D& cam) {
    const vec3f cam_pos = vec3f(cam.position);
    vec3f cam_to_plane;
    cam_to_plane.map() = pick.drag_plane_point.map() - cam_pos.map();
    const float n_len2 = cam_to_plane.map().squaredNorm();
    if (n_len2 < 1e-12f) return false;
    pick.drag_plane_normal.map() = cam_to_plane.map() / std::sqrt(n_len2);
    return true;
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
    ParticleInteractionState& pick = it.world().get_mut<ParticleInteractionState>();
    const ParticleRenderer& renderer = it.world().get<ParticleRenderer>();
    graphics::input::capture_mouse_left = false;
    const Vector2 mouse = GetMousePosition();
    const bool mouse_down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    pick.pointer_pressed = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    pick.pointer_released = IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    const Ray ray = GetMouseRay(mouse, graphics::get_raylib_camera_const());
    float closest = std::numeric_limits<float>::max();
    flecs::entity hovered = flecs::entity::null();

    queries::particle_pick_query.each([&](flecs::entity e, const Position& pos, const Mass& mass, ParticleState&) {
        const float radius_scale = std::pow(std::max((float)mass, 1e-12f), 0.0752575f); // 10000x mass -> 2x radius
        const float pick_radius = renderer.base_radius * radius_scale * pick.pick_radius_scale;
        const vec3f sphere_center = vec3f(pos.map());
        const RayCollision hit = GetRayCollisionSphere(ray, sphere_center, pick_radius);
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

            const vec3f normal = vec3f(ray.direction);
            const float normal_len2 = normal.map().squaredNorm();
            if (normal_len2 < 1e-12f) {
                pick.drag_plane_normal = {0.0f, 1.0f, 0.0f};
            } else {
                pick.drag_plane_normal.map() = normal.map() / std::sqrt(normal_len2);
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

        if (pick.dragging && pick.pressed.is_alive()) {
            const Camera3D& cam = graphics::get_raylib_camera_const();
            // keep drag plane normal camera-facing while dragging
            detail::update_drag_plane_normal_from_camera(pick, cam);
        }

        pick.last_x = mouse.x;
        pick.last_y = mouse.y;
    }

    // input canceled covers focus loss / input glitches where release event is missed
    const bool canceled = pick.pointer_down && !mouse_down && !pick.pointer_released;
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

// drag stage (kinematic):
// - temporarily pins dragged particle
// - writes position directly to cursor target
inline void drag_particles_kinematic(flecs::iter& it) {
    ParticleInteractionState& pick = it.world().get_mut<ParticleInteractionState>();
    // liveness guards keep this system no-op unless actively dragging
    if (!pick.dragging || !pick.pressed.is_alive()) return;

    if (!pick.drag_added_pin) {
        pick.pressed.add<IsPinned>();
        pick.drag_added_pin = true;
    }

    const Camera3D& cam = graphics::get_raylib_camera_const();
    // live camera-facing drag plane while dragging
    detail::update_drag_plane_normal_from_camera(pick, cam);

    const Vector2 mouse = GetMousePosition();
    const Ray ray = GetMouseRay(mouse, cam);
    vec3f hit;
    // if cursor ray cannot hit drag plane this frame, keep last position
    if (!detail::ray_plane_intersection(ray, pick.drag_plane_point, pick.drag_plane_normal, hit)) return;

    Position& pos = pick.pressed.get_mut<Position>();
    pos.map() = hit.map() + pick.drag_offset.map();
    detail::set_velocity_zero(pick.pressed);
}

// drag stage (virtual spring):
// - applies F = k(x_target - x) - d v
// - keeps particle unpinned
inline void drag_particles_spring(flecs::iter& it) {
    ParticleInteractionState& pick = it.world().get_mut<ParticleInteractionState>();
    // liveness guards keep this system no-op unless actively dragging
    if (!pick.dragging || !pick.pressed.is_alive()) return;

    if (pick.drag_added_pin) {
        detail::release_drag_pin(pick);
    }

    const Camera3D& cam = graphics::get_raylib_camera_const();
    // live camera-facing drag plane while dragging
    detail::update_drag_plane_normal_from_camera(pick, cam);

    const Vector2 mouse = GetMousePosition();
    const Ray ray = GetMouseRay(mouse, cam);
    vec3f hit;
    if (!detail::ray_plane_intersection(ray, pick.drag_plane_point, pick.drag_plane_normal, hit)) return;

    detail::apply_virtual_spring(
        pick.pressed,
        hit.map() + pick.drag_offset.map(),
        (float)props::dt.get<Real>(),
        pick.virtual_spring_k,
        pick.virtual_spring_d
    );
}

// debug stage:
// - draws current drag plane and normal while dragging
inline void draw_drag_plane_debug(flecs::iter& it) {
    const ParticleInteractionState& pick = it.world().get<ParticleInteractionState>();
    if (!pick.dragging || !pick.pressed.is_alive()) return;

    const Eigen::Vector3f c = pick.drag_plane_point.map();

    const Camera3D& cam = graphics::get_raylib_camera_const();
    const vec3f cam_pos = vec3f(cam.position);
    Eigen::Vector3f n = c - cam_pos.map();
    float n_len2 = n.squaredNorm();
    if (n_len2 >= 1e-12f) {
        n /= std::sqrt(n_len2);
    } else {
        n = pick.drag_plane_normal.map();
        n_len2 = n.squaredNorm();
        if (n_len2 < 1e-12f) return;
        n /= std::sqrt(n_len2);
    }

    const Eigen::Vector3f axis = (std::fabs(n.y()) < 0.9f)
        ? Eigen::Vector3f(0.0f, 1.0f, 0.0f)
        : Eigen::Vector3f(1.0f, 0.0f, 0.0f);
    const Eigen::Vector3f t0 = n.cross(axis).normalized();
    const Eigen::Vector3f t1 = n.cross(t0).normalized();

    constexpr float half = 1.5f;
    vec3f p0, p1, p2, p3, tip;
    p0.map() = c + (-t0 - t1) * half;
    p1.map() = c + ( t0 - t1) * half;
    p2.map() = c + ( t0 + t1) * half;
    p3.map() = c + (-t0 + t1) * half;
    tip.map() = c + n * 0.7f;
    const Color plane_color = ColorAlpha(ORANGE, 0.75f);
    DrawLine3D(p0, p1, plane_color);
    DrawLine3D(p1, p2, plane_color);
    DrawLine3D(p2, p3, plane_color);
    DrawLine3D(p3, p0, plane_color);
    DrawLine3D(pick.drag_plane_point, tip, RED);
}

} // namespace interaction

} // namespace systems
