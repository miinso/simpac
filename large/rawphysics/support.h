#ifndef RAW_PHYSICS_PHYSICS_SUPPORT_H
#define RAW_PHYSICS_PHYSICS_SUPPORT_H

#include "collider.h"
#include "types.h"

unsigned int support_point_get_index(Collider_Convex_Hull* convex_hull, const Vector3r& direction);
Vector3r support_point(Collider* collider, const Vector3r& direction);
Vector3r support_point_of_minkowski_difference(Collider* collider1, Collider* collider2, const Vector3r& direction);

#endif