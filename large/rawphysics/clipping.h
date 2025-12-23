#ifndef RAW_PHYSICS_PHYSICS_CLIPPING_H
#define RAW_PHYSICS_PHYSICS_CLIPPING_H

#include <vector>
#include "types.h"
#include "collider.h"

void clipping_get_contact_manifold(
    Collider* collider1, 
    Collider* collider2, 
    const Vector3r& normal, 
    Real penetration, 
    std::vector<Collider_Contact>& contacts);

#endif