#pragma once

#include "components.h"
#include "common/conversions.h"
#include "graphics.h"
#include "flecs.h"
#include "raylib.h"
#include <array>
#include <cmath>

namespace particle_plane {
namespace systems {

// Utility functions
inline float LinearToLog(float value, float min, float max) {
    float minLog = log10(min);
    float maxLog = log10(max);
    return (log10(value) - minLog) / (maxLog - minLog);
}

inline float LogToLinear(float value, float min, float max) {
    float minLog = log10(min);
    float maxLog = log10(max);
    return pow(10, value * (maxLog - minLog) + minLog);
}

// Collision systems
inline void handle_particle_plane_collisions(
    flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
    const auto& scene = it.world().get<Scene>();
    auto& audio = it.world().get_mut<AudioState>();

    it.world().each([&](flecs::entity e, const Position& plane_pos, const PlaneProperties& plane_props) {
        float penetration = (p.value - plane_pos.value).dot(plane_props.normal) - props.radius;

        if (penetration < 0) {
            Eigen::Vector3f v_n = v.value.dot(plane_props.normal) * plane_props.normal;
            Eigen::Vector3f v_t = v.value - v_n;

            v_n = -props.restitution * v_n;

            float friction_magnitude = std::min(plane_props.friction * v_n.norm(), v_t.norm());
            if (v_t.norm() > 0) {
                v_t -= friction_magnitude * v_t.normalized();
            }

            v.value = v_n + v_t;
            p.value -= (penetration - COLLISION_EPSILON) * plane_props.normal;

            auto& mutable_plane = e.get_mut<PlaneProperties>();
            mutable_plane.collision_count++;

            if (mutable_plane.is_audible) {
                audio.collision_sfx_count++;
                int idx = audio.collision_sfx_count % 9;
                if (audio.sequence[idx] != nullptr) {
                    PlaySound(*audio.sequence[idx]);
                }
            }
        }
    });
}

inline void handle_particle_particle_collisions(
    flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
    auto world = it.world();

    world.each([&](flecs::entity other_entity, const Position& other_p, const ParticleProperties& other_props) {
        if (it.entity(index) == other_entity)
            return;

        Eigen::Vector3f delta = p.value - other_p.value;
        float distance = delta.norm();
        float min_distance = props.radius + other_props.radius;

        if (distance < min_distance && distance > 0) {
            Eigen::Vector3f normal = delta.normalized();

            const auto& other_v = other_entity.get<Velocity>();
            Eigen::Vector3f rel_velocity = v.value - other_v.value;

            float impulse =
                -(1 + props.restitution) * rel_velocity.dot(normal) / ((1 / props.mass) + (1 / other_props.mass));

            v.value += impulse / props.mass * normal;
            Eigen::Vector3f new_other_v = other_v.value - impulse / other_props.mass * normal;

            float overlap = min_distance - distance;
            Eigen::Vector3f separation = normal * overlap * 0.5f;
            p.value += separation;
            Eigen::Vector3f new_other_p = other_p.value - separation;

            other_entity.set<Velocity>({new_other_v});
            other_entity.set<Position>({new_other_p});
        }
    });
}

inline void check_particle_rest_state(flecs::iter& it, size_t index, Position& p, Velocity& v, ParticleProperties& props) {
    const auto& scene = it.world().get<Scene>();

    if (v.value.norm() < RESTING_VELOCITY_THRESHOLD) {
        bool at_rest = false;
        it.world().each([&](const Position& plane_pos, const PlaneProperties& plane_props) {
            float d = (p.value - plane_pos.value).dot(plane_props.normal) - props.radius;
            if (std::abs(d) < RESTING_DISTANCE_THRESHOLD) {
                Eigen::Vector3f total_force = scene.gravity * props.mass;
                float normal_force = total_force.dot(plane_props.normal);
                if (normal_force < -RESTING_FORCE_THRESHOLD) {
                    Eigen::Vector3f tangential_force = total_force - normal_force * plane_props.normal;
                    if (tangential_force.norm() <= plane_props.friction * std::abs(normal_force)) {
                        at_rest = true;
                    }
                }
            }
        });
        props.is_rest = at_rest;
    } else {
        props.is_rest = false;
    }
}

// Rendering systems
inline void draw_particles(flecs::iter& it, size_t index, const Position& p, const ParticleProperties& props) {
    const auto& res = it.world().get<SimulationResources>();
    DrawModel(res.sphere_model, toRay(p.value), props.radius, props.is_rest ? GRAY : WHITE);
}

inline void draw_planes(flecs::iter& it, size_t index, const Position& p, const PlaneProperties& props) {
    if (!props.is_visible) return;

    const auto& res = it.world().get<SimulationResources>();
    const std::array<Color, 4> PLANE_COLORS = {
        Color{66, 133, 244, 255}, Color{234, 67, 53, 255}, Color{251, 188, 5, 255}, Color{52, 168, 83, 255}};

    Eigen::Vector3f position = p.value;
    Eigen::Vector3f normal = props.normal.normalized();

    Eigen::Vector3f up = Eigen::Vector3f::UnitY();
    if (normal.dot(up) > 0.99f || normal.dot(up) < -0.99f) {
        up = Eigen::Vector3f::UnitZ();
    }
    Eigen::Vector3f right = up.cross(normal).normalized();
    up = normal.cross(right).normalized();

    Color color = props.collision_count ? PLANE_COLORS[props.collision_count % PLANE_COLORS.size()] : WHITE;

    Eigen::Vector3f scale = Eigen::Vector3f::Ones() * 5;
    Eigen::Vector3f normal_end = position + normal;

    DrawModelEx(res.cube_model,
                toRay(position),
                toRay(right),
                std::acos(Eigen::Vector3f::UnitY().dot(normal)) * RAD2DEG,
                toRay(scale),
                color);

    DrawLine3D(toRay(position), toRay(normal_end), RED);
    DrawSphere(toRay(normal_end), 0.1f, RED);
}

// Input systems
inline void reset_particles(flecs::iter& it) {
    const auto& camera = graphics::get_camera_const();
    auto& audio = it.world().get_mut<AudioState>();

    auto add_particle = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& vel) {
        it.world().entity().add<Particle>().set<Position>({pos}).set<Velocity>({vel}).set<ParticleProperties>(
            {0.5f, 1.0f, 0.3f, 0.7f, false});
    };

    auto add_plane = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& normal, bool visible = true, bool audible = true) {
        it.world().entity().add<Plane>().set<Position>({pos}).set<PlaneProperties>({normal, 0.3f, 0.7f, 0, visible, audible});
    };

    if (IsKeyPressed(KEY_R)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
    }

    if (IsKeyPressed(KEY_DELETE)) {
        PlaySound(audio.notes[0]);
    }

    if (IsKeyPressed(KEY_ONE)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
        it.world().entity().add<Particle>().set<Position>({{0, 5, 0}}).set<Velocity>({{10, 0, 0}})
            .set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
    }

    if (IsKeyPressed(KEY_TWO)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
        it.world().entity().add<Particle>().set<Position>({{0, 5, 0}}).set<Velocity>({{10, 0, 10}})
            .set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
    }

    if (IsKeyPressed(KEY_THREE)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
        it.world().entity().add<Particle>().set<Position>({{0, 5, 0}}).set<Velocity>({{0, 0, 10}})
            .set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
    }

    if (IsKeyPressed(KEY_FOUR)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
        it.world().entity().add<Particle>().set<Position>({{0, 5, 0}}).set<Velocity>({{-10, 0, 10}})
            .set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
    }

    if (IsKeyPressed(KEY_LEFT_BRACKET)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
        it.world().each([&](flecs::entity e, const Plane&) { e.destruct(); });

        add_particle({1, 5, 0}, {10, 0, 10});
        add_plane({5, 5, 0}, {-1, 0, 0});
        add_plane({-5, 5, 0}, {1, 0, 0});
        add_plane({0, 0, 0}, {0, 1, 0}, false, false);
        add_plane({0, 10, 0}, {0, -1, 0}, false);
        add_plane({0, 5, 5}, {0, 0, -1});
        add_plane({0, 5, -5}, {0, 0, 1});
    }

    if (IsKeyPressed(KEY_RIGHT_BRACKET)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
        it.world().each([&](flecs::entity e, const Plane&) { e.destruct(); });

        add_particle({1, 5, 0}, {10, 0, 10});
        add_plane({-3, 0, 0}, {1, 1, 0}, true);
        add_plane({3, 0, 0}, {-1, 1, 0}, true);
    }
}

inline void create_particle_on_input(flecs::iter& it) {
    const auto& camera = graphics::get_camera_const();

    if (IsKeyPressed(KEY_SPACE)) {
        const auto origin = toEig(camera.position);
        const auto target = toEig(camera.target);
        const auto velocity = (target - origin + Eigen::Vector3f::UnitY() * 2.0f).normalized() * 10.0f;

        it.world().entity().add<Particle>().set<Position>({origin}).set<Velocity>({velocity})
            .set<ParticleProperties>({0.5f, 1.0f, 0.3f, 0.7f, false});
    }
}

} // namespace systems
} // namespace particle_plane
