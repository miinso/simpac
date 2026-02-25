#pragma once

#include <flecs.h>
#include <raylib.h>

#include "components.h"

namespace graphics {

// composable material components — maps to PBR shader uniforms.
// attach any subset to an entity; upload helpers skip missing uniforms.

struct MaterialLocs; // forward decl for upload methods

struct Albedo {
    color4f color = {1, 1, 1, 1};
    void upload(const MaterialLocs& locs) const;
};

struct Surface {
    float metallic = 0.0f;
    float roughness = 1.0f;
    float ao = 1.0f;
    void upload(const MaterialLocs& locs) const;
};

struct Emissive {
    color4f color = {0, 0, 0, 1};
    float power = 0.0f;
    void upload(const MaterialLocs& locs) const;
};

// cached shader uniform locs for material uploads
struct MaterialLocs {
    Shader shader = {};
    int albedo_color = -1;
    int metallic = -1;
    int roughness = -1;
    int ao = -1;
    int emissive_color = -1;
    int emissive_power = -1;
    int tiling = -1;

    // populate locs once for a given shader
    static MaterialLocs cache(Shader s) {
        MaterialLocs locs;
        locs.shader = s;
        locs.albedo_color  = GetShaderLocation(s, "albedoColor");
        locs.metallic      = GetShaderLocation(s, "metallicValue");
        locs.roughness     = GetShaderLocation(s, "roughnessValue");
        locs.ao            = GetShaderLocation(s, "aoValue");
        locs.emissive_color = GetShaderLocation(s, "emissiveColor");
        locs.emissive_power = GetShaderLocation(s, "emissivePower");
        locs.tiling        = GetShaderLocation(s, "tiling");
        return locs;
    }

    // upload all material components present on entity
    void upload(flecs::entity e) const;
};

namespace components {

inline void register_material_components(flecs::world& world) {
    world.component<Albedo>()
        .member<color4f>("color")
        .add(flecs::OnInstantiate, flecs::Inherit);

    world.component<Surface>()
        .member<float>("metallic")
        .member<float>("roughness")
        .member<float>("ao")
        .add(flecs::OnInstantiate, flecs::Inherit);

    world.component<Emissive>()
        .member<color4f>("color")
        .member<float>("power")
        .add(flecs::OnInstantiate, flecs::Inherit);
}

} // namespace components

// --- material upload definitions ---

inline void Albedo::upload(const MaterialLocs& locs) const {
    if (locs.albedo_color >= 0) {
        float v[4] = {color.x, color.y, color.z, color.w};
        SetShaderValue(locs.shader, locs.albedo_color, v, SHADER_UNIFORM_VEC4);
    }
}

inline void Surface::upload(const MaterialLocs& locs) const {
    if (locs.metallic >= 0) SetShaderValue(locs.shader, locs.metallic, &metallic, SHADER_UNIFORM_FLOAT);
    if (locs.roughness >= 0) SetShaderValue(locs.shader, locs.roughness, &roughness, SHADER_UNIFORM_FLOAT);
    if (locs.ao >= 0) SetShaderValue(locs.shader, locs.ao, &ao, SHADER_UNIFORM_FLOAT);
}

inline void Emissive::upload(const MaterialLocs& locs) const {
    if (locs.emissive_color >= 0) {
        float v[4] = {color.x, color.y, color.z, color.w};
        SetShaderValue(locs.shader, locs.emissive_color, v, SHADER_UNIFORM_VEC4);
    }
    if (locs.emissive_power >= 0) SetShaderValue(locs.shader, locs.emissive_power, &power, SHADER_UNIFORM_FLOAT);
}

inline void MaterialLocs::upload(flecs::entity e) const {
    if (auto* a = e.try_get<Albedo>()) a->upload(*this);
    if (auto* s = e.try_get<Surface>()) s->upload(*this);
    if (auto* em = e.try_get<Emissive>()) em->upload(*this);
}

} // namespace graphics
