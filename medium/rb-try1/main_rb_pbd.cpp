#include "flecs.h"

#include "AABB.h"
#include "BoundingSphereHierarchy.h"
#include "collision.hpp"
#include "geometry.hpp"
#include "rigid_body.hpp"

#include "graphics.h"
#include <iostream>

struct AABB {
    PBD::AABB aabb_;
};

struct BVH {
    PBD::PointCloudBSH bvh_;
    bool initialized = false;
    int render_depth = 0;
};

struct CollisionPair {
    flecs::entity e1;
    flecs::entity e2;
};

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.016f;
float tolerance = 0.01f;
float restitution = 0.0f;
float friction = 0.0f;

int main() {
    flecs::world ecs;
    ecs.set_target_fps(60);

    graphics::init(ecs);
    graphics::init_window(800, 800, "Physics Simulator");

    register_rb_systems(ecs);

    auto rb1 = rb_add_rigid_body(ecs, 1);
    rb1.add<Geometry>();
    rb1.add<AABB>();
    rb1.add<BVH>();

    auto rb2 = rb_add_rigid_body(ecs, 1);
    rb2.add<Geometry>();
    rb2.add<AABB>();
    rb2.add<BVH>();

    rb1.get_mut<Position>().value = {-1, 0, 0};
    rb1.get_mut<LinearVelocity>().value = {0.2, 0, 0};
    rb2.get_mut<Position>().value = {1, 0, 0};
    rb2.get_mut<LinearVelocity>().value = {-0.2, 0, 0};
    rb2.get_mut<AngularVelocity>().value = {0.0, 0.0, 0.3};

    // create raylib mesh
    Mesh mesh1 = GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb1, mesh1, 1.0f);
    init_collider_from_geometry(rb1);
    UnloadMesh(mesh1);

    Mesh mesh2 = GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb2, mesh2, 1.0f);
    init_collider_from_geometry(rb2);
    UnloadMesh(mesh2);

    // OnUpdate
    ecs.system<AABB, Geometry>("update_aabb").with<RigidBody>().each([](AABB& aabb, const Geometry& geom) {
        aabb.aabb_.m_p[0] = geom.verts.getPosition(0);
        aabb.aabb_.m_p[1] = geom.verts.getPosition(0);

        for (size_t i = 0; i < geom.verts.size(); ++i) {
            auto p = geom.verts.getPosition(i);

            if (aabb.aabb_.m_p[0][0] > p[0]) aabb.aabb_.m_p[0][0] = p[0];
            if (aabb.aabb_.m_p[0][1] > p[1]) aabb.aabb_.m_p[0][1] = p[1];
            if (aabb.aabb_.m_p[0][2] > p[2]) aabb.aabb_.m_p[0][2] = p[2];
            if (aabb.aabb_.m_p[1][0] < p[0]) aabb.aabb_.m_p[1][0] = p[0];
            if (aabb.aabb_.m_p[1][1] < p[1]) aabb.aabb_.m_p[1][1] = p[1];
            if (aabb.aabb_.m_p[1][2] < p[2]) aabb.aabb_.m_p[1][2] = p[2];
        }

        aabb.aabb_.m_p[0][0] -= tolerance;
        aabb.aabb_.m_p[0][1] -= tolerance;
        aabb.aabb_.m_p[0][2] -= tolerance;
        aabb.aabb_.m_p[1][0] += tolerance;
        aabb.aabb_.m_p[1][1] += tolerance;
        aabb.aabb_.m_p[1][2] += tolerance;
    });

    ecs.system<BVH, Geometry>("update_bvh").with<RigidBody>().each([](BVH& bvh, const Geometry& geom) {
        if (!bvh.initialized) {
            bvh.bvh_.init(geom.verts.getVertices().data(), geom.verts.size());
            bvh.bvh_.construct();
            bvh.initialized = true;
        } else {
            bvh.bvh_.update();
        }
    });

    // do pbd
    ecs.system<const Contact>().each([](flecs::iter& it, size_t, const Contact& contact) {
        auto rb1 = contact.e1;
        auto rb2 = contact.e2;

        Vector3r r1 = contact.x_world - rb1.get<Position>().value;
        Vector3r r2 = contact.closest_point_world - rb2.get<Position>().value;

        Vector3r n1 = rb1.get<Rotation>().value * contact.normal_world;
        Vector3r n2 = rb2.get<Rotation>().value * (-contact.normal_world);

        Vector3r temp1 = r1.cross(n1);
        Vector3r temp2 = r2.cross(n2);

        Real w1 = rb1.get<InverseMass>().value + (temp1.transpose() * rb1.get<WorldInverseInertia>().value * temp1);
        Real w2 = rb2.get<InverseMass>().value + (temp2.transpose() * rb2.get<WorldInverseInertia>().value * temp2);

        Real delta_lambda = -contact.dist / (w1 + w2) * 0.2f;

        {
            Vector3r p = delta_lambda * n1;
            rb1.get_mut<Position>().value += rb1.get<Rotation>().value * p * rb1.get<InverseMass>().value;

            Vector3r angular_impulse = rb1.get<Rotation>().value * rb1.get<WorldInverseInertia>().value * r1.cross(p);

            Quaternionr dq(0, angular_impulse.x(), angular_impulse.y(), angular_impulse.z());
            rb1.get_mut<Orientation>().value.coeffs() += 0.5f * (dq * rb1.get<Orientation>().value).coeffs();
            rb1.get_mut<Orientation>().value.normalize();
        }

        {
            Vector3r p = delta_lambda * n2;
            rb2.get_mut<Position>().value += rb2.get<Rotation>().value * p * rb2.get<InverseMass>().value;

            Vector3r angular_impulse = rb2.get<Rotation>().value * rb2.get<WorldInverseInertia>().value * r2.cross(p);

            Quaternionr dq(0, angular_impulse.x(), angular_impulse.y(), angular_impulse.z());
            rb2.get_mut<Orientation>().value.coeffs() += 0.5f * (dq * rb2.get<Orientation>().value).coeffs();
            rb2.get_mut<Orientation>().value.normalize();
        }
    });

    ecs.system<LinearVelocity,
               const Position,
               const OldPosition,
               AngularVelocity,
               const Orientation,
               const OldOrientation>("RbUpdateVelocities")
        .with<RigidBody>()
        .without<IsPinned>()
        .each(rb_update_velocities);

    // OnValidate
    ecs.system<const Position, const Rotation, Geometry>("update_world_mesh_rb")
        .with<RigidBody>()
        .each(geom_update_mesh_transform);

    ecs.system().with<CollisionPair>().each([](flecs::entity e) { e.destruct(); });

    auto query_all_rb = ecs.query_builder<RigidBody>().build();
    ecs.system().run([query_all_rb](flecs::iter& it) {
        query_all_rb.each([&](flecs::entity e1, RigidBody& rb1) {
            query_all_rb.each([&](flecs::entity e2, RigidBody& rb2) {
                if (e1 != e2) {
                    it.world().entity().set<CollisionPair>({e1, e2});
                }
            });
        });
    });

    ecs.system().with<Contact>().each([](flecs::entity e) { e.destruct(); });

    ecs.system<CollisionPair>().each([](flecs::iter& it, size_t, CollisionPair& pair) {
        auto rb1 = pair.e1;
        auto rb2 = pair.e2;

        const auto& aabb1 = rb1.get<AABB>();
        const auto& aabb2 = rb2.get<AABB>();

        if (PBD::AABB::intersection(aabb1.aabb_, aabb2.aabb_) == false) {
            return;
        }

        const auto& bvh1 = rb1.get<BVH>();
        const auto& bvh2 = rb2.get<BVH>();

        const auto& co1 = rb1.get<Collider>();
        const auto& co2 = rb2.get<Collider>();

        const auto& m1 = rb1.get<Mass>();
        const auto& m2 = rb2.get<Mass>();

        if ((m1.value == 0) && (m2.value == 0))
            return;

        const auto& geom1 = rb1.get<Geometry>();

        const auto& com1 = rb1.get<Position>();
        const auto& R1 = rb1.get<Rotation>();

        const auto& com2 = rb2.get<Position>();
        const auto& R2 = rb2.get<Rotation>();

        const auto& restitution1 = rb1.get<Restitution>();
        const auto& restitution2 = rb2.get<Restitution>();

        const auto& friction1 = rb2.get<Friction>();
        const auto& friction2 = rb2.get<Friction>();

        auto restitution = restitution1.value * restitution2.value;
        auto friction = friction1.value + friction2.value;

        auto predicate = [&](unsigned int node_index, unsigned int depth) {
            const PBD::BoundingSphere& bs = bvh1.bvh_.hull(node_index);
            const Vector3r& sphere_x = bs.x();
            const Vector3r sphere_x_w = R1.value * sphere_x + com1.value;

            AlignedBox3r box3f;
            box3f.extend(aabb2.aabb_.m_p[0]);
            box3f.extend(aabb2.aabb_.m_p[1]);
            const Real dist = box3f.exteriorDistance(sphere_x_w);

            if (dist < bs.r()) {
                const Vector3r x = R2.value * (sphere_x_w - com2.value) + com2.value;
                const double dist2 = co2.distance(x.template cast<double>(), tolerance);

                if (dist2 == std::numeric_limits<double>::max()) {
                    return true;
                }

                if (dist2 < bs.r()) {
                    return true;
                }
            }
            return false;
        };

        auto cb = [&](unsigned int node_index, unsigned int depth) {
            auto const& node = bvh1.bvh_.node(node_index);
            if (!node.is_leaf())
                return;

            for (auto i = node.begin; i < node.begin + node.n; ++i) {
                unsigned int vertex_id = bvh1.bvh_.entity(i);
                const Vector3r& x_w = geom1.verts.getPosition(vertex_id);
                const Vector3r x = R2.value.transpose() * (x_w - com2.value);
                Vector3r cp, n;
                Real dist;

                if (co2.collision_test(x, tolerance, cp, n, dist)) {
                    const Vector3r cp_w = R2.value * cp + com2.value;
                    const Vector3r n_w = R2.value * n;

                    it.world().entity().set<Contact>({rb1, rb2, x_w, cp_w, n_w, dist, restitution, friction});
                }
            }
        };

        bvh1.bvh_.traverse_depth_first(predicate, cb);
    });

    // OnRender
    ecs.system<const Geometry>("draw_geometry_rb")
        .with<RigidBody>()
        .kind(graphics::phase_on_render)
        .each([](const Geometry& geom) {
            DrawMesh(geom.renderable, LoadMaterialDefault(), MatrixIdentity());

            for (size_t i = 0; i < geom.verts.size(); ++i) {
                auto p = geom.verts.getPosition(i);
                DrawPoint3D({(float)p.x(), (float)p.y(), (float)p.z()}, DARKGREEN);
            }
        });

    ecs.system<const AABB>("draw_aabb").kind(graphics::phase_on_render).each([](const AABB& aabb) {
        BoundingBox bbox;
        bbox.min = {(float)aabb.aabb_.m_p[0][0], (float)aabb.aabb_.m_p[0][1], (float)aabb.aabb_.m_p[0][2]};
        bbox.max = {(float)aabb.aabb_.m_p[1][0], (float)aabb.aabb_.m_p[1][1], (float)aabb.aabb_.m_p[1][2]};

        DrawBoundingBox(bbox, BLUE);
    });

    ecs.system<const BVH>("draw_bvh")
        .kind(graphics::phase_on_render)
        .each([](const BVH& bvh) {
            if (!bvh.initialized) return;

            Color colors[] = {{255, 0, 0, 50}, {0, 255, 0, 50}, {0, 0, 255, 50}};

            auto predicate = [&](unsigned int node_index, unsigned int depth) {
                return (int)depth <= bvh.render_depth;
            };

            auto callback = [&](unsigned int node_index, unsigned int depth) {
                if (depth == bvh.render_depth) {
                    const PBD::BoundingSphere& bs = bvh.bvh_.hull(node_index);
                    Vector3r center = bs.x();
                    float radius = std::max(bs.r(), 0.05f);

                    DrawSphere({(float)center.x(), (float)center.y(), (float)center.z()}, radius, colors[depth % 3]);
                }
            };

            bvh.bvh_.traverse_depth_first(predicate, callback);
        })
        .disable();

    ecs.system<const Contact>("draw_contact").kind(graphics::phase_on_render).each([](const Contact& contact) {
        const auto& x = contact.x_world;
        const auto& cp = contact.closest_point_world;
        const auto& n = contact.normal_world;

        DrawSphere({(float)x.x(), (float)x.y(), (float)x.z()}, 0.03f, RED);
        DrawSphere({(float)cp.x(), (float)cp.y(), (float)cp.z()}, 0.03f, GREEN);

        Vector3r normal_end = cp + n * 0.2f;
        DrawLine3D({(float)cp.x(), (float)cp.y(), (float)cp.z()}, {(float)normal_end.x(), (float)normal_end.y(), (float)normal_end.z()}, BLUE);
        DrawLine3D({(float)x.x(), (float)x.y(), (float)x.z()}, {(float)cp.x(), (float)cp.y(), (float)cp.z()}, GRAY);
    });

    graphics::run_main_loop([]() { global_time += delta_time; });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
