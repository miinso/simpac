#include "epa.h"
#include <vector>
#include <limits>
#include <cassert>
#include "support.h"

static const Real EPSILON = 0.0001;

void polytope_from_gjk_simplex(const GJK_Simplex* s, std::vector<Vector3r>& polytope, std::vector<Vector3i>& faces) {
    assert(s->num == 4);
    polytope.reserve(4);
    faces.reserve(4);

    polytope.push_back(s->a);
    polytope.push_back(s->b);
    polytope.push_back(s->c);
    polytope.push_back(s->d);

    Vector3i i1(0, 1, 2); // ABC
    Vector3i i2(0, 2, 3); // ACD
    Vector3i i3(0, 3, 1); // ADB
    Vector3i i4(1, 2, 3); // BCD

    faces.push_back(i1);
    faces.push_back(i2);
    faces.push_back(i3);
    faces.push_back(i4);
}

void get_face_normal_and_distance_to_origin(const Vector3i& face, const std::vector<Vector3r>& polytope, Vector3r& normal, Real& distance) {
    const Vector3r& a = polytope[face.x()];
    const Vector3r& b = polytope[face.y()];
    const Vector3r& c = polytope[face.z()];

    Vector3r ab = b - a;
    Vector3r ac = c - a;
    normal = ab.cross(ac).normalized();

    assert(normal.x() != 0.0 || normal.y() != 0.0 || normal.z() != 0.0);

    // When this value is not 0, it is possible that the normals are not found even if the polytope is not degenerate
    const Real DISTANCE_TO_ORIGIN_TOLERANCE = 0.0000000000000;

    // the distance from the face's *plane* to the origin (considering an infinite plane).
    distance = normal.dot(a);
    if (distance < -DISTANCE_TO_ORIGIN_TOLERANCE) {
        // if the distance is less than 0, it means that our normal is point inwards instead of outwards
        // in this case, we just invert both normal and distance
        // this way, we don't need to worry about face's winding
        normal = -normal;
        distance = -distance;
    } else if (distance >= -DISTANCE_TO_ORIGIN_TOLERANCE && distance <= DISTANCE_TO_ORIGIN_TOLERANCE) {
        // if the distance is exactly 0.0, then it means that the origin is lying exactly on the face.
        // in this case, we can't directly infer the orientation of the normal.
        // since our shape is convex, we analyze the other vertices of the hull to deduce the orientation
        bool was_able_to_calculate_normal = false;
        for (size_t i = 0; i < polytope.size(); ++i) {
            const Vector3r& current = polytope[i];
            Real auxiliar_distance = normal.dot(current);
            if (auxiliar_distance < -DISTANCE_TO_ORIGIN_TOLERANCE || auxiliar_distance > DISTANCE_TO_ORIGIN_TOLERANCE) {
                // since the shape is convex, the other vertices should always be "behind" the normal plane
                normal = auxiliar_distance < -DISTANCE_TO_ORIGIN_TOLERANCE ? normal : -normal;
                was_able_to_calculate_normal = true;
                break;
            }
        }

        // If we were not able to calculate the normal, it means that ALL points of the polytope are in the same plane
        // Therefore, we either have a degenerate polytope or our tolerance is not big enough
        assert(was_able_to_calculate_normal);
    }
}

void add_edge(std::vector<Vector2i>& edges, const Vector2i& edge, const std::vector<Vector3r>& polytope) {
    // @TODO: we can use a hash table here
    for (size_t i = 0; i < edges.size(); ++i) {
        const Vector2i& current = edges[i];
        if (edge.x() == current.x() && edge.y() == current.y()) {
            edges.erase(edges.begin() + i);
            return;
        }
        if (edge.x() == current.y() && edge.y() == current.x()) {
            edges.erase(edges.begin() + i);
            return;
        }

        // @TEMPORARY: Once indexes point to unique vertices, this won't be needed.
        const Vector3r& current_v1 = polytope[current.x()];
        const Vector3r& current_v2 = polytope[current.y()];
        const Vector3r& edge_v1 = polytope[edge.x()];
        const Vector3r& edge_v2 = polytope[edge.y()];

        if (current_v1 == edge_v1 && current_v2 == edge_v2) {
            edges.erase(edges.begin() + i);
            return;
        }

        if (current_v1 == edge_v2 && current_v2 == edge_v1) {
            edges.erase(edges.begin() + i);
            return;
        }
    }

    edges.push_back(edge);
}

static Vector3r triangle_centroid(const Vector3r& p1, const Vector3r& p2, const Vector3r& p3) {
    Vector3r centroid = p1 + p2 + p3;
    return centroid / 3.0;
}

bool epa(Collider* collider1, Collider* collider2, GJK_Simplex* simplex, Vector3r& normal, Real& penetration) {
    std::vector<Vector3r> polytope;
    std::vector<Vector3i> faces;

    // build initial polytope from GJK simplex
    polytope_from_gjk_simplex(simplex, polytope, faces);

    std::vector<Vector3r> normals;
    std::vector<Real> faces_distance_to_origin;
    normals.reserve(128);
    faces_distance_to_origin.reserve(128);

    Vector3r min_normal;
    Real min_distance = std::numeric_limits<Real>::max();

    for (size_t i = 0; i < faces.size(); ++i) {
        Vector3r face_normal;
        Real distance;
        const Vector3i& face = faces[i];

        get_face_normal_and_distance_to_origin(face, polytope, face_normal, distance);

        normals.push_back(face_normal);
        faces_distance_to_origin.push_back(distance);

        if (distance < min_distance) {
            min_distance = distance;
            min_normal = face_normal;
        }
    }

    std::vector<Vector2i> edges;
    edges.reserve(1024);
    bool converged = false;
    for (unsigned int it = 0; it < 100; ++it) {
        Vector3r support_point = support_point_of_minkowski_difference(collider1, collider2, min_normal);

        // If the support time lies on the face currently set as the closest to the origin, we are done.
        Real d = min_normal.dot(support_point);
        if (std::abs(d - min_distance) < EPSILON) {
            normal = min_normal;
            penetration = min_distance;
            converged = true;
            break;
        }

        // add new point to polytope
        unsigned int new_point_index = polytope.size();
        polytope.push_back(support_point);

        // Expand Polytope
        for (size_t i = 0; i < normals.size(); ) {
            const Vector3r& face_normal = normals[i];
            const Vector3i& face = faces[i];

            // If the face normal points towards the support point, we need to reconstruct it.
            Vector3r centroid = triangle_centroid(
                polytope[face.x()],
                polytope[face.y()],
                polytope[face.z()]
            );

            // If the face normal points towards the support point, we need to reconstruct it.
            if (face_normal.dot(support_point - centroid) > 0.0) {
                Vector2i edge1(face.x(), face.y());
                Vector2i edge2(face.y(), face.z());
                Vector2i edge3(face.z(), face.x());

                add_edge(edges, edge1, polytope);
                add_edge(edges, edge2, polytope);
                add_edge(edges, edge3, polytope);

                // Relative order between the two arrays should be kept.
                faces.erase(faces.begin() + i);
                faces_distance_to_origin.erase(faces_distance_to_origin.begin() + i);
                normals.erase(normals.begin() + i);
            } else {
                ++i;
            }
        }

        for (size_t i = 0; i < edges.size(); ++i) {
            const Vector2i& edge = edges[i];
            Vector3i new_face;
            new_face.x() = edge.x();
            new_face.y() = edge.y();
            new_face.z() = new_point_index;
            faces.push_back(new_face);

            Vector3r new_face_normal;
            Real new_face_distance;
            get_face_normal_and_distance_to_origin(new_face, polytope, new_face_normal, new_face_distance);

            normals.push_back(new_face_normal);
            faces_distance_to_origin.push_back(new_face_distance);
        }

        min_distance = std::numeric_limits<Real>::max();
        for (size_t i = 0; i < faces_distance_to_origin.size(); ++i) {
            Real distance = faces_distance_to_origin[i];
            if (distance < min_distance) {
                min_distance = distance;
                min_normal = normals[i];
            }
        }

        edges.clear();
    }

    if (!converged) {
        // EPA did not converge
    }

    return converged;
}