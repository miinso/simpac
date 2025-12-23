#include <Eigen/Dense>
#include <flecs.h>

#include "VolumeIntegration.h"

#include "components.hpp"

void geom_update_mesh_transform(const Position& com, const Rotation& R, Geometry& geom) {
    // update mesh transform
    for (size_t i = 0; i < geom.verts0.size(); ++i) {
        geom.verts.getPosition(i) = R.value * geom.verts0.getPosition(i) + com.value;
    }

    // update mesh normals
    geom.mesh.updateNormals(geom.verts, 0);
    geom.mesh.updateVertexNormals(geom.verts);

    // update raylib mesh buffers
    const auto& vertices = geom.verts.getVertices();
    const auto& normals = geom.mesh.getVertexNormals();

    // copy new vertex positions
    for (size_t i = 0; i < vertices.size(); i++) {
        geom.renderable.vertices[i * 3 + 0] = vertices[i].x();
        geom.renderable.vertices[i * 3 + 1] = vertices[i].y();
        geom.renderable.vertices[i * 3 + 2] = vertices[i].z();
    }

    // copy new normals
    for (size_t i = 0; i < normals.size(); i++) {
        geom.renderable.normals[i * 3 + 0] = normals[i].x();
        geom.renderable.normals[i * 3 + 1] = normals[i].y();
        geom.renderable.normals[i * 3 + 2] = normals[i].z();
    }

    // would bad things happen if `verts` holds double instead of float?

    // update GPU buffers
    rl::UpdateMeshBuffer(geom.renderable,
                         rl::SHADER_LOC_VERTEX_POSITION,
                         geom.renderable.vertices,
                         geom.renderable.vertexCount * 3 * sizeof(float),
                         0);
    rl::UpdateMeshBuffer(geom.renderable,
                         rl::SHADER_LOC_VERTEX_NORMAL,
                         geom.renderable.normals,
                         geom.renderable.vertexCount * 3 * sizeof(float),
                         0);
}

void geom_init_mesh_(Geometry& geom,
                     const unsigned int num_verts,
                     const unsigned int num_faces,
                     const Vector3r* vertices,
                     const unsigned int* indices,
                     const Utilities::IndexedFaceMesh::UVIndices& uv_indices,
                     const Utilities::IndexedFaceMesh::UVs& uvs,
                     const Vector3r& scale,
                     const bool use_flat_shading) {

    geom.mesh.release();
    geom.mesh.initMesh(num_verts, num_faces * 2, num_faces);
    geom.verts0.resize(num_verts);
    geom.verts.resize(num_verts);
    geom.mesh.setFlatShading(use_flat_shading);

    // set vertex positions with scaling
    for (unsigned int i = 0; i < num_verts; i++) {
        geom.verts0.getPosition(i) = vertices[i].cwiseProduct(scale);
        geom.verts.getPosition(i) = geom.verts0.getPosition(i);
    }

    // add faces
    for (unsigned int i = 0; i < num_faces; i++) {
        geom.mesh.addFace(&indices[3 * i]);
    }

    // setup UVs and build topology
    geom.mesh.copyUVs(uv_indices, uvs);
    geom.mesh.buildNeighbors();

    //

    // create raylib mesh
    geom.renderable = {0};
    geom.renderable.vertexCount = num_verts;
    geom.renderable.triangleCount = num_faces;

    // allocate vertex data
    geom.renderable.vertices = new float[num_verts * 3];
    geom.renderable.texcoords = new float[num_verts * 2];
    geom.renderable.normals = new float[num_verts * 3];
    geom.renderable.indices = new unsigned short[num_faces * 3];

    // copy vertex positions
    const auto& verts = geom.verts.getVertices();
    for (size_t i = 0; i < num_verts; i++) {
        geom.renderable.vertices[i * 3 + 0] = verts[i].x();
        geom.renderable.vertices[i * 3 + 1] = verts[i].y();
        geom.renderable.vertices[i * 3 + 2] = verts[i].z();
    }

    // copy normals
    const auto& normals = geom.mesh.getVertexNormals();
    for (size_t i = 0; i < num_verts; i++) {
        geom.renderable.normals[i * 3 + 0] = normals[i].x();
        geom.renderable.normals[i * 3 + 1] = normals[i].y();
        geom.renderable.normals[i * 3 + 2] = normals[i].z();
    }

    // copy UVs if available
    if (!uvs.empty()) {
        for (size_t i = 0; i < num_verts; i++) {
            geom.renderable.texcoords[i * 2 + 0] = uvs[i].x();
            geom.renderable.texcoords[i * 2 + 1] = uvs[i].y();
        }
    }

    // copy indices
    const auto& meshIndices = geom.mesh.getFaces();
    for (size_t i = 0; i < meshIndices.size(); i++) {
        geom.renderable.indices[i] = (unsigned short)meshIndices[i];
    }

    // upload to GPU
    rl::UploadMesh(&geom.renderable, true); // dynamic for updates
}

void geom_compute_mass_properties_(
    flecs::entity rb, // rigid body component containing mass properties
    Geometry& geom, // geometry component
    const float density // density for mass calculation
) {
    // create volume integration calculator
    Utilities::VolumeIntegration vi(
        geom.verts0.size(), // number of vertices
        geom.mesh.numFaces(), // number of faces
        &geom.verts0.getPosition(0), // vertex positions array
        geom.mesh.getFaces().data() // face indices array
    );

    // compute mass properties
    vi.compute_inertia_tensor(density);

    // set mass and inertia
    rb.get_mut<Density>()->value = density;
    rb.get_mut<Mass>()->value = vi.getMass();
    rb.get_mut<InverseMass>()->value = (vi.getMass() != 0.0) ? (1.0 / vi.getMass()) : 0.0;
    rb.get_mut<LocalInertia>()->value = vi.getInertia();
    rb.get_mut<LocalInverseInertia>()->value = vi.getInertia().inverse();
    // what about world inertia

    // update center of mass position
    Vector3r com = vi.getCenterOfMass();
    rb.get_mut<Position>()->value -= com;
    rb.get_mut<OldPosition>()->value -= com;

    // adjust vertex positions relative to new center of mass
    for (unsigned int i = 0; i < geom.verts0.size(); i++) {
        geom.verts0.getPosition(i) -= com;
        geom.verts.getPosition(i) = geom.verts0.getPosition(i);
    }
}

// creates geometry component from raylib mesh and initializes mass properties
void geom_init_geometry_from_rlmesh(
    flecs::entity rb_entity, const rl::Mesh& rlMesh, float density) {
    // add geometry component if not exists
    if (!rb_entity.has<Geometry>()) {
        rb_entity.add<Geometry>();
    }
    auto geom = rb_entity.get_mut<Geometry>();

    // convert vertices
    std::vector<Vector3r> vertices;
    vertices.reserve(rlMesh.vertexCount);
    for (int i = 0; i < rlMesh.vertexCount; i++) {
        vertices.push_back(Vector3r(
            rlMesh.vertices[i * 3], rlMesh.vertices[i * 3 + 1], rlMesh.vertices[i * 3 + 2]));
    }

    // convert indices
    std::vector<unsigned int> indices;
    int num_triangles;
    if (rlMesh.indices) {
        num_triangles = rlMesh.triangleCount;
        indices.reserve(num_triangles * 3);
        for (int i = 0; i < num_triangles * 3; i++) {
            indices.push_back(static_cast<unsigned int>(rlMesh.indices[i]));
        }
    } else {
        num_triangles = rlMesh.vertexCount / 3;
        indices.reserve(num_triangles * 3);
        for (unsigned int i = 0; i < rlMesh.vertexCount; i++) {
            indices.push_back(i);
        }
    }

    // convert uvs
    Utilities::IndexedFaceMesh::UVIndices uv_indices;
    Utilities::IndexedFaceMesh::UVs uvs;
    if (rlMesh.texcoords) {
        uvs.reserve(rlMesh.vertexCount);
        for (int i = 0; i < rlMesh.vertexCount; i++) {
            uvs.push_back(Vector2r(rlMesh.texcoords[i * 2], rlMesh.texcoords[i * 2 + 1]));
        }
        uv_indices = indices;
    }

    // initialize mesh
    geom_init_mesh_(
        *geom,
        vertices.size(),
        num_triangles,
        vertices.data(),
        indices.data(),
        uv_indices,
        uvs,
        Vector3r(1.0f, 1.0f, 1.0f),
        false);

    // compute mass properties
    geom_compute_mass_properties_(rb_entity, *geom, density);
}

rl::Material geom_create_geometry_material() {
    // create a basic material
    rl::Material material = rl::LoadMaterialDefault();

    // set diffuse color (light gray with slight blue tint)
    material.maps[rl::MATERIAL_MAP_DIFFUSE].color = {220, 220, 240, 100};

    // set specular properties
    material.maps[rl::MATERIAL_MAP_SPECULAR].color = {255, 255, 255, 100}; // white specular

    // configure material properties
    material.maps[rl::MATERIAL_MAP_DIFFUSE].value = 0.9f; // diffuse strength
    material.maps[rl::MATERIAL_MAP_SPECULAR].value = 0.5f; // specular strength
    material.maps[rl::MATERIAL_MAP_METALNESS].value = 0.0f; // non-metallic
    material.maps[rl::MATERIAL_MAP_ROUGHNESS].value = 0.5f; // medium roughness
    material.maps[rl::MATERIAL_MAP_OCCLUSION].value = 1.0f; // no occlusion

    return material;
}
