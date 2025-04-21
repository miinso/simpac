#include "clipping.h"
#include <vector>
#include <cmath>
#include <cassert>
#include <limits>
#include <algorithm>
#include "gjk.h"
#include "support.h"

struct Plane {
    Vector3r normal;
    Vector3r point;
};

static bool is_point_in_plane(const Plane* plane, const Vector3r& position) {
    Real distance = -plane->normal.dot(plane->point);
    if (position.dot(plane->normal) + distance < 0.0) {
        return false;
    }

    return true;
}

static bool plane_edge_intersection(const Plane* plane, const Vector3r& start, const Vector3r& end, Vector3r& out_point) {
    const Real EPSILON = 0.000001;
    Vector3r ab = end - start;

    // Check that the edge and plane are not parallel and thus never intersect
    // We do this by projecting the line (start - A, End - B) ab along the plane
    Real ab_p = plane->normal.dot(ab);
    if (std::abs(ab_p) > EPSILON) {
        // Generate a random point on the plane (any point on the plane will suffice)
        Real distance = -plane->normal.dot(plane->point);
        Vector3r p_co = -distance * plane->normal;

        // Work out the edge factor to scale edge by
        // e.g. how far along the edge to traverse before it meets the plane.
        // This is computed by: -proj<plane_nrml>(edge_start - any_planar_point) / proj<plane_nrml>(edge_start - edge_end)
        Real fac = -plane->normal.dot(start - p_co) / ab_p;

        // Stop any large floating point divide issues with almost parallel planes
        fac = std::min(std::max(fac, 0.0), 1.0); 

        // Return point on edge
        out_point = start + fac * ab;
        return true;
    }

    return false;
}

// Clips the input polygon to the input clip planes
// If remove_instead_of_clipping is true, vertices that are lying outside the clipping planes will be removed instead of clipped
// Based on https://research.ncl.ac.uk/game/mastersdegree/gametechnologies/previousinformation/physics5collisionmanifolds/
static void sutherland_hodgman(const std::vector<Vector3r>& input_polygon, int num_clip_planes, const Plane* clip_planes, 
                              std::vector<Vector3r>& out_polygon, bool remove_instead_of_clipping) {
    assert(!out_polygon.empty());
    assert(num_clip_planes > 0);

    // Create temporary list of vertices
    // We will keep ping-pong'ing between the two lists updating them as we go.
    std::vector<Vector3r> input(input_polygon);
    std::vector<Vector3r> output;
    output.reserve(4 * input.size());

    for (int i = 0; i < num_clip_planes; ++i) {
        // If every single point has already been removed previously, just exit
        if (input.empty()) {
            break;
        }

        const Plane* plane = &clip_planes[i];

        // Loop through each edge of the polygon and clip that edge against the current plane.
        Vector3r temp_point, start_point = input[input.size() - 1];
        for (size_t j = 0; j < input.size(); ++j) {
            Vector3r end_point = input[j];
            bool start_in_plane = is_point_in_plane(plane, start_point);
            bool end_in_plane = is_point_in_plane(plane, end_point);

            if (remove_instead_of_clipping) {
                if (end_in_plane) {
                    output.push_back(end_point);
                }
            } else {
                // If the edge is entirely within the clipping plane, keep it as it is
                if (start_in_plane && end_in_plane) {
                    output.push_back(end_point);
                }
                // If the edge interesects the clipping plane, cut the edge along clip plane
                else if (start_in_plane && !end_in_plane) {
                    if (plane_edge_intersection(plane, start_point, end_point, temp_point)) {
                        output.push_back(temp_point);
                    }
                } else if (!start_in_plane && end_in_plane) {
                    if (plane_edge_intersection(plane, start_point, end_point, temp_point)) {
                        output.push_back(temp_point);
                    }

                    output.push_back(end_point);
                }
            }
            // ..otherwise the edge is entirely outside the clipping plane and should be removed/ignored

            start_point = end_point;
        }

        // Swap input/output polygons, and clear output list for us to generate afresh
        input.swap(output);
        output.clear();
    }

    out_polygon = input;
}

static Vector3r get_closest_point_polygon(const Vector3r& position, Plane* reference_plane) {
    Real d = (-reference_plane->normal).dot(reference_plane->point);
    return position - (reference_plane->normal.dot(position) + d) * reference_plane->normal;
}

static std::vector<Plane> build_boundary_planes(Collider_Convex_Hull* convex_hull, unsigned int target_face_idx) {
    std::vector<Plane> result;
    result.reserve(16);
    
    const std::vector<unsigned int>& face_neighbors = convex_hull->face_to_neighbors[target_face_idx];

    for (size_t i = 0; i < face_neighbors.size(); ++i) {
        Collider_Convex_Hull_Face neighbor_face = convex_hull->transformed_faces[face_neighbors[i]];
        Plane p;
        p.point = convex_hull->transformed_vertices[neighbor_face.elements[0]];
        p.normal = -neighbor_face.normal;
        result.push_back(p);
    }

    return result;
}

static unsigned int get_face_with_most_fitting_normal(unsigned int support_idx, const Collider_Convex_Hull* convex_hull, const Vector3r& normal) {
    const Real EPSILON = 0.000001;
    const std::vector<unsigned int>& support_faces = convex_hull->vertex_to_faces[support_idx];

    Real max_proj = -std::numeric_limits<Real>::max();
    unsigned int selected_face_idx = 0;
    
    for (size_t i = 0; i < support_faces.size(); ++i) {
        const Collider_Convex_Hull_Face& face = convex_hull->transformed_faces[support_faces[i]];
        Real proj = face.normal.dot(normal);
        if (proj > max_proj) {
            max_proj = proj;
            selected_face_idx = support_faces[i];
        }
    }

    return selected_face_idx;
}

static Vector4i get_edge_with_most_fitting_normal(unsigned int support1_idx, unsigned int support2_idx, 
                                                 const Collider_Convex_Hull* convex_hull1,
                                                 const Collider_Convex_Hull* convex_hull2, 
                                                 const Vector3r& normal, Vector3r& edge_normal) {
    Vector3r inverted_normal = -normal;

    const Vector3r& support1 = convex_hull1->transformed_vertices[support1_idx];
    const Vector3r& support2 = convex_hull2->transformed_vertices[support2_idx];

    const std::vector<unsigned int>& support1_neighbors = convex_hull1->vertex_to_neighbors[support1_idx];
    const std::vector<unsigned int>& support2_neighbors = convex_hull2->vertex_to_neighbors[support2_idx];

    Real max_dot = -std::numeric_limits<Real>::max();
    Vector4i selected_edges;

    for (size_t i = 0; i < support1_neighbors.size(); ++i) {
        const Vector3r& neighbor1 = convex_hull1->transformed_vertices[support1_neighbors[i]];
        Vector3r edge1 = support1 - neighbor1;
        
        for (size_t j = 0; j < support2_neighbors.size(); ++j) {
            const Vector3r& neighbor2 = convex_hull2->transformed_vertices[support2_neighbors[j]];
            Vector3r edge2 = support2 - neighbor2;

            Vector3r current_normal = edge1.cross(edge2).normalized();
            Vector3r current_normal_inverted = -current_normal;

            Real dot = current_normal.dot(normal);
            if (dot > max_dot) {
                max_dot = dot;
                selected_edges[0] = support1_idx;
                selected_edges[1] = support1_neighbors[i];
                selected_edges[2] = support2_idx;
                selected_edges[3] = support2_neighbors[j];
                edge_normal = current_normal;
            }

            dot = current_normal_inverted.dot(normal);
            if (dot > max_dot) {
                max_dot = dot;
                selected_edges[0] = support1_idx;
                selected_edges[1] = support1_neighbors[i];
                selected_edges[2] = support2_idx;
                selected_edges[3] = support2_neighbors[j];
                edge_normal = current_normal_inverted;
            }
        }
    }

    return selected_edges;
}

// This function calculates the distance between two indepedent skew lines in the 3D world
// The first line is given by a known point P1 and a direction vector D1
// The second line is given by a known point P2 and a direction vector D2
// Outputs:
// L1 is the closest POINT to the second line that belongs to the first line
// L2 is the closest POINT to the first line that belongs to the second line
// _N is the number that satisfies L1 = P1 + _N * D1
// _M is the number that satisfies L2 = P2 + _M * D2
static bool collision_distance_between_skew_lines(const Vector3r& p1, const Vector3r& d1, 
                                                 const Vector3r& p2, const Vector3r& d2, 
                                                 Vector3r* l1, Vector3r* l2, Real* _n, Real* _m) {
    Real n1 = d1.x() * d2.x() + d1.y() * d2.y() + d1.z() * d2.z();
    Real n2 = d2.x() * d2.x() + d2.y() * d2.y() + d2.z() * d2.z();
    Real m1 = -d1.x() * d1.x() - d1.y() * d1.y() - d1.z() * d1.z();
    Real m2 = -d2.x() * d1.x() - d2.y() * d1.y() - d2.z() * d1.z();
    Real r1 = -d1.x() * p2.x() + d1.x() * p1.x() - d1.y() * p2.y() + d1.y() * p1.y() - d1.z() * p2.z() + d1.z() * p1.z();
    Real r2 = -d2.x() * p2.x() + d2.x() * p1.x() - d2.y() * p2.y() + d2.y() * p1.y() - d2.z() * p2.z() + d2.z() * p1.z();

    // Solve 2x2 linear system
    if ((n1 * m2) - (n2 * m1) == 0) {
        return false;
    }
    Real n = ((r1 * m2) - (r2 * m1)) / ((n1 * m2) - (n2 * m1));
    Real m = ((n1 * r2) - (n2 * r1)) / ((n1 * m2) - (n2 * m1));

    if (l1) {
        *l1 = p1 + m * d1;
    }
    if (l2) {
        *l2 = p2 + n * d2;
    }
    if (_n) {
        *_n = n;
    }
    if (_m) {
        *_m = m;
    }

    return true;
}

static std::vector<Vector3r> get_vertices_of_faces(Collider_Convex_Hull* hull, const Collider_Convex_Hull_Face& face) {
    std::vector<Vector3r> vertices;
    vertices.reserve(face.elements.size());
    
    for (size_t i = 0; i < face.elements.size(); ++i) {
        vertices.push_back(hull->transformed_vertices[face.elements[i]]);
    }
    
    return vertices;
}

void convex_convex_contact_manifold(Collider* collider1, Collider* collider2, const Vector3r& normal, std::vector<Collider_Contact>& contacts) {
    assert(collider1->type == COLLIDER_TYPE_CONVEX_HULL);
    assert(collider2->type == COLLIDER_TYPE_CONVEX_HULL);
    Collider_Convex_Hull* convex_hull1 = &collider1->convex_hull;
    Collider_Convex_Hull* convex_hull2 = &collider2->convex_hull;

    const Real EPSILON = 0.0001;

    Vector3r inverted_normal = -normal;

    Vector3r edge_normal;
    unsigned int support1_idx = support_point_get_index(convex_hull1, normal);
    unsigned int support2_idx = support_point_get_index(convex_hull2, inverted_normal);
    unsigned int face1_idx = get_face_with_most_fitting_normal(support1_idx, convex_hull1, normal);
    unsigned int face2_idx = get_face_with_most_fitting_normal(support2_idx, convex_hull2, inverted_normal);
    Collider_Convex_Hull_Face face1 = convex_hull1->transformed_faces[face1_idx];
    Collider_Convex_Hull_Face face2 = convex_hull2->transformed_faces[face2_idx];
    Vector4i edges = get_edge_with_most_fitting_normal(support1_idx, support2_idx, convex_hull1, convex_hull2, normal, edge_normal);

    Real chosen_normal1_dot = face1.normal.dot(normal);
    Real chosen_normal2_dot = face2.normal.dot(inverted_normal);
    Real edge_normal_dot = edge_normal.dot(normal);

    if (edge_normal_dot > chosen_normal1_dot + EPSILON && edge_normal_dot > chosen_normal2_dot + EPSILON) {
        // EDGE collision case
        Vector3r l1, l2;
        Vector3r p1 = convex_hull1->transformed_vertices[edges[0]];
        Vector3r d1 = convex_hull1->transformed_vertices[edges[1]] - p1;
        Vector3r p2 = convex_hull2->transformed_vertices[edges[2]];
        Vector3r d2 = convex_hull2->transformed_vertices[edges[3]] - p2;
        
        assert(collision_distance_between_skew_lines(p1, d1, p2, d2, &l1, &l2, nullptr, nullptr));
        
        Collider_Contact contact;
        contact.collision_point1 = l1;
        contact.collision_point2 = l2; 
        contact.normal = normal;
        contacts.push_back(contact);
    } else {
        // FACE collision case
        bool is_face1_the_reference_face = chosen_normal1_dot > chosen_normal2_dot;
        std::vector<Vector3r> reference_face_support_points = is_face1_the_reference_face ?
            get_vertices_of_faces(convex_hull1, face1) : get_vertices_of_faces(convex_hull2, face2);
        std::vector<Vector3r> incident_face_support_points = is_face1_the_reference_face ?
            get_vertices_of_faces(convex_hull2, face2) : get_vertices_of_faces(convex_hull1, face1);

        std::vector<Plane> boundary_planes = is_face1_the_reference_face ? 
            build_boundary_planes(convex_hull1, face1_idx) : build_boundary_planes(convex_hull2, face2_idx);

        std::vector<Vector3r> clipped_points;
        sutherland_hodgman(incident_face_support_points, boundary_planes.size(), boundary_planes.data(), clipped_points, false);

        Plane reference_plane;
        reference_plane.normal = is_face1_the_reference_face ? -face1.normal : -face2.normal;
        reference_plane.point = reference_face_support_points[0];

        std::vector<Vector3r> final_clipped_points;
        sutherland_hodgman(clipped_points, 1, &reference_plane, final_clipped_points, true);

        for (size_t i = 0; i < final_clipped_points.size(); ++i) {
            const Vector3r& point = final_clipped_points[i];
            Vector3r closest_point = get_closest_point_polygon(point, &reference_plane);
            Vector3r point_diff = point - closest_point;
            Real contact_penetration;

            // we are projecting the points that are in the incident face on the reference planes
            // so the points that we have are part of the incident object.
            Collider_Contact contact;
            if (is_face1_the_reference_face) {
                contact_penetration = point_diff.dot(normal);
                contact.collision_point1 = point - contact_penetration * normal;
                contact.collision_point2 = point;
            } else {
                contact_penetration = -point_diff.dot(normal);
                contact.collision_point1 = point;
                contact.collision_point2 = point + contact_penetration * normal;
            }

            contact.normal = normal;

            if (contact_penetration < 0.0) {
                contacts.push_back(contact);
            }
        }
    }

    if (contacts.empty()) {
        // Warning: no intersection was found
    }
}

void clipping_get_contact_manifold(Collider* collider1, Collider* collider2, const Vector3r& normal, Real penetration,
    std::vector<Collider_Contact>& contacts) {
    // TODO: For now, we only consider CONVEX and SPHERE colliders.
    // If new colliders are added, we can think about making this more generic.

    if (collider1->type == COLLIDER_TYPE_SPHERE) {
        Vector3r sphere_collision_point = support_point(collider1, normal);

        Collider_Contact contact;
        contact.collision_point1 = sphere_collision_point;
        contact.collision_point2 = sphere_collision_point - penetration * normal;
        contact.normal = normal;
        contacts.push_back(contact);
    } else if (collider2->type == COLLIDER_TYPE_SPHERE) {
        Vector3r inverse_normal = -normal;
        Vector3r sphere_collision_point = support_point(collider2, inverse_normal);

        Collider_Contact contact;
        contact.collision_point1 = sphere_collision_point + penetration * normal;
        contact.collision_point2 = sphere_collision_point;
        contact.normal = normal;
        contacts.push_back(contact);
    } else {
        // For now, this case must be convex-convex
        assert(collider1->type == COLLIDER_TYPE_CONVEX_HULL);
        assert(collider2->type == COLLIDER_TYPE_CONVEX_HULL);
        convex_convex_contact_manifold(collider1, collider2, normal, contacts);
    }
}