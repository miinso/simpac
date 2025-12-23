// geometry/components.hpp
#pragma once

#include "IndexedFaceMesh.h"
#include "ParticleData.h"
#include "geometry/types.hpp"
#include "raylib.h"
#include "raymath.h"

#include "rlgl.h"
#include <memory>
#include <vector>

namespace phys {
    namespace components {

        struct MeshFilter {
            Utilities::IndexedFaceMesh mesh;
            PBD::VertexData local_vertices;
            PBD::VertexData world_vertices;

            MeshFilter() { }

            void init_mesh(const unsigned int num_verts,
                           const unsigned int num_faces,
                           const Vector3r* vertices,
                           const unsigned int* indices,
                           const Utilities::IndexedFaceMesh::UVIndices& uv_indices,
                           const Utilities::IndexedFaceMesh::UVs& uvs,
                           const Vector3r& scale,
                           const bool use_flat_shading) {
                mesh.release();
                mesh.initMesh(num_verts, num_faces * 2, num_faces);
                local_vertices.resize(num_verts);
                world_vertices.resize(num_verts);
                mesh.setFlatShading(use_flat_shading);

                // set vertex positions with scaling
                for (unsigned int i = 0; i < num_verts; i++) {
                    local_vertices.getPosition(i) = vertices[i];
                    world_vertices.getPosition(i) = local_vertices.getPosition(i);
                }

                // add faces
                for (unsigned int i = 0; i < num_faces; i++) {
                    mesh.addFace(&indices[3 * i]);
                }

                // setup UVs and build topology
                mesh.copyUVs(uv_indices, uvs);
                mesh.buildNeighbors();
            }

            void init_mesh(const Mesh& m,
                           const Vector3r& scale = Vector3r::Identity(),
                           bool use_flat_shading = false) {
                // convert vertices
                std::vector<Vector3r> vertices;
                vertices.reserve(m.vertexCount);
                for (int i = 0; i < m.vertexCount; i++) {
                    vertices.push_back(
                        Vector3r(m.vertices[i * 3], m.vertices[i * 3 + 1], m.vertices[i * 3 + 2]));
                }

                // convert indices
                std::vector<unsigned int> indices;
                int num_triangles;
                if (m.indices) {
                    num_triangles = m.triangleCount;
                    indices.reserve(num_triangles * 3);
                    for (int i = 0; i < num_triangles * 3; i++) {
                        indices.push_back(static_cast<unsigned int>(m.indices[i]));
                    }
                } else {
                    num_triangles = m.vertexCount / 3;
                    indices.reserve(num_triangles * 3);
                    for (unsigned int i = 0; i < m.vertexCount; i++) {
                        indices.push_back(i);
                    }
                }

                // convert uvs
                Utilities::IndexedFaceMesh::UVIndices uv_indices;
                Utilities::IndexedFaceMesh::UVs uvs;
                if (m.texcoords) {
                    uvs.reserve(m.vertexCount);
                    for (int i = 0; i < m.vertexCount; i++) {
                        uvs.push_back(Vector2r(m.texcoords[i * 2], m.texcoords[i * 2 + 1]));
                    }
                    uv_indices = indices;
                }

                init_mesh(vertices.size(),
                          num_triangles,
                          vertices.data(),
                          indices.data(),
                          uv_indices,
                          uvs,
                          scale,
                          use_flat_shading);
            }

            void transform_vertices(const Vector3r& position,
                                    const Quaternionr& orientation,
                                    const Vector3r& scale,
                                    const MeshFilter& filter) {
                Matrix3r R = orientation.matrix();

                for (unsigned int i = 0; i < local_vertices.size(); ++i) {
                    world_vertices.getPosition(i) = R * local_vertices.getPosition(i) + position;
                }

                mesh.updateNormals(world_vertices, 0);
                mesh.updateVertexNormals(world_vertices);
            }

            size_t get_vertex_count() const {
                return local_vertices.size();
            }
            size_t get_face_count() const {
                return mesh.numFaces();
            }
            size_t get_edge_count() const {
                return mesh.numEdges();
            }
        };

        struct MeshRenderer {
            Mesh renderable;
            bool is_dirty = true;
            bool is_initialized = false;

            ~MeshRenderer() {
                release_resources();
            }

            void release_resources() {
                if (renderable.vboId != nullptr) {
                    UnloadMesh(renderable);
                    renderable = {0};
                    is_initialized = false;
                }
            }

            void initialize_renderable(const MeshFilter& filter) {
                if (is_initialized)
                    return;

                const auto& mesh = filter.mesh;
                const auto& vertices = filter.world_vertices;
                const auto& indices = mesh.getFaces();
                const auto& uvs = mesh.getUVs();

                renderable = {0};
                renderable.vertexCount = vertices.size();
                renderable.triangleCount = indices.size() / 3;

                renderable.vertices = new float[renderable.vertexCount * 3];
                renderable.normals = new float[renderable.vertexCount * 3];
                renderable.texcoords = new float[renderable.vertexCount * 2];
                renderable.indices = new unsigned short[mesh.numFaces() * 3];

                const auto& verts = vertices.getVertices();
                for (int i = 0; i < renderable.vertexCount; i++) {
                    renderable.vertices[i * 3 + 0] = verts[i].x();
                    renderable.vertices[i * 3 + 1] = verts[i].y();
                    renderable.vertices[i * 3 + 2] = verts[i].z();
                }

                const auto& normals = mesh.getVertexNormals();
                for (int i = 0; i < renderable.vertexCount; i++) {
                    renderable.normals[i * 3 + 0] = normals[i].x();
                    renderable.normals[i * 3 + 1] = normals[i].y();
                    renderable.normals[i * 3 + 2] = normals[i].z();
                }

                if (!uvs.empty()) {
                    for (int i = 0; i < renderable.vertexCount; ++i) {
                        renderable.texcoords[i * 2 + 0] = uvs[i].x();
                        renderable.texcoords[i * 2 + 1] = uvs[i].y();
                    }
                }

                for (size_t i = 0; i < indices.size(); ++i) {
                    renderable.indices[i] = static_cast<unsigned short>(indices[i]);
                }

                UploadMesh(&renderable, true);

                is_initialized = true;
                is_dirty = false;
            }

            void update_renderable(const MeshFilter& filter) {
                if (!is_initialized) {
                    initialize_renderable(filter);
                    return;
                }

                // if (!is_dirty)
                //     return;

                // update raylib mesh buffers
                const auto& vertices = filter.world_vertices.getVertices();
                const auto& normals = filter.mesh.getVertexNormals();

                // copy new vertex positions
                for (size_t i = 0; i < vertices.size(); i++) {
                    renderable.vertices[i * 3 + 0] = vertices[i].x();
                    renderable.vertices[i * 3 + 1] = vertices[i].y();
                    renderable.vertices[i * 3 + 2] = vertices[i].z();
                }

                // copy new normals
                for (size_t i = 0; i < normals.size(); i++) {
                    renderable.normals[i * 3 + 0] = normals[i].x();
                    renderable.normals[i * 3 + 1] = normals[i].y();
                    renderable.normals[i * 3 + 2] = normals[i].z();
                }

                UpdateMeshBuffer(renderable,
                                 SHADER_LOC_VERTEX_POSITION,
                                 renderable.vertices,
                                 renderable.vertexCount * 3 * sizeof(float),
                                 0);

                UpdateMeshBuffer(renderable,
                                 SHADER_LOC_VERTEX_NORMAL,
                                 renderable.normals,
                                 renderable.vertexCount * 3 * sizeof(float),
                                 0);

                is_dirty = false;
            }

            // void draw(Material& mat, Matrix& transform) const {
            void draw() const {
                if (renderable.vboId != nullptr) {
                    DrawMesh(renderable, LoadMaterialDefault(), MatrixIdentity());
                }
            }

            void draw_wireframe(const Color& color = BLUE) const {
                if (renderable.vboId == nullptr)
                    return;

                rlPushMatrix();
                rlBegin(RL_LINES);
                rlColor4ub(color.r, color.g, color.b, color.a);

                for (int i = 0; i < renderable.triangleCount * 3; i += 3) {
                    unsigned short i1 = renderable.indices[i];
                    unsigned short i2 = renderable.indices[i + 1];
                    unsigned short i3 = renderable.indices[i + 2];

                    rlVertex3f(renderable.vertices[i1 * 3],
                               renderable.vertices[i1 * 3 + 1],
                               renderable.vertices[i1 * 3 + 2]);
                    rlVertex3f(renderable.vertices[i2 * 3],
                               renderable.vertices[i2 * 3 + 1],
                               renderable.vertices[i2 * 3 + 2]);

                    rlVertex3f(renderable.vertices[i2 * 3],
                               renderable.vertices[i2 * 3 + 1],
                               renderable.vertices[i2 * 3 + 2]);
                    rlVertex3f(renderable.vertices[i3 * 3],
                               renderable.vertices[i3 * 3 + 1],
                               renderable.vertices[i3 * 3 + 2]);

                    rlVertex3f(renderable.vertices[i3 * 3],
                               renderable.vertices[i3 * 3 + 1],
                               renderable.vertices[i3 * 3 + 2]);
                    rlVertex3f(renderable.vertices[i1 * 3],
                               renderable.vertices[i1 * 3 + 1],
                               renderable.vertices[i1 * 3 + 2]);
                }
                rlEnd();
                rlPopMatrix();
            }

            void draw_vertex_normals(float scale = 1.0f, const Color& color = GREEN) const {
                if (renderable.vboId == nullptr)
                    return;

                rlPushMatrix();
                rlBegin(RL_LINES);
                rlColor4ub(color.r, color.g, color.b, color.a);

                for (int i = 0; i < renderable.vertexCount; i++) {
                    Vector3r pos(renderable.vertices[i * 3],
                                 renderable.vertices[i * 3 + 1],
                                 renderable.vertices[i * 3 + 2]);
                    Vector3r normal(renderable.normals[i * 3],
                                    renderable.normals[i * 3 + 1],
                                    renderable.normals[i * 3 + 2]);

                    Vector3r end = pos + normal * scale;

                    rlVertex3f(pos.x(), pos.y(), pos.z());
                    rlVertex3f(end.x(), end.y(), end.z());
                }
                rlEnd();
                rlPopMatrix();
            }
        };
    } // namespace components
} // namespace phys
