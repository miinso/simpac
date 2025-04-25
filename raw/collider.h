#ifndef RAW_PHYSICS_PHYSICS_COLLIDER_H
#define RAW_PHYSICS_PHYSICS_COLLIDER_H

#include <vector>
#include <memory>
#include "types.h"

struct Collider_Contact {
    Vector3r collision_point1;
    Vector3r collision_point2;
    Vector3r normal;
};

struct Collider_Convex_Hull_Face {
    std::vector<unsigned int> elements;
    Vector3r normal;
};

struct Collider_Convex_Hull {
    std::vector<Vector3r> vertices;
    std::vector<Vector3r> transformed_vertices;
    std::vector<Collider_Convex_Hull_Face> faces;
    std::vector<Collider_Convex_Hull_Face> transformed_faces;

    std::vector<std::vector<unsigned int>> vertex_to_faces;
    std::vector<std::vector<unsigned int>> vertex_to_neighbors;
    std::vector<std::vector<unsigned int>> face_to_neighbors;
};

struct Collider_Sphere {
    Real radius;
    Vector3r center;
};


enum Collider_Type {
    COLLIDER_TYPE_SPHERE,
    // COLLIDER_TYPE_PLAIN,
    // COLLIDER_TYPE_BOX,
    // COLLIDER_TYPE_CAPSULE,
    COLLIDER_TYPE_CONVEX_HULL
    // COLLIDER_TYPE_SDF,
};

struct Collider {
    Collider_Type type;
    
    // union {
    //     Collider_Convex_Hull convex_hull;
    //     Collider_Sphere sphere;
    // };

    Collider_Convex_Hull convex_hull;
    Collider_Sphere sphere;
};

struct Colliders { std::vector<Collider> value; };

// @NOTE: for simplicity (and speed), we don't deal with scaling in the colliders.
// therefore, if the object is scaled, the collider needs to be recreated (and the vertices should be already scaled when creating it)
Collider collider_convex_hull_create(const std::vector<Vector3r>& vertices, const std::vector<unsigned int>& indices);
Collider collider_sphere_create(const Real radius);

void colliders_update(std::vector<Collider>& colliders, const Vector3r& translation, const Quaternionr* rotation);
void colliders_destroy(std::vector<Collider>& colliders);
Matrix3r colliders_get_default_inertia_tensor(const std::vector<Collider>& colliders, Real mass);
Real colliders_get_bounding_sphere_radius(const std::vector<Collider>& colliders);
std::vector<Collider_Contact> colliders_get_contacts(const std::vector<Collider>& colliders1, const std::vector<Collider>& colliders2);

#endif