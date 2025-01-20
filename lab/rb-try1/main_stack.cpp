#include "flecs.h"

#include "AABB.h"
#include "BoundingSphereHierarchy.h"
#include "geometry.hpp"
#include "collision.hpp"
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
// float restitution = 0.0f;
// float friction = 0.0f;

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
    // rb2.add<IsPinned>();
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

    rl::Mesh mesh1 = rl::GenMeshCube(8, 0.2, 8);
    geom_init_geometry_from_rlmesh(rb1, mesh1, 1.0f);
    init_sdfcollider_from_geometry(rb1);
    // or
    // Geometry geom = geom_add_geometry_from_rlmesh();
    // Collider coll = coll_add_collider_from_geometry();
    // rb1.add<Geometry>(geom);
    // rb1.add<Collider>(coll);
    rl::UnloadMesh(mesh1);

    rl::Mesh mesh2 = rl::GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb2, mesh2, 0.1f);
    init_sdfcollider_from_geometry(rb2);
    rl::UnloadMesh(mesh2);

    rl::Mesh mesh3 = rl::GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb3, mesh3, 0.1f);
    init_sdfcollider_from_geometry(rb3);
    rl::UnloadMesh(mesh3);

    rl::Mesh mesh4 = rl::GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb4, mesh4, 0.1f);
    init_sdfcollider_from_geometry(rb4);
    rl::UnloadMesh(mesh4);

    rl::Mesh mesh5 = rl::GenMeshCube(1, 1, 1);
    geom_init_geometry_from_rlmesh(rb5, mesh5, 0.1f);
    init_sdfcollider_from_geometry(rb5);
    rl::UnloadMesh(mesh5);

    rb2.get_mut<Position>()->value = {0, 1, 0};
    rb3.get_mut<Position>()->value = {0, 2.5, 0};
    rb4.get_mut<Position>()->value = {0, 3.5, 0};
    rb5.get_mut<Position>()->value = {0, 4.5, 0};
    // auto qq = rl::QuaternionFromEuler(45, 0, 0);
    // rb5.get_mut<Orientation>()->value = {qq.w, qq.x, qq.y, qq.z};

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

    // must be updated before collision handling
    ecs.system<const Position, const Rotation, Geometry>("update_world_mesh_rb")
        .with<RigidBody>()
        .each(geom_update_mesh_transform);

    ecs.system<AABB, Geometry>("update_aabb").with<RigidBody>().each([](AABB& aabb, const Geometry& geom) {
        aabb.aabb_.m_p[0] = geom.verts.getPosition(0);
        aabb.aabb_.m_p[1] = geom.verts.getPosition(0);

        for (size_t i = 0; i < geom.verts.size(); ++i) {
            auto p = geom.verts.getPosition(i);

            if (aabb.aabb_.m_p[0][0] > p[0])
                aabb.aabb_.m_p[0][0] = p[0];
            if (aabb.aabb_.m_p[0][1] > p[1])
                aabb.aabb_.m_p[0][1] = p[1];
            if (aabb.aabb_.m_p[0][2] > p[2])
                aabb.aabb_.m_p[0][2] = p[2];
            if (aabb.aabb_.m_p[1][0] < p[0])
                aabb.aabb_.m_p[1][0] = p[0];
            if (aabb.aabb_.m_p[1][1] < p[1])
                aabb.aabb_.m_p[1][1] = p[1];
            if (aabb.aabb_.m_p[1][2] < p[2])
                aabb.aabb_.m_p[1][2] = p[2];
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

    // clear any collision pairs
    ecs.system().with<CollisionPair>().each([](flecs::entity e) { e.destruct(); });

    auto query_all_rb = ecs.query_builder<RigidBody>().build();
    ecs.system().run([query_all_rb](flecs::iter& it) {
        query_all_rb.each([&](flecs::entity e1, RigidBody& rb1) {
            query_all_rb.each([&](flecs::entity e2, RigidBody& rb2) {
                if (e1 != e2) {
                    it.world().entity().set<CollisionPair>({e1, e2});
                }
                // if (e1 > e2) {
                //     it.world().entity().set<CollisionPair>({e1, e2});
                // }
            });
        });
    });

    // ecs.system().with<CollisionPair>().each([](flecs::iter& it, size_t) {
    //     std::cout << "Number of collision pairs: " << it.count() << std::endl;
    // });

    // an rb can have more than one collision object??
    ecs.system<CollisionPair>().each([](flecs::iter& it, size_t, CollisionPair& pair) {
        auto rb1 = pair.e1;
        auto rb2 = pair.e2;

        auto aabb1 = rb1.get<AABB>();
        auto aabb2 = rb2.get<AABB>();

        // [boadphase]
        if (PBD::AABB::intersection(aabb1->aabb_, aabb2->aabb_) == false) {
            return;
        }

        // std::cout << "[broadphase] Rb " << rb1.path() << " and Rb " << rb2.path() << " bumped into each other!"
        //           << std::endl;

        auto bvh1 = rb1.get<BVH>();
        auto bvh2 = rb2.get<BVH>();

        auto co1 = rb1.get<Collider>();
        auto co2 = rb2.get<Collider>();

        auto m1 = rb1.get<Mass>();
        auto m2 = rb2.get<Mass>();

        // don't report collision between static bodies
        // if ((m1 == 0) && (m2 == 0))
        if (rb1.has<IsPinned>() && rb2.has<IsPinned>())
            return;

        // surface vertices, in world coord
        auto geom1 = rb1.get<Geometry>();

        auto com1 = rb1.get<Position>();
        auto R1 = rb1.get<Rotation>();

        auto com2 = rb2.get<Position>();
        auto R2 = rb2.get<Rotation>();

        auto restitution1 = rb1.get<Restitution>();
        auto restitution2 = rb2.get<Restitution>();

        auto friction1 = rb1.get<Friction>();
        auto friction2 = rb2.get<Friction>();

        auto restitution = restitution1->value * restitution2->value;
        auto friction = friction1->value + friction2->value;

        // [midphase]
        auto predicate = [&](unsigned int node_index, unsigned int depth) {
            const PBD::BoundingSphere& bs = bvh1->bvh_.hull(node_index);
            const Vector3r& sphere_x = bs.x();
            // WARNING: Fix this!
            // const Vector3r sphere_x_w = R1->value * sphere_x + com1->value;
            const Vector3r sphere_x_w = bs.x();

            AlignedBox3r box3f;
            box3f.extend(aabb2->aabb_.m_p[0]);
            box3f.extend(aabb2->aabb_.m_p[1]);
            const Real dist = box3f.exteriorDistance(sphere_x_w);

            // Test if center of bounding sphere intersects AABB
            if (dist < bs.r()) {
                // Test if distance of center of bounding sphere to collision object is smaller than the radius
                // const Vector3r x = R2->value * (sphere_x_w - com2->value) + com2->value;
                const Vector3r x = R2->value.transpose() * (sphere_x_w - com2->value);
                const double dist2 = co2->distance(x.template cast<double>(), tolerance);

                if (dist2 == std::numeric_limits<double>::max()) {
                    // std::cout << "[midphase] Penetration detected between " << rb1.path() << " and " << rb2.path()
                    //           << std::endl;
                    return true;
                }

                if (dist2 < bs.r()) {
                    // std::cout << "[midphase] Near collision detected between " << rb1.path() << " and " << rb2.path()
                    //           << std::endl;
                    return true;
                }
            }
            return false;
        };

        // [narrowphase]
        auto cb = [&](unsigned int node_index, unsigned int depth) {
            auto const& node = bvh1->bvh_.node(node_index);
            if (!node.is_leaf())
                return;

            for (auto i = node.begin; i < node.begin + node.n; ++i) {
                // unsigned int index = bvh.entity(i) + offset;
                unsigned int vertex_id = bvh1->bvh_.entity(i);
                const Vector3r& x_w = geom1->verts.getPosition(vertex_id);
                const Vector3r x = R2->value.transpose() * (x_w - com2->value); // w.r.t. rb2
                Vector3r cp, n;
                Real dist;
                // std::cout << "asd" << std::endl;
                if (co2->collision_test(x, tolerance, cp, n, dist)) {
                    const Vector3r cp_w = R2->value * cp + com2->value;
                    const Vector3r n_w = R2->value * n;

                    // std::cout << "[narrowphase] hit! center hit!" << std::endl;

                    it.world().entity().set<Contact>({rb1, rb2, x_w, cp_w, n_w, dist, restitution, friction});
                }
            }
        };

        bvh1->bvh_.traverse_depth_first(predicate, cb);
    });

    // compute contact jacobian
    //      compute contact frame
    //      compute jacobian
    ecs.system<Contact>().each(rb_compute_contact_jacobian);

    // clear constraint forces
    ecs.system<ConstraintForce, ConstraintTorque>().with<RigidBody>().each(rb_clear_constraint_forces);

    // calc constraint forces
    ecs.system().run(solve_contacts_pgs);

    // ecs.system().run([](flecs::iter& it) { std::cout << it.delta_time() << std::endl; });

    // integrate
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
            rl::DrawMesh(geom.renderable, mat1, rl::MatrixIdentity());

            // get mesh faces and vertices
            const auto& faces = geom.mesh.getFaces();
            const auto& vertices = geom.verts.getVertices();

            // iterate through faces (triangles)
            for (size_t i = 0; i < faces.size(); i += 3) {
                // get vertices of current triangle
                const auto& v1 = vertices[faces[i]];
                const auto& v2 = vertices[faces[i + 1]];
                const auto& v3 = vertices[faces[i + 2]];

                // convert to raylib Vector3
                rl::Vector3 p1 = {v1.x(), v1.y(), v1.z()};
                rl::Vector3 p2 = {v2.x(), v2.y(), v2.z()};
                rl::Vector3 p3 = {v3.x(), v3.y(), v3.z()};

                // draw triangle wireframe
                rl::DrawLine3D(p1, p2, rl::DARKGRAY);
                rl::DrawLine3D(p2, p3, rl::DARKGRAY);
                rl::DrawLine3D(p3, p1, rl::DARKGRAY);
            }

            for (size_t i = 0; i < geom.verts.size(); ++i) {
                auto p = geom.verts.getPosition(i);
                rl::DrawPoint3D({p.x(), p.y(), p.z()}, rl::DARKGREEN);
            }
        });

    ecs.system<const AABB>("draw_aabb").kind(graphics::OnRender).each([](const AABB& aabb) {
        rl::BoundingBox bbox;
        bbox.min = {aabb.aabb_.m_p[0][0], aabb.aabb_.m_p[0][1], aabb.aabb_.m_p[0][2]};
        bbox.max = {aabb.aabb_.m_p[1][0], aabb.aabb_.m_p[1][1], aabb.aabb_.m_p[1][2]};

        rl::DrawBoundingBox(bbox, rl::BLUE);
    });

    ecs.system<const BVH>("draw_bvh")
        .kind(graphics::OnRender)
        .each([](const BVH& bvh) {
            if (!bvh.initialized)
                return;

            rl::Color colors[] = {
                {255, 0, 0, 50}, //
                {0, 255, 0, 50}, //
                {0, 0, 255, 50} //
            };

            // depth-first traverse with visualization
            auto predicate = [&](unsigned int node_index, unsigned int depth) {
                return (int)depth <= bvh.render_depth;
            };

            auto callback = [&](unsigned int node_index, unsigned int depth) {
                if (depth == bvh.render_depth) {
                    const PBD::BoundingSphere& bs = bvh.bvh_.hull(node_index);
                    Vector3r center = bs.x();
                    float radius = std::max(bs.r(), 0.05f);

                    rl::DrawSphere({center.x(), center.y(), center.z()}, radius, colors[depth % 3]);
                }
            };

            bvh.bvh_.traverse_depth_first(predicate, callback);
        })
        .disable();

    ecs.system<const Contact>("draw_contact").kind(graphics::OnRender).each([](const Contact& contact) {
        // const auto& contact = contacts[i];
        const auto& x = contact.x_world;
        const auto& cp = contact.closest_point_world;
        const auto& n = contact.normal_world;

        // draw query point (x_world)
        rl::DrawSphere({x.x(), x.y(), x.z()}, 0.03f, rl::RED);

        // draw closest point (closest_point_world)
        rl::DrawSphere({cp.x(), cp.y(), cp.z()}, 0.03f, rl::GREEN);

        // draw normal vector from closest point
        Vector3r normal_end = cp + n * 0.2f;
        rl::DrawLine3D({cp.x(), cp.y(), cp.z()}, {normal_end.x(), normal_end.y(), normal_end.z()}, rl::BLUE);

        // draw distance line between points
        rl::DrawLine3D({x.x(), x.y(), x.z()}, {cp.x(), cp.y(), cp.z()}, rl::GRAY);
    });

    graphics::run_main_loop([]() { global_time += delta_time; });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}