#include "Eigen/Dense"
#include "Eigen/Geometry"
#include "flecs.h"
#include "graphics_module.h"
#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "hw2aux.h"

const int CANVAS_WIDTH = 800;
const int CANVAS_HEIGHT = 600;
const float EPSILON = 1e-6f;

// utils

struct Position {
    Eigen::Vector3f value;
};

struct Velocity {
    Eigen::Vector3f value;
};

struct ParticleProperties {
    float radius;
    float mass;
    float friction;
    float restitution;
    bool is_rest;

    // float mass;
    float invmass;

    float size_decay;
    float color_decay;
    float velocity_decay;
    float lifespan = 3;

    // bool is_rest;
};

struct SimulationResources {
    Model sphere_model;
    Model plane_model;
    Model cube_model;
    Texture2D texture;
};

// Tag components
struct Particle { };
struct Plane { };

struct particle_lifespan {
    particle_lifespan() {
        t = 0;
    }
    float t;
};

struct PlaneProperties {
    Eigen::Vector3f normal;
    float friction;
    float restitution;
    int collision_count;
    bool is_visible;
    bool is_audible;
};

struct Scene {
    Eigen::Vector3f gravity;
    float air_resistance;
    float simulation_time_step;
    float accumulated_time;

    double frame_time;
    double sim_time;

    float min_frequency = 30;
    float max_frequency = 10000;
    float frequency_slider;
};

struct TriangleInfo {
    size_t meshId;
    size_t triangleIndex;
};

// generator
struct DiskGenerator {
    Eigen::Vector3f center;
    Eigen::Vector3f normal;
    float radius;
    float particleRate;
    float accumulatedParticles;
};

float uniformRandom(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

Eigen::Vector3f randomPointOnDisk(const DiskGenerator& generator) {
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

void generateParticles(flecs::iter& it, size_t, DiskGenerator& generator) {
    float deltaTime = it.delta_time();
    generator.accumulatedParticles += generator.particleRate * deltaTime;
    int particlesToGenerate = static_cast<int>(generator.accumulatedParticles);
    generator.accumulatedParticles -= particlesToGenerate;

    for (int i = 0; i < particlesToGenerate; ++i) {
        Eigen::Vector3f position = randomPointOnDisk(generator);
        Eigen::Vector3f velocity =
            generator.normal * 5 + Eigen::Vector3f::Random().normalized() * 0.1f; // Add some randomness

        it.world()
            .entity()
            .add<Particle>()
            .set<Position>({position})
            .set<Velocity>({velocity})
            .add<particle_lifespan>()
            .set<ParticleProperties>({0.05f, 1.0f, 0.3f, 0.7f, false});
    }
}
// generator

// collision
struct SpatialHash {
    float cellSize;
    Eigen::Vector3f minBounds;
    std::unordered_map<size_t, std::vector<TriangleInfo>> grid;

    SpatialHash(float cellSize, const Eigen::Vector3f& minBounds)
        : cellSize(cellSize)
        , minBounds(minBounds) { }

    size_t hash(const Eigen::Vector3f& position) const {
        Eigen::Vector3i indices = ((position - minBounds) / cellSize).cast<int>();
        return (size_t)(indices[0] * 73856093 ^ indices[1] * 19349663 ^ indices[2] * 83492791);
    }

    void insert(const Eigen::Vector3f& v0,
                const Eigen::Vector3f& v1,
                const Eigen::Vector3f& v2,
                size_t meshId,
                size_t triangleIndex) {
        Eigen::Vector3f min = v0.cwiseMin(v1).cwiseMin(v2);
        Eigen::Vector3f max = v0.cwiseMax(v1).cwiseMax(v2);

        Eigen::Vector3i minIndices = ((min - minBounds) / cellSize).cast<int>();
        Eigen::Vector3i maxIndices = ((max - minBounds) / cellSize).cast<int>();

        for (int x = minIndices[0]; x <= maxIndices[0]; ++x) {
            for (int y = minIndices[1]; y <= maxIndices[1]; ++y) {
                for (int z = minIndices[2]; z <= maxIndices[2]; ++z) {
                    size_t key = hash(Eigen::Vector3f(x, y, z) * cellSize + minBounds);
                    grid[key].push_back({meshId, triangleIndex});
                }
            }
        }
    }

    std::vector<TriangleInfo> getTriangles(const Eigen::Vector3f& position) const {
        auto it = grid.find(hash(position));
        return (it != grid.end()) ? it->second : std::vector<TriangleInfo>();
    }

    void clear() {
        grid.clear();
    }
};

SpatialHash globalSpatialHash(0.5f, Eigen::Vector3f(-10, -10, -10));

void UpdateSpatialHashSystem(flecs::iter& it) {
    globalSpatialHash.clear();
    size_t meshId = 0;

    auto body_query = it.world().query<const body, const trimesh>();
    body_query.each([&](flecs::entity e, const body& b, const trimesh& mesh) {
        for (size_t i = 0; i < mesh.tri_ids.size(); i += 3) {
            Eigen::Vector3f v0 = b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i]])) + b.position;
            Eigen::Vector3f v1 =
                b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i + 1]])) + b.position;
            Eigen::Vector3f v2 =
                b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i + 2]])) + b.position;

            globalSpatialHash.insert(v0, v1, v2, meshId, i / 3);
        }
        meshId++;
    });
}

void buildSpatialHash(const trimesh& mesh, const body& b, SpatialHash& spatialHash, size_t meshId) {
    for (size_t i = 0; i < mesh.tri_ids.size(); i += 3) {
        Eigen::Vector3f v0 = b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i]])) + b.position;
        Eigen::Vector3f v1 = b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i + 1]])) + b.position;
        Eigen::Vector3f v2 = b.orientation * (b.scale.cwiseProduct(mesh.vertices[mesh.tri_ids[i + 2]])) + b.position;

        spatialHash.insert(v0, v1, v2, meshId, i / 3);
    }
}

Eigen::Vector3f barycentric_coordinates(
    const Eigen::Vector3f& p, const Eigen::Vector3f& a, const Eigen::Vector3f& b, const Eigen::Vector3f& c) {
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

void handle_particle_triangle_collision(
    Position& p,
    Velocity& v,
    ParticleProperties& props,
    const Eigen::Vector3f& v0,
    const Eigen::Vector3f& v1,
    const Eigen::Vector3f& v2,
    float dt) {

    Eigen::Vector3f normal = (v1 - v0).cross(v2 - v0).normalized();

    float d_n = (p.value - v0).dot(normal);
    float d_n1 = (p.value + v.value * dt - v0).dot(normal);

    if (d_n * d_n1 < 0) { // Collision detected
        // Calculate collision point
        float t = d_n / (d_n - d_n1);
        Eigen::Vector3f collision_point = p.value + t * v.value * dt;

        // check if collision point is inside the triangle
        Eigen::Vector3f barycentric = barycentric_coordinates(collision_point, v0, v1, v2);
        if (barycentric.x() >= 0 && barycentric.y() >= 0 && barycentric.z() >= 0 && barycentric.x() <= 1 &&
            barycentric.y() <= 1 && barycentric.z() <= 1) {

            // Update position
            p.value = collision_point + (1 + props.restitution) * std::abs(d_n1) * normal;

            // Update velocity
            Eigen::Vector3f v_n = v.value.dot(normal) * normal;
            Eigen::Vector3f v_t = v.value - v_n;
            v.value = -props.restitution * v_n + (1 - props.friction) * v_t;
        }
    }
}

void ParticleTriangleCollisionSystem(flecs::iter& it, size_t, Position& p, Velocity& v, ParticleProperties& props) {
    auto body_query = it.world().query<const body, const trimesh>();
    std::vector<std::pair<const body*, const trimesh*>> bodyMeshPairs;
    body_query.each([&](flecs::entity e, const body& b, const trimesh& mesh) {
        bodyMeshPairs.emplace_back(&b, &mesh);
    });

    // for (auto i : it) {
    Eigen::Vector3f start_pos = p.value;
    Eigen::Vector3f end_pos = start_pos + v.value * it.delta_time();

    Eigen::Vector3f min_pos = start_pos.cwiseMin(end_pos);
    Eigen::Vector3f max_pos = start_pos.cwiseMax(end_pos);

    std::vector<TriangleInfo> potentialTriangles;
    for (float x = min_pos.x(); x <= max_pos.x(); x += globalSpatialHash.cellSize) {
        for (float y = min_pos.y(); y <= max_pos.y(); y += globalSpatialHash.cellSize) {
            for (float z = min_pos.z(); z <= max_pos.z(); z += globalSpatialHash.cellSize) {
                Eigen::Vector3f cellPos(x, y, z);
                auto triangles = globalSpatialHash.getTriangles(cellPos);
                potentialTriangles.insert(potentialTriangles.end(), triangles.begin(), triangles.end());
            }
        }
    }

    for (const auto& triangleInfo : potentialTriangles) {
        if (triangleInfo.meshId >= bodyMeshPairs.size()) {
            continue; // skip if meshId is out of range
        }
        const body* b = bodyMeshPairs[triangleInfo.meshId].first;
        const trimesh* mesh = bodyMeshPairs[triangleInfo.meshId].second;
        size_t j = triangleInfo.triangleIndex * 3;
        if (j + 2 >= mesh->tri_ids.size()) {
            continue; // skip if triangle index is out of range
        }
        Eigen::Vector3f v0 = b->orientation * (b->scale.cwiseProduct(mesh->vertices[mesh->tri_ids[j]])) + b->position;
        Eigen::Vector3f v1 =
            b->orientation * (b->scale.cwiseProduct(mesh->vertices[mesh->tri_ids[j + 1]])) + b->position;
        Eigen::Vector3f v2 =
            b->orientation * (b->scale.cwiseProduct(mesh->vertices[mesh->tri_ids[j + 2]])) + b->position;

        handle_particle_triangle_collision(p, v, props, v0, v1, v2, it.delta_time());
    }
    // }
}

void register_particle_triangle_collision_system(flecs::world& ecs) {
    ecs.system<Position, Velocity, ParticleProperties>("ParticleTriangleCollision")
        .kind(flecs::OnUpdate)
        .each(ParticleTriangleCollisionSystem);
}
///////////////// collision specifics

void draw_particles(flecs::iter& it, size_t index, const Position& p, const ParticleProperties& props) {
    auto res = it.world().get<SimulationResources>();
    DrawModel(res->sphere_model, e2r(p.value), props.radius, SKYBLUE);

    // DrawPoint3D(e2r(p.value), RED);

    // DrawTriangle3D(e2r({0, 0, 0}), e2r({0, 1, 0}), e2r({1, 0, 0}), GREEN);
}

void handle_keypress_system(flecs::iter& it) {
    const auto& camera =
        graphics::graphics::activeCamera == 1 ? graphics::graphics::camera1 : graphics::graphics::camera2;
    const auto origin = r2e(camera.position);
    const auto target = r2e(camera.target);
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

int main() {

    std::cout << "Hi from " << __FILE__ << std::endl;

    flecs::world ecs;

    ecs.component<Position>();
    ecs.component<Velocity>();
    ecs.component<ParticleProperties>();
    ecs.component<PlaneProperties>();
    ecs.component<Particle>();
    ecs.component<Plane>();

    ecs.set<Scene>({
        -9.8f * Eigen::Vector3f::UnitY(), // gravity
        0.5f, // air_resistance
        1.0f / 100.0f,
        0.0f // accumulated_time
    });

    // Import and initialize graphics
    ecs.import <graphics::graphics>();
    graphics::graphics::init_window(ecs, CANVAS_WIDTH, CANVAS_HEIGHT, "Particle Simulator");

    // Initialize simulation resources, must be called after graphics::init
    SimulationResources res;
    res.sphere_model = LoadModelFromMesh(GenMeshSphere(1, 4, 4));
    ecs.set<SimulationResources>(res); // register res

    // Create entities
    auto add_particle = [&](const Eigen::Vector3f& pos, const Eigen::Vector3f& vel) {
        ecs.entity()
            .add<Particle>()
            .set<Position>({pos})
            .set<Velocity>({vel})
            .add<particle_lifespan>()
            .set<ParticleProperties>({0.3f, 1.0f, 0.3f, 0.7f, false});
    };

    add_particle({1, 5, 0}, {5, -10, 10});

    // create generator
    DiskGenerator diskGenerator{
        Eigen::Vector3f(0, 5, 0), // center
        Eigen::Vector3f(0, -1, 0), // normal (upwards)
        0.5f, // radius
        2000.0f, // particleRate (particles per second)
        0.0f // accumulatedParticles
    };

    ecs.system<DiskGenerator>("UpdateDiskGenerator").kind(flecs::PreUpdate).each([](DiskGenerator& generator) {
        if (graphics::graphics::activeCamera == 2) {
            const auto& camera = graphics::graphics::camera2;
            generator.center = r2e(camera.position);
            generator.normal = r2e(camera.target) - r2e(camera.position);
            generator.normal.normalize();
        }
    });
    ecs.system<DiskGenerator>("GenerateParticles").kind(flecs::PreUpdate).each(generateParticles);

    ecs.entity("DiskGenerator").set<DiskGenerator>(diskGenerator);

    // from hw2aux.h
    install_bodies(ecs);

    ecs.system("HandleKeypress").kind(flecs::PreUpdate).run(handle_keypress_system);

    ecs.system("UpdateSpatialHash").kind(flecs::PreUpdate).run(UpdateSpatialHashSystem);

    auto progress_particle_system =
        ecs.system<Position, Velocity, particle_lifespan, const ParticleProperties>().kind(0).each(
            [](flecs::iter& it,
               size_t index,
               Position& p,
               Velocity& v,
               particle_lifespan& pl,
               const ParticleProperties& props) {
                auto scene = it.world().get<Scene>();
                if (!props.is_rest) {
                    v.value +=
                        it.delta_time() * (1.0f / props.mass) * (scene->gravity + scene->air_resistance * -v.value);
                    p.value += it.delta_time() * v.value;
                }

                pl.t += it.delta_time();
                if (pl.t > props.lifespan) {
                    it.entity(index).destruct();
                }
            });

    ecs.system("physics::step").kind(flecs::OnUpdate).run([&](flecs::iter& it) {
        auto scene = it.world().get_mut<Scene>();
        scene->accumulated_time += it.delta_time();

        auto start_time = std::chrono::high_resolution_clock::now();

        progress_particle_system.run(it.delta_time()); // careful with timestepping here
        // particle_triangle_collision_system.run(it.delta_time());
        // handle_collisions_system.run(it.delta_time());

        auto end_time = std::chrono::high_resolution_clock::now();
        scene->sim_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    });

    auto particle_triangle_collision_system =
        ecs.system<Position, Velocity, ParticleProperties>("ParticleTriangleCollision")
            .kind(flecs::OnUpdate)
            .each(ParticleTriangleCollisionSystem);

    auto progress_body_system = ecs.system<body>().each([](flecs::iter& it, size_t, body& b) {
        Eigen::Quaternionf small_rotation(Eigen::AngleAxisf(0.01f, Eigen::Vector3f::UnitY()));

        b.orientation *= small_rotation;
    });

    ecs.system("graphics::begin").kind(flecs::PostUpdate).run([](flecs::iter& it) {
        it.world().get_mut<Scene>()->frame_time = 0.0;
        BeginDrawing();
    });

    ecs.system("graphics::clear").kind(flecs::PostUpdate).run([](flecs::iter& it) { ClearBackground(RAYWHITE); });

    ecs.system("graphics::begin_mode_3d").kind(flecs::PostUpdate).run([](flecs::iter& it) {
        BeginMode3D(graphics::graphics::activeCamera == 1 ? graphics::graphics::camera1 : graphics::graphics::camera2);
        DrawGrid(12, 10.0f / 12);
    });

    ecs.system<const Position, const ParticleProperties>("DrawParticles").kind(flecs::PostUpdate).each(draw_particles);

    ecs.system<const body, const trimesh>("render_body")
        .kind(flecs::PostUpdate)
        .each([](const body& b, const trimesh& t) {
            Eigen::Matrix3f rotMat = b.orientation.toRotationMatrix();

            Vector3 position = {b.position.x(), b.position.y(), b.position.z()};
            Vector3 scale = {b.scale.x(), b.scale.y(), b.scale.z()};

            Eigen::AngleAxisf aa(b.orientation);

            Vector3 rotationAxis = {aa.axis().x(), aa.axis().y(), aa.axis().z()};

            float rotationAngle = aa.angle() * RAD2DEG;

            DrawModelEx(t.model, position, rotationAxis, rotationAngle, scale, WHITE);
            DrawModelWiresEx(t.model, position, rotationAxis, rotationAngle, scale, BLUE);

            Eigen::Vector3f origin = b.position;
            Eigen::Vector3f xAxis = origin + rotMat.col(0);
            Eigen::Vector3f yAxis = origin + rotMat.col(1);
            Eigen::Vector3f zAxis = origin + rotMat.col(2);

            DrawLine3D(e2r(origin), e2r(xAxis), RED);
            DrawLine3D(e2r(origin), e2r(yAxis), GREEN);
            DrawLine3D(e2r(origin), e2r(zAxis), BLUE);

            DrawSphere(e2r(origin), 0.1f, RED);
        });

    ecs.system<DiskGenerator>("render_generator").kind(flecs::PostUpdate).each([](const DiskGenerator& generator) {
        Vector3 center = e2r(generator.center);
        Vector3 normal = e2r(generator.normal);
        // Vector3 rotationAxis = {normal.x, -normal.y, 0}; // Perpendicular to normal
        // float rotationAngle = std::acos(normal.z) * RAD2DEG;

        // DrawCircle3D(center, generator.radius, rotationAxis, rotationAngle, SKYBLUE);

        // Draw the normal vector
        Vector3 normalEnd = {center.x + normal.x * generator.radius,
                             center.y + normal.y * generator.radius,
                             center.z + normal.z * generator.radius};
        DrawLine3D(center, normalEnd, RED);

        DrawSphere(center, 0.1f, YELLOW);

        float cylinderLength = generator.radius * 0.2f;
        Vector3 startPos = {center.x - normal.x * cylinderLength * 0.5f,
                            center.y - normal.y * cylinderLength * 0.5f,
                            center.z - normal.z * cylinderLength * 0.5f};
        Vector3 endPos = {center.x + normal.x * cylinderLength * 0.5f,
                          center.y + normal.y * cylinderLength * 0.5f,
                          center.z + normal.z * cylinderLength * 0.5f};

        auto cylindercolor = DARKBLUE;
        // cylindercolor.a = 50;
        DrawCylinderWiresEx(startPos, endPos, generator.radius, generator.radius, 32, DARKBLUE);
    });

    ecs.system<const aabb, const body>("RenderAABB")
        .kind(flecs::PostUpdate)
        .each([](flecs::iter& it, size_t index, const aabb& a, const body& b) {
            auto scene = it.world().get_mut<Scene>();

            Vector3 min = e2r(a.min);
            Vector3 max = e2r(a.max);

            static auto prev_time = std::chrono::high_resolution_clock::now();
            auto now = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float>(now - prev_time).count();
            prev_time = now;

            float colorValue = (std::sin(scene->accumulated_time * 10) + 1.0f) / 2.0f; // -1~1 -> 0~1
            Color color = {(unsigned char)(SKYBLUE.r + colorValue * (DARKBLUE.r - SKYBLUE.r)),
                           (unsigned char)(SKYBLUE.g + colorValue * (DARKBLUE.g - SKYBLUE.g)),
                           (unsigned char)(SKYBLUE.b + colorValue * (DARKBLUE.b - SKYBLUE.b)),
                           255};

            DrawBoundingBox({min, max}, color);
        });

    // renderable + particle
    // renderable + model

    ecs.system("graphics::end_mode_3d").kind(flecs::OnStore).run([](flecs::iter& it) {
        EndMode3D();
        DrawFPS(20, 20);
    });

    ecs.system("DisplayTimingInfo").kind(flecs::OnStore).run([](flecs::iter& it) {
        auto scene = it.world().get_mut<Scene>();
        scene->frame_time = GetFrameTime() * 1000.0; // to milliseconds

        auto particle_query = it.world().query<const Particle>();
        int total_particles = particle_query.count();
        // auto scene = it.world().get<Scene>();
        DrawText(TextFormat("Simulation Took: %.2f ms", scene->sim_time), 20, 60, 20, LIME);
        DrawText(TextFormat("Frame Took: %.2f ms", scene->frame_time), 20, 80, 20, LIME);
        DrawText(TextFormat("Total Particles: %d", total_particles), 20, 120, 20, LIME);
        // DrawText(TextFormat("Simulation Timestep: %.7f ms", scene->simulation_time_step),
        // 		 20,
        // 		 120,
        // 		 20,
        // 		 LIME);
    });

    ecs.system("graphics::end").kind(flecs::OnStore).run([](flecs::iter& it) { EndDrawing(); });

    // main loop
    graphics::graphics::run_main_loop(ecs, []() {
        // callback if needed
    });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}