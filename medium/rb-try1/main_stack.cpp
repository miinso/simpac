#include "flecs.h"

#include "AABB.h"
#include "BoundingSphereHierarchy.h"
#include "collision.hpp"
#include "geometry.hpp"
#include "rigid_body2.hpp"
#include "solve.hpp"

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
float delta_time = 0.01f;
float tolerance = 0.05f;

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 800, "Physics Simulator");

    auto mat1 = geom_create_geometry_material();

    auto rb1 = rb_add_rigid_body(ecs, 1);
    rb1.add<Geometry>();
    rb1.add<AABB>();
    rb1.add<BVH>();
    rb1.add<IsPinned>();
    rb1.set<Friction>({1.0f});

    auto rb2 = rb_add_rigid_body(ecs, 1);
    rb2.add<Geometry>();
    rb2.add<AABB>();
    rb2.add<BVH>();
    rb2.set<Friction>({1.0f});

    auto rb3 = rb_add_rigid_body(ecs, 1);
    rb3.add<Geometry>();
    rb3.add<AABB>();
    rb3.add<BVH>();
    rb3.set<Friction>({1.0f});

    auto rb4 = rb_add_rigid_body(ecs, 1);
    rb4.add<Geometry>();
    rb4.add<AABB>();
    rb4.add<BVH>();
    rb4.set<Friction>({1.0f});

    auto rb5 = rb_add_rigid_body(ecs, 1);
    rb5.add<Geometry>();
    rb5.add<AABB>();
    rb5.add<BVH>();
    rb5.set<Friction>({1.0f});

    Mesh mesh1 = GenMeshCube(8, 0.2, 8);
    geom_init_geometry_from_rlmesh(rb1, mesh1, 1.0f);
    init_collider_from_geometry(rb1);
    UnloadMesh(mesh1);

    Mesh mesh2 = GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb2, mesh2, 0.1f);
    init_collider_from_geometry(rb2);
    UnloadMesh(mesh2);

    Mesh mesh3 = GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb3, mesh3, 0.1f);
    init_collider_from_geometry(rb3);
    UnloadMesh(mesh3);

    Mesh mesh4 = GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb4, mesh4, 0.1f);
    init_collider_from_geometry(rb4);
    UnloadMesh(mesh4);

    Mesh mesh5 = GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb5, mesh5, 0.1f);
    init_collider_from_geometry(rb5);
    UnloadMesh(mesh5);

    rb2.get_mut<Position>().value = {0, 1, 0};
    rb3.get_mut<Position>().value = {0, 2.5, 0};
    rb4.get_mut<Position>().value = {0, 3.5, 0};
    rb5.get_mut<Position>().value = {0, 4.5, 0};

    // PreUpdate
    ecs.system("clear_contacts").with<Contact>().each([](flecs::entity e) { e.destruct(); });
    ecs.system<LinearForce, AngularForce, ConstraintForce, ConstraintTorque>("reset_forces")
        .with<RigidBody>()
        .each(rb_reset_forces);
    ecs.system<LinearForce, const Mass>("apply_gravity").with<RigidBody>().without<IsPinned>().each(rb_apply_gravity);
    ecs.system<Rotation, WorldInertia, WorldInverseInertia, const LocalInertia, const Orientation>("update_inertia")
        .with<RigidBody>()
        .each(rb_update_world_inertia);

    // OnUpdate
    ecs.system<const Position, const Rotation, Geometry>("update_world_mesh_rb")
        .with<RigidBody>()
        .each(geom_update_mesh_transform);

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

    // OnValidate
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

        if (rb1.has<IsPinned>() && rb2.has<IsPinned>())
            return;

        const auto& geom1 = rb1.get<Geometry>();

        const auto& com1 = rb1.get<Position>();
        const auto& R1 = rb1.get<Rotation>();

        const auto& com2 = rb2.get<Position>();
        const auto& R2 = rb2.get<Rotation>();

        const auto& restitution1 = rb1.get<Restitution>();
        const auto& restitution2 = rb2.get<Restitution>();

        const auto& friction1 = rb1.get<Friction>();
        const auto& friction2 = rb2.get<Friction>();

        auto restitution = restitution1.value * restitution2.value;
        auto friction = friction1.value + friction2.value;

        auto predicate = [&](unsigned int node_index, unsigned int depth) {
            const PBD::BoundingSphere& bs = bvh1.bvh_.hull(node_index);
            const Vector3r& sphere_x = bs.x();
            const Vector3r sphere_x_w = bs.x();

            AlignedBox3r box3f;
            box3f.extend(aabb2.aabb_.m_p[0]);
            box3f.extend(aabb2.aabb_.m_p[1]);
            const Real dist = box3f.exteriorDistance(sphere_x_w);

            if (dist < bs.r()) {
                const Vector3r x = R2.value.transpose() * (sphere_x_w - com2.value);
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

    ecs.system<Contact>().each(rb_compute_contact_jacobian);
    ecs.system<ConstraintForce, ConstraintTorque>().with<RigidBody>().each(rb_clear_constraint_forces);
    ecs.system().run(solve_contacts_pgs);

    ecs.system<LinearVelocity,
               AngularVelocity,
               Mass,
               LinearForce,
               ConstraintForce,
               AngularForce,
               ConstraintTorque,
               Position,
               Orientation,
               WorldInertia,
               WorldInverseInertia>("rb_integrate")
        .with<RigidBody>()
        .each(rb_integrate);

    // OnRender
    ecs.system<const Geometry>("draw_geometry_rb")
        .with<RigidBody>()
        .kind(graphics::OnRender)
        .each([mat1](const Geometry& geom) {
            DrawMesh(geom.renderable, mat1, MatrixIdentity());

            const auto& faces = geom.mesh.getFaces();
            const auto& vertices = geom.verts.getVertices();

            for (size_t i = 0; i < faces.size(); i += 3) {
                const auto& v1 = vertices[faces[i]];
                const auto& v2 = vertices[faces[i + 1]];
                const auto& v3 = vertices[faces[i + 2]];

                Vector3 p1 = {(float)v1.x(), (float)v1.y(), (float)v1.z()};
                Vector3 p2 = {(float)v2.x(), (float)v2.y(), (float)v2.z()};
                Vector3 p3 = {(float)v3.x(), (float)v3.y(), (float)v3.z()};

                DrawLine3D(p1, p2, DARKGRAY);
                DrawLine3D(p2, p3, DARKGRAY);
                DrawLine3D(p3, p1, DARKGRAY);
            }

            for (size_t i = 0; i < geom.verts.size(); ++i) {
                auto p = geom.verts.getPosition(i);
                DrawPoint3D({(float)p.x(), (float)p.y(), (float)p.z()}, DARKGREEN);
            }
        });

    ecs.system<const AABB>("draw_aabb").kind(graphics::OnRender).each([](const AABB& aabb) {
        BoundingBox bbox;
        bbox.min = {(float)aabb.aabb_.m_p[0][0], (float)aabb.aabb_.m_p[0][1], (float)aabb.aabb_.m_p[0][2]};
        bbox.max = {(float)aabb.aabb_.m_p[1][0], (float)aabb.aabb_.m_p[1][1], (float)aabb.aabb_.m_p[1][2]};

        DrawBoundingBox(bbox, BLUE);
    });

    ecs.system<const BVH>("draw_bvh")
        .kind(graphics::OnRender)
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

    ecs.system<const Contact>("draw_contact").kind(graphics::OnRender).each([](const Contact& contact) {
        const auto& x = contact.x_world;
        const auto& cp = contact.closest_point_world;
        const auto& n = contact.normal_world;

        DrawSphere({(float)x.x(), (float)x.y(), (float)x.z()}, 0.03f, RED);
        DrawSphere({(float)cp.x(), (float)cp.y(), (float)cp.z()}, 0.03f, GREEN);

        Vector3r normal_end = cp + n * 0.2f;
        DrawLine3D({(float)cp.x(), (float)cp.y(), (float)cp.z()}, {(float)normal_end.x(), (float)normal_end.y(), (float)normal_end.z()}, BLUE);
        DrawLine3D({(float)x.x(), (float)x.y(), (float)x.z()}, {(float)cp.x(), (float)cp.y(), (float)cp.z()}, GRAY);
    });

    ecs.system("TickTime")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter&) {
            global_time += delta_time;
        });

    graphics::run_loop();

    std::cout << "Simulation ended." << std::endl;
    return 0;
}
