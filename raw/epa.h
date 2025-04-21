#ifndef RAW_PHYSICS_PHYSICS_EPA_H
#define RAW_PHYSICS_PHYSICS_EPA_H

// #include <common.h>
#include "types.h"
#include "gjk.h"

bool epa(Collider* collider1, Collider* collider2, GJK_Simplex* simplex, Vector3r& normal, Real& penetration);

#endif