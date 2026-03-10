#pragma once

#include "components.h"
#include "common/conversions.h"
#include "common/geometry.h"
#include "common/random.h"
#include "graphics.h"
#include "flecs.h"
#include "raylib.h"

namespace particle_mesh {
namespace systems {

// Global spatial hash instance
inline TriangleSpatialHash& get_spatial_hash() {
    static TriangleSpatialHash hash(0.5f);
    return hash;
}

// Utility: Random point on disk
inline Eigen::Vector3f randomPointOnDisk(const DiskGenerator& generator) {
    float r = std::sqrt(uniformRandom(0, 1)) * generator.radius;
    float theta = uniformRandom(0, 2 * PI);

    Eigen::Vector3f localPoint(r * std::cos(theta), r * std::sin(theta), 0);

    Eigen::Vector3f u = generator.normal.unitOrthogonal();
    Eigen::Vector3f v = generator.normal.cross(u);

    Eigen::Matrix3f rotation;
    rotation.col(0) = u;
    rotation.col(1) = v;
    rotation.col(2) = generator.normal;

    return generator.center + rotation * localPoint;
}

// Utility: Barycentric coordinates
inline Eigen::Vector3f barycentric_coordinates(const Eigen::Vector3f& p, const Eigen::Vector3f& a,
                                                const Eigen::Vector3f& b, const Eigen::Vector3f& c) {
    Eigen::Vector3f v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = v0.dot(v0);
    float d01 = v0.dot(v1);
    float d11 = v1.dot(v1);
    float d20 = v2.dot(v0);
    float d21 = v2.dot(v1);
    float denom = d00 * d11 - d01 * d01;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return Eigen::Vector3f(u, v, w);
}

// Generate particles from disk generator
inline void generateParticles(flecs::iter& it, size_t, DiskGenerator& generator) {
    float deltaTime = it.delta_time();
    generator.accumulatedParticles += generator.particleRate * deltaTime;
    int particlesToGenerate = static_cast<int>(generator.accumulatedParticles);
    generator.accumulatedParticles -= particlesToGenerate;

    for (int i = 0; i < particlesToGenerate; ++i) {
        Eigen::Vector3f position = randomPointOnDisk(generator);
        Eigen::Vector3f velocity =
            generator.normal * 5 + Eigen::Vector3f::Random().normalized() * 0.1f;

        it.world()
            .entity()
            .add<Particle>()
            .set<Position>({position})
            .set<Velocity>({velocity})
            .add<particle_lifespan>()
            .set<ParticleProperties>({0.05f, 1.0f, 0.3f, 0.7f, false});
    }
}

// Update spatial hash from triangle meshes
inline void update_spatial_hash(flecs::iter& it) {
    auto& hash = get_spatial_hash();
    hash.clear();
    size_t meshId = 0;

    auto body_query = it.world().query<const body, const trimesh>();
    body_query.each([&](flecs::entity e, const body& b, const trimesh& mesh) {
        for (size_t i = 0; i < mesh.tri_ids.size(); i += 3) {
            Eigen::Vector3f v0 = b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i]])) + b.position;
            Eigen::Vector3f v1 = b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i + 1]])) + b.position;
            Eigen::Vector3f v2 = b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i + 2]])) + b.position;
            hash.insert(v0, v1, v2, meshId, i / 3);
        }
        meshId++;
    });
}

// Handle particle-triangle collision
inline void handle_particle_triangle_collision(Position& p, Velocity& v, ParticleProperties& props,
                                               const Eigen::Vector3f& v0, const Eigen::Vector3f& v1,
                                               const Eigen::Vector3f& v2, float dt) {
    Eigen::Vector3f normal = (v1 - v0).cross(v2 - v0).normalized();
    float d_n = (p.value - v0).dot(normal);
    float d_n1 = (p.value + v.value * dt - v0).dot(normal);

    if (d_n * d_n1 < 0) {
        float t = d_n / (d_n - d_n1);
        Eigen::Vector3f collision_point = p.value + t * v.value * dt;

        Eigen::Vector3f barycentric = barycentric_coordinates(collision_point, v0, v1, v2);
        if (barycentric.x() >= 0 && barycentric.y() >= 0 && barycentric.z() >= 0 &&
            barycentric.x() <= 1 && barycentric.y() <= 1 && barycentric.z() <= 1) {
            p.value = collision_point + (1 + props.restitution) * std::abs(d_n1) * normal;
            Eigen::Vector3f v_n = v.value.dot(normal) * normal;
            Eigen::Vector3f v_t = v.value - v_n;
            v.value = -props.restitution * v_n + (1 - props.friction) * v_t;
        }
    }
}

// Particle-triangle collision system
inline void particle_triangle_collision(flecs::iter& it, size_t, Position& p, Velocity& v,
                                        ParticleProperties& props) {
    auto body_query = it.world().query<const body, const trimesh>();
    std::vector<std::pair<const body*, const trimesh*>> bodyMeshPairs;
    body_query.each([&](flecs::entity e, const body& b, const trimesh& mesh) {
        bodyMeshPairs.emplace_back(&b, &mesh);
    });

    auto& hash = get_spatial_hash();
    Eigen::Vector3f start_pos = p.value;
    Eigen::Vector3f end_pos = start_pos + v.value * it.delta_time();

    auto potentialTriangles = hash.queryPath(start_pos, end_pos);

    for (const auto& triangleInfo : potentialTriangles) {
        if (triangleInfo.meshId >= bodyMeshPairs.size()) continue;
        const body* b = bodyMeshPairs[triangleInfo.meshId].first;
        const trimesh* mesh = bodyMeshPairs[triangleInfo.meshId].second;
        size_t j = triangleInfo.triangleIndex * 3;
        if (j + 2 >= mesh->tri_ids.size()) continue;

        Eigen::Vector3f v0 = b->orientation * (b->scale.cwiseProduct(mesh->vertices[mesh->tri_ids[j]])) + b->position;
        Eigen::Vector3f v1 = b->orientation * (b->scale.cwiseProduct(mesh->vertices[mesh->tri_ids[j + 1]])) + b->position;
        Eigen::Vector3f v2 = b->orientation * (b->scale.cwiseProduct(mesh->vertices[mesh->tri_ids[j + 2]])) + b->position;

        handle_particle_triangle_collision(p, v, props, v0, v1, v2, it.delta_time());
    }
}

// Draw particles
inline void draw_particles(flecs::iter& it, size_t index, const Position& p, const ParticleProperties& props) {
    const auto& res = it.world().get<SimulationResources>();
    DrawModel(res.sphere_model, toRay(p.value), props.radius, SKYBLUE);
}

// Handle keypress
inline void handle_keypress(flecs::iter& it) {
    const auto& camera = graphics::get_camera_const();
    const auto origin = toEig(camera.position);
    const auto target = toEig(camera.target);
    const auto velocity = (target - origin + Eigen::Vector3f::UnitY() * 2.0f).normalized() * 2.0f;

    auto add_particle = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& vel) {
        it.world()
            .entity()
            .add<Particle>()
            .set<Position>({pos})
            .set<Velocity>({vel})
            .add<particle_lifespan>()
            .set<ParticleProperties>({0.3f, 1.0f, 0.3f, 0.7f, false});
    };

    if (IsKeyPressed(KEY_R)) {
        it.world().each([&](flecs::entity e, const Particle&) { e.destruct(); });
    }

    if (IsKeyDown(KEY_SPACE)) {
        add_particle(origin, velocity);
    }
}

// Render body
inline void render_body(const body& b, const trimesh& t) {
    Eigen::Matrix3f rotMat = b.orientation.toRotationMatrix();
    Eigen::AngleAxisf aa(b.orientation);
    float rotationAngle = aa.angle() * RAD2DEG;

    DrawModelEx(t.model, toRay(b.position), toRay(aa.axis()), rotationAngle, toRay(b.scale), WHITE);
    DrawModelWiresEx(t.model, toRay(b.position), toRay(aa.axis()), rotationAngle, toRay(b.scale), BLUE);

    Eigen::Vector3f origin = b.position;
    Eigen::Vector3f axis_x = origin + Eigen::Vector3f(rotMat.col(0));
    Eigen::Vector3f axis_y = origin + Eigen::Vector3f(rotMat.col(1));
    Eigen::Vector3f axis_z = origin + Eigen::Vector3f(rotMat.col(2));
    DrawLine3D(toRay(origin), toRay(axis_x), RED);
    DrawLine3D(toRay(origin), toRay(axis_y), GREEN);
    DrawLine3D(toRay(origin), toRay(axis_z), BLUE);
    DrawSphere(toRay(origin), 0.1f, RED);
}

// Render generator
inline void render_generator(const DiskGenerator& gen) {
    Eigen::Vector3f normalEnd = gen.center + gen.normal * gen.radius;
    DrawLine3D(toRay(gen.center), toRay(normalEnd), RED);
    DrawSphere(toRay(gen.center), 0.1f, YELLOW);

    float cylinderLength = gen.radius * 0.2f;
    Eigen::Vector3f startPos = gen.center - gen.normal * cylinderLength * 0.5f;
    Eigen::Vector3f endPos = gen.center + gen.normal * cylinderLength * 0.5f;
    DrawCylinderWiresEx(toRay(startPos), toRay(endPos), gen.radius, gen.radius, 32, DARKBLUE);
}

// Render AABB
inline void render_aabb(flecs::iter& it, size_t index, const aabb& a, const body& b) {
    auto& scene = it.world().get_mut<Scene>();

    float colorValue = (std::sin(scene.accumulated_time * 10) + 1.0f) / 2.0f;
    Color color = {(unsigned char)(SKYBLUE.r + colorValue * (DARKBLUE.r - SKYBLUE.r)),
                   (unsigned char)(SKYBLUE.g + colorValue * (DARKBLUE.g - SKYBLUE.g)),
                   (unsigned char)(SKYBLUE.b + colorValue * (DARKBLUE.b - SKYBLUE.b)), 255};
    DrawBoundingBox({toRay(a.min), toRay(a.max)}, color);
}

} // namespace systems
} // namespace particle_mesh
