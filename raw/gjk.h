#ifndef RAW_PHYSICS_PHYSICS_GJK_H
#define RAW_PHYSICS_PHYSICS_GJK_H

// #include <common.h>
#include "types.h"
#include "collider.h"

struct GJK_Simplex {
    Vector3r a, b, c, d;
    unsigned int num;
};

bool gjk_collides(Collider* collider1, Collider* collider2, GJK_Simplex* simplex);

#endif