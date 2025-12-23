#include "support.h"
#include <limits>
#include <cassert>

unsigned int support_point_get_index(Collider_Convex_Hull* convex_hull, const Vector3r& direction) {
    unsigned int selected_index = 0;
    Real max_dot = -std::numeric_limits<Real>::max();
    
    for (unsigned int i = 0; i < convex_hull->transformed_vertices.size(); ++i) {
        Real dot = convex_hull->transformed_vertices[i].dot(direction);
        if (dot > max_dot) {
            selected_index = i;
            max_dot = dot;
        }
    }

    return selected_index;
}

Vector3r support_point(Collider* collider, const Vector3r& direction) {
    switch (collider->type) {
        case COLLIDER_TYPE_CONVEX_HULL: {
            unsigned int selected_index = support_point_get_index(&collider->convex_hull, direction);
            return collider->convex_hull.transformed_vertices[selected_index];
        } break;
        
        case COLLIDER_TYPE_SPHERE: {
            return collider->sphere.center + collider->sphere.radius * direction.normalized();
        } break;
    }
    
    assert(false);
    return Vector3r::Zero();
}

Vector3r support_point_of_minkowski_difference(Collider* collider1, Collider* collider2, const Vector3r& direction) {
    Vector3r support1 = support_point(collider1, direction);
    Vector3r support2 = support_point(collider2, -direction);

    return support1 - support2;
}