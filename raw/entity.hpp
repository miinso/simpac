#include "types.h"
#include "collider.h"
#include <flecs.h>
#include <raylib.h>
#include <vector>

using namespace phys::pbd;
// using namespace phys::pbd::rigidbody;

inline void register_prefabs (flecs::world& ecs) {

    ecs.prefab<prefabs::RigidBody> ()
    .add<RigidBody> ()

    .add<Position> ()
    .add<Orientation> ()
    .add<LinearVelocity> ()
    .add<AngularVelocity> ()

    .add<Position0> ()
    .add<Orientation0> ()
    .add<LinearVelocity0> ()
    .add<AngularVelocity0> ()

    .add<OldPosition> ()
    .add<OldOrientation> ()
    .add<OldLinearVelocity> ()
    .add<OldAngularVelocity> ()

    .add<LinearForce> ()
    .add<AngularForce> () // torque
    .add<Mass> ()
    .add<InverseMass> ()
    .add<LocalInertia> ()
    .add<LocalInverseInertia> ()
    // .add<WorldInertia> ()
    // .add<WorldInverseInertia> ()
    .add<Mesh>()
    // .add<PhysicsMesh>();
    .add<Forces> ();

    
}

flecs::entity add_rigid_body (flecs::world& ecs,
const Vector3r& x0      = Vector3r::Zero (),
const Quaternionr& q0   = Quaternionr::Identity (),
const Vector3r& v0      = Vector3r::Zero (),
const Vector3r& omega0  = Vector3r::Zero (),
Real mass               = 1.0f,
const Matrix3r& inertia = Matrix3r::Identity ()) {

    // TODO: inertia should be set when a physics_mesh is attached to this
    // entity do closed form thing or volume integration.
    Matrix3r inv_inertia = inertia.inverse ();

    return ecs.entity ()
    .is_a<prefabs::RigidBody> ()

    .set<Position> ({ x0 })
    .set<Orientation> ({ q0 })
    .set<LinearVelocity> ({ v0 })
    .set<AngularVelocity> ({ omega0 })

    .set<Position0> ({ x0 })
    .set<Orientation0> ({ q0 })
    .set<LinearVelocity0> ({ v0 })
    .set<AngularVelocity0> ({ omega0 })

    .set<OldPosition> ({ x0 })
    .set<OldOrientation> ({ q0 })
    .set<OldLinearVelocity> ({ v0 })
    .set<OldAngularVelocity> ({ omega0 })

    .set<LinearForce> ({ Vector3r::Zero () })
    .set<AngularForce> ({ Vector3r::Zero () })
    .set<Mass> ({ mass })
    .set<InverseMass> ({ 1.0f / mass })
    .set<LocalInertia> ({ inertia })
    .set<LocalInverseInertia> ({ inv_inertia });
    // .set<WorldInertia> ({ inertia })
    // .set<WorldInverseInertia> ({ inv_inertia });
}

// flecs::entity add_rigidbody() {}
// flecs::entity add_constraint() {}
// void draw_rigidbody() {}

Collider createSphereCollider(Real radius) {
    return collider_sphere_create(radius);
}

Collider createConvexHullColliderFromMesh(const Mesh& mesh, const Vector3r& scale) {
    std::vector<Vector3r> vertices;
    vertices.reserve(mesh.vertexCount);
    
    for (int i = 0; i < mesh.vertexCount; ++i) {
        Vector3r position;
        position.x() = static_cast<Real>(mesh.vertices[i * 3]) * scale.x();
        position.y() = static_cast<Real>(mesh.vertices[i * 3 + 1]) * scale.y();
        position.z() = static_cast<Real>(mesh.vertices[i * 3 + 2]) * scale.z();
        vertices.push_back(position);
    }
    
    std::vector<unsigned int> indices;
    indices.reserve(mesh.triangleCount * 3);
    for (int i = 0; i < mesh.triangleCount * 3; ++i) {
        indices.push_back(static_cast<unsigned int>(mesh.indices[i]));
    }
    
    return collider_convex_hull_create(vertices, indices);
}

std::vector<Collider> createSphereColliders(Real radius) {
    std::vector<Collider> colliders;
    colliders.push_back(createSphereCollider(radius));
    return colliders;
}

std::vector<Collider> createMeshColliders(const Mesh& mesh, const Vector3r& scale) {
    std::vector<Collider> colliders;
    colliders.push_back(createConvexHullColliderFromMesh(mesh, scale));
    return colliders;
}

std::vector<Collider> example_util_create_sphere_convex_hull_array(Real radius) {
    Collider collider = collider_sphere_create(radius);
    
    std::vector<Collider> colliders;
    colliders.push_back(collider);
    
    return colliders;
}

void throw_object(flecs::world& ecs) {
    // mesh = rendermesh = raylib::Mesh
    // collider = physicsmesh = derived from raylib::Mesh

    // Mesh m = GenMeshCube(1, 1, 1);

    int r = rand();
    int is_sphere = 0;
    
    Mesh m;

    if (r % 4 == 0) {
        m = GenMeshCube(1, 1, 1);
    } else if (r % 4 == 1) {
        m = GenMeshCone(0.5, 1, 8);
    } else if (r % 4 == 2) {
        m = GenMeshCylinder(0.5, 1, 8);
    } else {
        is_sphere = 1;
        m = GenMeshSphere(0.5, 8, 8);
    }

    // mesh.verts, mesh.indices -> required to generate collider
    // or decomposed convex collider pieces

    // a body can have multiple colliders.
    // each collider add its own inertia to its parent body
    Vector3r scale;
    std::vector<Collider> colliders;

    if (is_sphere) {
        Real radius = 1.0;
        scale = Vector3r::Ones() * radius;
        colliders = createSphereColliders(radius);
    } else {
        scale = Vector3r::Ones();
        colliders = createMeshColliders(m, scale);
    }

    auto e1 = add_rigid_body(ecs);
    e1.set<Mesh>(m);
    auto radius = colliders_get_bounding_sphere_radius(colliders);
    e1.set<BoundingSphere>({radius});
    auto inertia = colliders_get_default_inertia_tensor(colliders, e1.get<Mass>()->value);
    e1.set<LocalInertia>({inertia});
    e1.set<LocalInverseInertia>({inertia.inverse()});
    // e1.get_mut<LinearVelocity>()->value = {0, 1, 0};


    // DrawMesh(Mesh mesh, Material material, Matrix transform);
    // TODO: build the world transform using x, q, s
    

}