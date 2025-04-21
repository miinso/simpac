#include "collider.h"
#include <unordered_map>
#include <memory>
#include <limits>
#include <cassert>
#include <cmath>
#include <functional>
// #include "../util.h"
#include "gjk.h"
#include "clipping.h"
#include "epa.h"

// Hash functions for Vector3r to use with unordered_map
namespace std {
    template<>
    struct hash<Vector3r> {
        size_t operator()(const Vector3r& v) const {
            size_t h1 = std::hash<Real>()(v.x());
            size_t h2 = std::hash<Real>()(v.y());
            size_t h3 = std::hash<Real>()(v.z());
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}

// Comparison for Vector3r
bool vector3r_compare(const Vector3r& a, const Vector3r& b) {
    constexpr Real EPSILON = 1e-6;
    return std::abs(a.x() - b.x()) < EPSILON && 
           std::abs(a.y() - b.y()) < EPSILON && 
           std::abs(a.z() - b.z()) < EPSILON;
}

Collider collider_sphere_create(const Real radius) {
    Collider collider;
    collider.type = COLLIDER_TYPE_SPHERE;
    collider.sphere.radius = radius;
    collider.sphere.center = Vector3r::Zero();
    return collider;
}

void collider_sphere_destroy(Collider* collider) {
    // Nothing to destroy for sphere collider
}

static Real get_sphere_collider_bounding_sphere_radius(const Collider* collider) {
    return collider->sphere.radius;
}

static bool do_triangles_share_same_vertex(const Vector3i& t1, const Vector3i& t2) {
    return t1.x() == t2.x() || t1.x() == t2.y() || t1.x() == t2.z() ||
           t1.y() == t2.x() || t1.y() == t2.y() || t1.y() == t2.z() ||
           t1.z() == t2.x() || t1.z() == t2.y() || t1.z() == t2.z();
}

static bool do_faces_share_same_vertex(const std::vector<unsigned int>& e1, const std::vector<unsigned int>& e2) {
    for (size_t i = 0; i < e1.size(); ++i) {
        unsigned int i1 = e1[i];
        for (size_t j = 0; j < e2.size(); ++j) {
            unsigned int i2 = e2[j];
            if (i1 == i2) {
                return true;
            }
        }
    }

    return false;
}

static void collect_faces_planar_to(const std::vector<Vector3r>& hull, 
                                    const std::vector<Vector3i>& hull_triangle_faces,
                                    const std::vector<std::vector<unsigned int>>& triangle_faces_to_neighbor_faces_map,
                                    std::vector<bool>& is_triangle_face_already_processed_arr, 
                                    unsigned int face_to_test_idx, 
                                    const Vector3r& target_normal, 
                                    std::vector<Vector3i>& out) {
    constexpr Real EPSILON = 0.000001;
    const Vector3i& face_to_test = hull_triangle_faces[face_to_test_idx];
    const Vector3r& v1 = hull[face_to_test.x()];
    const Vector3r& v2 = hull[face_to_test.y()];
    const Vector3r& v3 = hull[face_to_test.z()];
    
    Vector3r v12 = v2 - v1;
    Vector3r v13 = v3 - v1;
    Vector3r face_normal = v12.cross(v13).normalized();

    if (is_triangle_face_already_processed_arr[face_to_test_idx]) {
        return;
    }

    Real projection = face_normal.dot(target_normal);

    if ((projection - 1.0) > -EPSILON && (projection - 1.0) < EPSILON) {
        out.push_back(face_to_test);
        is_triangle_face_already_processed_arr[face_to_test_idx] = true;

        const std::vector<unsigned int>& neighbor_faces = triangle_faces_to_neighbor_faces_map[face_to_test_idx];

        for (size_t i = 0; i < neighbor_faces.size(); ++i) {
            unsigned int neighbor_face_idx = neighbor_faces[i];
            collect_faces_planar_to(hull, hull_triangle_faces, triangle_faces_to_neighbor_faces_map,
                is_triangle_face_already_processed_arr, neighbor_face_idx, target_normal, out);
        }
    }
}

static int get_edge_index(const std::vector<Vector2i>& edges, const Vector2i& edge) {
    for (int i = 0; i < edges.size(); ++i) {
        const Vector2i& current_edge = edges[i];
        if (current_edge.x() == edge.x() && current_edge.y() == edge.y()) {
            return i;
        }
        if (current_edge.x() == edge.y() && current_edge.y() == edge.x()) {
            return i;
        }
    }

    return -1;
}

static Collider_Convex_Hull_Face create_convex_hull_face(const std::vector<Vector3i>& triangles, const Vector3r& face_normal) {
    std::vector<Vector2i> edges;

    // Collect the edges that form the border of the face
    for (size_t i = 0; i < triangles.size(); ++i) {
        const Vector3i& triangle = triangles[i];

        Vector2i edge1(triangle.x(), triangle.y());
        Vector2i edge2(triangle.y(), triangle.z());
        Vector2i edge3(triangle.z(), triangle.x());

        int edge1_idx = get_edge_index(edges, edge1);

        if (edge1_idx >= 0) {
            edges.erase(edges.begin() + edge1_idx);
        } else {
            edges.push_back(edge1);
        }

        int edge2_idx = get_edge_index(edges, edge2);

        if (edge2_idx >= 0) {
            edges.erase(edges.begin() + edge2_idx);
        } else {
            edges.push_back(edge2);
        }

        int edge3_idx = get_edge_index(edges, edge3);

        if (edge3_idx >= 0) {
            edges.erase(edges.begin() + edge3_idx);
        } else {
            edges.push_back(edge3);
        }
    }

    // Nicely order the edges
    for (size_t i = 0; i < edges.size(); ++i) {
        Vector2i current_edge = edges[i];
        for (size_t j = i + 1; j < edges.size(); ++j) {
            Vector2i candidate_edge = edges[j];

            if (current_edge.y() != candidate_edge.x() && current_edge.y() != candidate_edge.y()) {
                continue;
            }

            if (current_edge.y() == candidate_edge.y()) {
                unsigned int tmp = candidate_edge.x();
                candidate_edge.x() = candidate_edge.y();
                candidate_edge.y() = tmp;
            }

            Vector2i tmp = edges[i + 1];
            edges[i + 1] = candidate_edge;
            edges[j] = tmp;
        }
    }

    assert(edges[0].x() == edges[edges.size() - 1].y());

    // Simply create the face elements based on the edges
    std::vector<unsigned int> face_elements;
    for (size_t i = 0; i < edges.size(); ++i) {
        const Vector2i& current_edge = edges[i];
        face_elements.push_back(current_edge.x());
    }

    Collider_Convex_Hull_Face face;
    face.elements = face_elements;
    face.normal = face_normal;
    return face;
}

static bool is_neighbor_already_in_vertex_to_neighbors_map(const std::vector<unsigned int>& vertex_to_neighbors, unsigned int neighbor) {
    for (size_t i = 0; i < vertex_to_neighbors.size(); ++i) {
        if (vertex_to_neighbors[i] == neighbor) {
            return true;
        }
    }

    return false;
}

static Real get_convex_hull_collider_bounding_sphere_radius(const Collider* collider) {
    Real max_distance = 0.0;
    for (size_t i = 0; i < collider->convex_hull.vertices.size(); ++i) {
        const Vector3r& v = collider->convex_hull.vertices[i];
        Real distance = v.norm();
        if (distance > max_distance) {
            max_distance = distance;
        }
    }

    return max_distance;
}

// Create a convex hull from the vertices+indices
// For now, we assume that the mesh is already a convex hull
// This function only makes sure that vertices are unique - duplicated vertices will be merged.
Collider collider_convex_hull_create(const std::vector<Vector3r>& vertices, const std::vector<unsigned int>& indices) {
    std::unordered_map<Vector3r, unsigned int, std::hash<Vector3r>> vertex_to_idx_map;
    std::vector<Vector3r> hull;
    
    // Build hull, eliminating duplicated vertex
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Vector3r& current_vertex = vertices[i];
        auto it = vertex_to_idx_map.find(current_vertex);
        if (it == vertex_to_idx_map.end()) {
            unsigned int current_index = hull.size();
            hull.push_back(current_vertex);
            vertex_to_idx_map[current_vertex] = current_index;
        }
    }

    // Collect all triangle faces that compose the just-built hull
    std::vector<Vector3i> hull_triangle_faces;
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i1 = indices[i];
        unsigned int i2 = indices[i + 1];
        unsigned int i3 = indices[i + 2];
        const Vector3r& v1 = vertices[i1];
        const Vector3r& v2 = vertices[i2];
        const Vector3r& v3 = vertices[i3];

        auto it1 = vertex_to_idx_map.find(v1);
        auto it2 = vertex_to_idx_map.find(v2);
        auto it3 = vertex_to_idx_map.find(v3);
        
        assert(it1 != vertex_to_idx_map.end());
        assert(it2 != vertex_to_idx_map.end());
        assert(it3 != vertex_to_idx_map.end());
        
        unsigned int new_i1 = it1->second;
        unsigned int new_i2 = it2->second;
        unsigned int new_i3 = it3->second;
        
        Vector3i triangle(new_i1, new_i2, new_i3);
        hull_triangle_faces.push_back(triangle);
    }

    // Prepare vertex to faces map
    std::vector<std::vector<unsigned int>> vertex_to_faces_map(hull.size());

    // Prepare vertex to neighbors map
    std::vector<std::vector<unsigned int>> vertex_to_neighbors_map(hull.size());

    // Prepare triangle faces to neighbors map
    std::vector<std::vector<unsigned int>> triangle_faces_to_neighbor_faces_map(hull_triangle_faces.size());

    // Create the vertex to neighbors map
    for (size_t i = 0; i < hull_triangle_faces.size(); ++i) {
        const Vector3i& triangle_face = hull_triangle_faces[i];

        for (size_t j = 0; j < hull_triangle_faces.size(); ++j) {
            if (i == j) {
                continue;
            }

            const Vector3i& face_to_test = hull_triangle_faces[j];
            if (do_triangles_share_same_vertex(triangle_face, face_to_test)) {
                triangle_faces_to_neighbor_faces_map[i].push_back(j);
            }
        }

        // Fill vertex to edges map
        if (!is_neighbor_already_in_vertex_to_neighbors_map(vertex_to_neighbors_map[triangle_face.x()], triangle_face.y())) {
            vertex_to_neighbors_map[triangle_face.x()].push_back(triangle_face.y());
        }
        if (!is_neighbor_already_in_vertex_to_neighbors_map(vertex_to_neighbors_map[triangle_face.x()], triangle_face.z())) {
            vertex_to_neighbors_map[triangle_face.x()].push_back(triangle_face.z());
        }
        if (!is_neighbor_already_in_vertex_to_neighbors_map(vertex_to_neighbors_map[triangle_face.y()], triangle_face.x())) {
            vertex_to_neighbors_map[triangle_face.y()].push_back(triangle_face.x());
        }
        if (!is_neighbor_already_in_vertex_to_neighbors_map(vertex_to_neighbors_map[triangle_face.y()], triangle_face.z())) {
            vertex_to_neighbors_map[triangle_face.y()].push_back(triangle_face.z());
        }
        if (!is_neighbor_already_in_vertex_to_neighbors_map(vertex_to_neighbors_map[triangle_face.z()], triangle_face.x())) {
            vertex_to_neighbors_map[triangle_face.z()].push_back(triangle_face.x());
        }
        if (!is_neighbor_already_in_vertex_to_neighbors_map(vertex_to_neighbors_map[triangle_face.z()], triangle_face.y())) {
            vertex_to_neighbors_map[triangle_face.z()].push_back(triangle_face.y());
        }
    }

    // Collect all 'de facto' faces of the convex hull
    std::vector<Collider_Convex_Hull_Face> faces;
    std::vector<bool> is_triangle_face_already_processed_arr(hull_triangle_faces.size(), false);

    for (size_t i = 0; i < hull_triangle_faces.size(); ++i) {
        if (is_triangle_face_already_processed_arr[i]) {
            continue;
        }

        const Vector3i& triangle_face = hull_triangle_faces[i];
        const Vector3r& v1 = hull[triangle_face.x()];
        const Vector3r& v2 = hull[triangle_face.y()];
        const Vector3r& v3 = hull[triangle_face.z()];

        Vector3r v12 = v2 - v1;
        Vector3r v13 = v3 - v1;
        Vector3r normal = v12.cross(v13).normalized();

        std::vector<Vector3i> planar_faces;
        collect_faces_planar_to(hull, hull_triangle_faces, triangle_faces_to_neighbor_faces_map,
            is_triangle_face_already_processed_arr, i, normal, planar_faces);

        Collider_Convex_Hull_Face new_face = create_convex_hull_face(planar_faces, normal);
        unsigned int new_face_index = faces.size();
        faces.push_back(new_face);

        // Fill vertex to faces map accordingly
        for (size_t j = 0; j < planar_faces.size(); ++j) {
            const Vector3i& planar_face = planar_faces[j];
            vertex_to_faces_map[planar_face.x()].push_back(new_face_index);
            vertex_to_faces_map[planar_face.y()].push_back(new_face_index);
            vertex_to_faces_map[planar_face.z()].push_back(new_face_index);
        }
    }

    // Prepare face to neighbors map
    std::vector<std::vector<unsigned int>> face_to_neighbor_faces_map(faces.size());

    // Fill faces to neighbor faces map
    for (size_t i = 0; i < faces.size(); ++i) {
        const Collider_Convex_Hull_Face& face = faces[i];

        for (size_t j = 0; j < faces.size(); ++j) {
            if (i == j) {
                continue;
            }

            const Collider_Convex_Hull_Face& candidate_face = faces[j];
            if (do_faces_share_same_vertex(face.elements, candidate_face.elements)) {
                face_to_neighbor_faces_map[i].push_back(j);
            }
        }
    }

    Collider_Convex_Hull convex_hull;
    convex_hull.faces = faces;
    convex_hull.transformed_faces = faces;
    convex_hull.vertices = hull;
    convex_hull.transformed_vertices = hull;
    convex_hull.vertex_to_faces = vertex_to_faces_map;
    convex_hull.vertex_to_neighbors = vertex_to_neighbors_map;
    convex_hull.face_to_neighbors = face_to_neighbor_faces_map;

    Collider collider;
    collider.type = COLLIDER_TYPE_CONVEX_HULL;
    collider.convex_hull = convex_hull;
    return collider;
}

static void collider_convex_hull_destroy(Collider* collider) {
    // With std::vector, everything is automatically cleaned up
    // No manual memory management needed
}

static void collider_destroy(Collider* collider) {
    switch (collider->type) {
        case COLLIDER_TYPE_CONVEX_HULL: {
            collider_convex_hull_destroy(collider);
        } break;
        case COLLIDER_TYPE_SPHERE: {
            collider_sphere_destroy(collider);
        } break;
    }
}

void colliders_destroy(std::vector<Collider>& colliders) {
    for (size_t i = 0; i < colliders.size(); ++i) {
        Collider* collider = &colliders[i];
        collider_destroy(collider);
    }
}

static void collider_update(Collider* collider, const Vector3r& translation, const Quaternionr* rotation) {
    switch (collider->type) {
        case COLLIDER_TYPE_CONVEX_HULL: {
            // Matrix4r model_matrix_no_scale = util_get_model_matrix_no_scale(rotation, translation);
            Matrix4r model_matrix_no_scale = Matrix4r::Identity();
            model_matrix_no_scale.block<3,3>(0,0) = rotation->toRotationMatrix();
            model_matrix_no_scale.block<3,1>(0,3) = translation;
            for (size_t i = 0; i < collider->convex_hull.transformed_vertices.size(); ++i) {
                Vector4r vertex(
                    collider->convex_hull.vertices[i].x(),
                    collider->convex_hull.vertices[i].y(),
                    collider->convex_hull.vertices[i].z(),
                    1.0
                );
                Vector4r transformed_vertex = model_matrix_no_scale * vertex;
                transformed_vertex /= transformed_vertex.w();
                collider->convex_hull.transformed_vertices[i] = transformed_vertex.head<3>();
            }

            for (size_t i = 0; i < collider->convex_hull.transformed_faces.size(); ++i) {
                const Vector3r& normal = collider->convex_hull.faces[i].normal;
                Vector3r transformed_normal = model_matrix_no_scale.block<3,3>(0,0) * normal;
                collider->convex_hull.transformed_faces[i].normal = transformed_normal.normalized();
            }
        } break;
        case COLLIDER_TYPE_SPHERE: {
            collider->sphere.center = translation;
        } break;
        default: {
            assert(false);
        } break;
    }
}

void colliders_update(std::vector<Collider>& colliders, const Vector3r& translation, const Quaternionr* rotation) {
    for (size_t i = 0; i < colliders.size(); ++i) {
        Collider* collider = &colliders[i];
        collider_update(collider, translation, rotation);
    }
}

// @TODO: We need to rewrite this function
Matrix3r colliders_get_default_inertia_tensor(const std::vector<Collider>& colliders, Real mass) {
    // For now, the center of mass is always assumed to be at 0,0,0
    if (colliders.size() == 1) {
        const Collider& collider = colliders[0];

        if (collider.type == COLLIDER_TYPE_SPHERE) {
            // for now we assume the sphere is centered at its center of mass (because then the inertia tensor is simple)
            assert(collider.sphere.center.isZero());

            Real I = (2.0 / 5.0) * mass * collider.sphere.radius * collider.sphere.radius;
            Matrix3r result = Matrix3r::Zero();
            result(0, 0) = I;
            result(1, 1) = I;
            result(2, 2) = I;
            return result;
        }
    }

    unsigned int total_num_vertices = 0;
    for (size_t i = 0; i < colliders.size(); ++i) {
        const Collider& collider = colliders[i];
        total_num_vertices += collider.convex_hull.vertices.size();
    }

    Real mass_per_vertex = mass / total_num_vertices;

    Matrix3r result = Matrix3r::Zero();
    for (size_t i = 0; i < colliders.size(); ++i) {
        const Collider& collider = colliders[i];
        assert(collider.type == COLLIDER_TYPE_CONVEX_HULL);

        for (size_t j = 0; j < collider.convex_hull.vertices.size(); ++j) {
            const Vector3r& v = collider.convex_hull.vertices[j];
            result(0, 0) += mass_per_vertex * (v.y() * v.y() + v.z() * v.z());
            result(0, 1) += mass_per_vertex * v.x() * v.y();
            result(0, 2) += mass_per_vertex * v.x() * v.z();
            result(1, 0) += mass_per_vertex * v.x() * v.y();
            result(1, 1) += mass_per_vertex * (v.x() * v.x() + v.z() * v.z());
            result(1, 2) += mass_per_vertex * v.y() * v.z();
            result(2, 0) += mass_per_vertex * v.x() * v.z();
            result(2, 1) += mass_per_vertex * v.y() * v.z();
            result(2, 2) += mass_per_vertex * (v.x() * v.x() + v.y() * v.y());
        }
    }

    return result;
}

static Real collider_get_bounding_sphere_radius(const Collider* collider) {
    switch (collider->type) {
        case COLLIDER_TYPE_CONVEX_HULL: {
            return get_convex_hull_collider_bounding_sphere_radius(collider);
        } break;
        case COLLIDER_TYPE_SPHERE: {
            return get_sphere_collider_bounding_sphere_radius(collider);
        } break;
    }

    assert(false);
    return 0.0;
}

Real colliders_get_bounding_sphere_radius(const std::vector<Collider>& colliders) {
    Real max_bounding_sphere_radius = -std::numeric_limits<Real>::max();
    for (size_t i = 0; i < colliders.size(); ++i) {
        const Collider* collider = &colliders[i];
        Real bounding_sphere_radius = collider_get_bounding_sphere_radius(collider);
        if (bounding_sphere_radius > max_bounding_sphere_radius) {
            max_bounding_sphere_radius = bounding_sphere_radius;
        }
    }

    return max_bounding_sphere_radius;
}

static void collider_get_contacts(Collider* collider1, Collider* collider2, std::vector<Collider_Contact>& contacts) {
    GJK_Simplex simplex;
    Real penetration;
    Vector3r normal;

    // If both colliders are spheres, calling EPA is not only extremely slow, but also provide bad results.
    // GJK is also not necessary. In this case, just calculate everything analytically.
    if (collider1->type == COLLIDER_TYPE_SPHERE && collider2->type == COLLIDER_TYPE_SPHERE) {
        Vector3r distance_vector = collider1->sphere.center - collider2->sphere.center;
        Real distance_sqd = distance_vector.dot(distance_vector);
        Real min_distance = collider1->sphere.radius + collider2->sphere.radius;
        if (distance_sqd < (min_distance * min_distance)) {
            // Spheres are colliding
            normal = (collider2->sphere.center - collider1->sphere.center).normalized();
            penetration = min_distance - sqrt(distance_sqd);
            clipping_get_contact_manifold(collider1, collider2, normal, penetration, contacts);
        }

        return;
    }

    // Call GJK to check if there is a collision
    if (gjk_collides(collider1, collider2, &simplex)) {
        // There is a collision.

        // Get the collision normal using EPA
        if (!epa(collider1, collider2, &simplex, normal, penetration)) {
            return;
        }

        // Finally, clip the results to get the result manifold
        clipping_get_contact_manifold(collider1, collider2, normal, penetration, contacts);
    }

    return;
}

std::vector<Collider_Contact> colliders_get_contacts(const std::vector<Collider>& colliders1, const std::vector<Collider>& colliders2) {
    std::vector<Collider_Contact> contacts;
    contacts.reserve(16);

    for (size_t i = 0; i < colliders1.size(); ++i) {
        Collider* collider1 = const_cast<Collider*>(&colliders1[i]);
        for (size_t j = 0; j < colliders2.size(); ++j) {
            Collider* collider2 = const_cast<Collider*>(&colliders2[j]);
            collider_get_contacts(collider1, collider2, contacts);
        }
    }

    return contacts;
}