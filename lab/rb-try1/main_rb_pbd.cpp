#include "flecs.h"

#include "AABB.h"
#include "BoundingSphereHierarchy.h"
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

// struct Contact {
//     flecs::entity e1;
//     flecs::entity e2;
//     Vector3r x_world;
//     Vector3r closest_point_world;
//     Vector3r normal_world;
//     Real dist;
//     Real restitution;
//     Real friction;
// };

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
    // rb1.add<IsPinned>();

    auto rb2 = rb_add_rigid_body(ecs, 1);
    rb2.add<Geometry>();
    rb2.add<AABB>();
    rb2.add<BVH>();

    rb1.get_mut<Position>()->value = {-1, 0, 0};
    rb1.get_mut<LinearVelocity>()->value = {0.2, 0, 0};
    rb2.get_mut<Position>()->value = {1, 0, 0};
    rb2.get_mut<LinearVelocity>()->value = {-0.2, 0, 0};
    rb2.get_mut<AngularVelocity>()->value = {0.0, 0.0, 0.3};

    // create raylib mesh
    rl::Mesh mesh1 = rl::GenMeshCube(1, 1, 1);
    init_geometry_from_rlmesh(rb1, mesh1, 1.0f);
    rl::UnloadMesh(mesh1);
    // rb1.get_mut<Mass>()->value = std::numeric_limits<float>::max();
    // rb1.get_mut<InverseMass>()->value = 0;

    rl::Mesh mesh2 = rl::GenMeshCube(1, 1, 1);
    init_geometry_from_rlmesh(rb2, mesh2, 1.0f);
    rl::UnloadMesh(mesh2);

    // OnUpdate
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

    // do pbd
    ecs.system<const Contact>().each([](flecs::iter& it, size_t, const Contact& contact) {
        // get both bodies
        auto rb1 = contact.e1;
        auto rb2 = contact.e2;

        // get local positions relative to COM
        Vector3r r1 = contact.x_world - rb1.get<Position>()->value; // r1 in world space
        Vector3r r2 = contact.closest_point_world - rb2.get<Position>()->value; // r2 in world space

        // transform normal to local space of each body
        Vector3r n1 = rb1.get<Rotation>()->value * contact.normal_world; // n1 in body1 space
        Vector3r n2 = rb2.get<Rotation>()->value * (-contact.normal_world); // n2 in body2 space

        // compute denominators
        Vector3r temp1 = r1.cross(n1);
        Vector3r temp2 = r2.cross(n2);

        Real w1 = rb1.get<InverseMass>()->value + (temp1.transpose() * rb1.get<WorldInverseInertia>()->value * temp1);
        Real w2 = rb2.get<InverseMass>()->value + (temp2.transpose() * rb2.get<WorldInverseInertia>()->value * temp2);

        // compute position correction
        Real delta_lambda = -contact.dist / (w1 + w2) * 0.2f;

        // apply correction to first body
        {
            // position update
            Vector3r p = delta_lambda * n1;
            rb1.get_mut<Position>()->value += rb1.get<Rotation>()->value * p * rb1.get<InverseMass>()->value;

            // orientation update
            Vector3r angular_impulse = rb1.get<Rotation>()->value * rb1.get<WorldInverseInertia>()->value * r1.cross(p);

            Quaternionr dq(0, angular_impulse.x(), angular_impulse.y(), angular_impulse.z());
            rb1.get_mut<Orientation>()->value.coeffs() += 0.5f * (dq * rb1.get<Orientation>()->value).coeffs();
            rb1.get_mut<Orientation>()->value.normalize();
        }

        // apply correction to second body
        {
            // position update
            Vector3r p = delta_lambda * n2;
            rb2.get_mut<Position>()->value += rb2.get<Rotation>()->value * p * rb2.get<InverseMass>()->value;

            // orientation update
            Vector3r angular_impulse = rb2.get<Rotation>()->value * rb2.get<WorldInverseInertia>()->value * r2.cross(p);

            Quaternionr dq(0, angular_impulse.x(), angular_impulse.y(), angular_impulse.z());
            rb2.get_mut<Orientation>()->value.coeffs() += 0.5f * (dq * rb2.get<Orientation>()->value).coeffs();
            rb2.get_mut<Orientation>()->value.normalize();
        }
    });

    // Update velocities
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

    // must be updated before collision handling
    ecs.system<const Position, const Rotation, Geometry>("update_world_mesh_rb")
        .with<RigidBody>()
        .each(geom_update_mesh_transform);

    // clear any collision pairs
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

    // ecs.system().with<CollisionPair>().each([](flecs::iter& it, size_t) {
    //     std::cout << "Number of collision pairs: " << it.count() << std::endl;
    // });

    ecs.system().with<Contact>().each([](flecs::entity e) { e.destruct(); });
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

        auto bvh1 = rb1.get<BVH>();
        auto bvh2 = rb2.get<BVH>();

        auto co1 = rb1.get<Collider>();
        auto co2 = rb2.get<Collider>();

        auto m1 = rb1.get<Mass>();
        auto m2 = rb2.get<Mass>();

        if ((m1 == 0) && (m2 == 0))
            return; // don't report collision between static bodies

        // surface vertices, in world coord
        auto geom1 = rb1.get<Geometry>();

        auto com1 = rb1.get<Position>();
        auto R1 = rb1.get<Rotation>();

        auto com2 = rb2.get<Position>();
        auto R2 = rb2.get<Rotation>();

        auto restitution1 = rb1.get<Restitution>();
        auto restitution2 = rb2.get<Restitution>();

        auto friction1 = rb2.get<Friction>();
        auto friction2 = rb2.get<Friction>();

        auto restitution = restitution1->value * restitution2->value;
        auto friction = friction1->value + friction2->value;

        // [midphase]
        auto predicate = [&](unsigned int node_index, unsigned int depth) {
            const PBD::BoundingSphere& bs = bvh1->bvh_.hull(node_index);
            const Vector3r& sphere_x = bs.x();
            const Vector3r sphere_x_w = R1->value * sphere_x + com1->value;

            AlignedBox3r box3f;
            box3f.extend(aabb2->aabb_.m_p[0]);
            box3f.extend(aabb2->aabb_.m_p[1]);
            const Real dist = box3f.exteriorDistance(sphere_x_w);

            // Test if center of bounding sphere intersects AABB
            if (dist < bs.r()) {
                // Test if distance of center of bounding sphere to collision object is smaller than the radius
                const Vector3r x = R2->value * (sphere_x_w - com2->value) + com2->value;
                const double dist2 = co2->distance(x.template cast<double>(), tolerance);

                if (dist2 == std::numeric_limits<double>::max()) {
                    return true;
                }

                if (dist2 < bs.r()) {
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
                unsigned int vertex_id = bvh1->bvh_.entity(i);
                const Vector3r& x_w = geom1->verts.getPosition(vertex_id);
                const Vector3r x = R2->value.transpose() * (x_w - com2->value); // send x_w to body space of rb2
                Vector3r cp, n;
                Real dist;

                if (co2->collision_test(x, tolerance, cp, n, dist)) {
                    const Vector3r cp_w = R2->value * cp + com2->value;
                    const Vector3r n_w = R2->value * n;

                    it.world().entity().set<Contact>({rb1, rb2, x_w, cp_w, n_w, dist, restitution, friction});
                }
            }
        };

        bvh1->bvh_.traverse_depth_first(predicate, cb);
    });

    // ecs.system().with<Contact>().each([](flecs::iter& it, size_t) {
    //     std::cout << "Number of contacts: " << it.count() << std::endl;
    // });

    // OnRender
    ecs.system<const Geometry>("draw_geometry_rb")
        .with<RigidBody>()
        .kind(graphics::OnRender)
        .each([](const Geometry& geom) {
            rl::DrawMesh(geom.renderable, rl::LoadMaterialDefault(), rl::MatrixIdentity());

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