#include "gjk.h"
#include <vector>
#include <cmath>
#include <limits>
#include <cassert>
#include "support.h"

static void add_to_simplex(GJK_Simplex* simplex, const Vector3r& point) {
    switch (simplex->num) {
        case 1: {
            simplex->b = simplex->a;
            simplex->a = point;
        } break;
        case 2: {
            simplex->c = simplex->b;
            simplex->b = simplex->a;
            simplex->a = point;
        } break;
        case 3: {
            simplex->d = simplex->c;
            simplex->c = simplex->b;
            simplex->b = simplex->a;
            simplex->a = point;
        } break;
        default: {
            assert(false);
        } break;
    }

    ++simplex->num;
}

static Vector3r triple_cross(const Vector3r& a, const Vector3r& b, const Vector3r& c) {
    return (a.cross(b)).cross(c);
}

static bool do_simplex_2(GJK_Simplex* simplex, Vector3r* direction) {
    const Vector3r& a = simplex->a; // the last point added
    const Vector3r& b = simplex->b;

    Vector3r ao = -a;
    Vector3r ab = b - a;

    if (ab.dot(ao) >= 0.0) {
        simplex->a = a;
        simplex->b = b;
        simplex->num = 2;
        *direction = triple_cross(ab, ao, ab);
    } else {
        simplex->a = a;
        simplex->num = 1;
        *direction = ao;
    }

    return false;
}

static bool do_simplex_3(GJK_Simplex* simplex, Vector3r* direction) {
    const Vector3r& a = simplex->a; // the last point added
    const Vector3r& b = simplex->b;
    const Vector3r& c = simplex->c;

    Vector3r ao = -a;
    Vector3r ab = b - a;
    Vector3r ac = c - a;
    Vector3r abc = ab.cross(ac);

    if ((abc.cross(ac)).dot(ao) >= 0.0) {
        if (ac.dot(ao) >= 0.0) {
            // AC region
            simplex->a = a;
            simplex->b = c;
            simplex->num = 2;
            *direction = triple_cross(ac, ao, ac);
        } else {
            if (ab.dot(ao) >= 0.0) {
                // AB region
                simplex->a = a;
                simplex->b = b;
                simplex->num = 2;
                *direction = triple_cross(ab, ao, ab);
            } else {
                // A region
                simplex->a = a;
                simplex->num = 1;
                *direction = ao;
            }
        }
    } else {
        if ((ab.cross(abc)).dot(ao) >= 0.0) {
            if (ab.dot(ao) >= 0.0) {
                // AB region
                simplex->a = a;
                simplex->b = b;
                simplex->num = 2;
                *direction = triple_cross(ab, ao, ab);
            } else {
                // A region
                simplex->a = a;
                simplex->num = 1;
                *direction = ao;
            }
        } else {
            if (abc.dot(ao) >= 0.0) {
                // ABC region ("up")
                simplex->a = a;
                simplex->b = b;
                simplex->c = c;
                simplex->num = 3;
                *direction = abc;
            } else {
                // ABC region ("down")
                simplex->a = a;
                simplex->b = c;
                simplex->c = b;
                simplex->num = 3;
                *direction = -abc;
            }
        }
    }

    return false;
}

static bool do_simplex_4(GJK_Simplex* simplex, Vector3r* direction) {
    const Vector3r& a = simplex->a; // the last point added
    const Vector3r& b = simplex->b;
    const Vector3r& c = simplex->c;
    const Vector3r& d = simplex->d;

    Vector3r ao = -a;
    Vector3r ab = b - a;
    Vector3r ac = c - a;
    Vector3r ad = d - a;
    Vector3r abc = ab.cross(ac);
    Vector3r acd = ac.cross(ad);
    Vector3r adb = ad.cross(ab);

    unsigned char plane_information = 0x0;

    if (abc.dot(ao) >= 0.0) {
        plane_information |= 0x1;
    }
    if (acd.dot(ao) >= 0.0) {
        plane_information |= 0x2;
    }
    if (adb.dot(ao) >= 0.0) {
        plane_information |= 0x4;
    }

    switch (plane_information) {
        case 0x0: {
            // Intersection
            return true;
        } break;
        case 0x1: {
            // Triangle ABC
            if ((abc.cross(ac)).dot(ao) >= 0.0) {
                if (ac.dot(ao) >= 0.0) {
                    // AC region
                    simplex->a = a;
                    simplex->b = c;
                    simplex->num = 2;
                    *direction = triple_cross(ac, ao, ac);
                } else {
                    if (ab.dot(ao) >= 0.0) {
                        // AB region
                        simplex->a = a;
                        simplex->b = b;
                        simplex->num = 2;
                        *direction = triple_cross(ab, ao, ab);
                    } else {
                        // A region
                        simplex->a = a;
                        simplex->num = 1;
                        *direction = ao;
                    }
                }
            } else {
                if ((ab.cross(abc)).dot(ao) >= 0.0) {
                    if (ab.dot(ao) >= 0.0) {
                        // AB region
                        simplex->a = a;
                        simplex->b = b;
                        simplex->num = 2;
                        *direction = triple_cross(ab, ao, ab);
                    } else {
                        // A region
                        simplex->a = a;
                        simplex->num = 1;
                        *direction = ao;
                    }
                } else {
                    // ABC region
                    simplex->a = a;
                    simplex->b = b;
                    simplex->c = c;
                    simplex->num = 3;
                    *direction = abc;
                }
            }
        } break;
        case 0x2: {
            // Triangle ACD
            if ((acd.cross(ad)).dot(ao) >= 0.0) {
                if (ad.dot(ao) >= 0.0) {
                    // AD region
                    simplex->a = a;
                    simplex->b = d;
                    simplex->num = 2;
                    *direction = triple_cross(ad, ao, ad);
                } else {
                    if (ac.dot(ao) >= 0.0) {
                        // AC region
                        simplex->a = a;
                        simplex->b = c;
                        simplex->num = 2;
                        *direction = triple_cross(ab, ao, ab);
                    } else {
                        // A region
                        simplex->a = a;
                        simplex->num = 1;
                        *direction = ao;
                    }
                }
            } else {
                if ((ac.cross(acd)).dot(ao) >= 0.0) {
                    if (ac.dot(ao) >= 0.0) {
                        // AC region
                        simplex->a = a;
                        simplex->b = c;
                        simplex->num = 2;
                        *direction = triple_cross(ac, ao, ac);
                    } else {
                        // A region
                        simplex->a = a;
                        simplex->num = 1;
                        *direction = ao;
                    }
                } else {
                    // ACD region
                    simplex->a = a;
                    simplex->b = c;
                    simplex->c = d;
                    simplex->num = 3;
                    *direction = acd;
                }
            }
        } break;
        case 0x3: {
            // Line AC
            if (ac.dot(ao) >= 0.0) {
                simplex->a = a;
                simplex->b = c;
                simplex->num = 2;
                *direction = triple_cross(ac, ao, ac);
            } else {
                simplex->a = a;
                simplex->num = 1;
                *direction = ao;
            }

        } break;
        case 0x4: {
            // Triangle ADB
            if ((adb.cross(ab)).dot(ao) >= 0.0) {
                if (ab.dot(ao) >= 0.0) {
                    // AB region
                    simplex->a = a;
                    simplex->b = b;
                    simplex->num = 2;
                    *direction = triple_cross(ab, ao, ab);
                } else {
                    if (ad.dot(ao) >= 0.0) {
                        // AD region
                        simplex->a = a;
                        simplex->b = d;
                        simplex->num = 2;
                        *direction = triple_cross(ad, ao, ad);
                    } else {
                        // A region
                        simplex->a = a;
                        simplex->num = 1;
                        *direction = ao;
                    }
                }
            } else {
                if ((ad.cross(adb)).dot(ao) >= 0.0) {
                    if (ad.dot(ao) >= 0.0) {
                        // AD region
                        simplex->a = a;
                        simplex->b = d;
                        simplex->num = 2;
                        *direction = triple_cross(ad, ao, ad);
                    } else {
                        // A region
                        simplex->a = a;
                        simplex->num = 1;
                        *direction = ao;
                    }
                } else {
                    // ADB region
                    simplex->a = a;
                    simplex->b = d;
                    simplex->c = b;
                    simplex->num = 3;
                    *direction = adb;
                }
            }
        } break;
        case 0x5: {
            // Line AB
            if (ab.dot(ao) >= 0.0) {
                simplex->a = a;
                simplex->b = b;
                simplex->num = 2;
                *direction = triple_cross(ab, ao, ab);
            } else {
                simplex->a = a;
                simplex->num = 1;
                *direction = ao;
            }
        } break;
        case 0x6: {
            // Line AD
            if (ad.dot(ao) >= 0.0) {
                simplex->a = a;
                simplex->b = d;
                simplex->num = 2;
                *direction = triple_cross(ad, ao, ad);
            } else {
                simplex->a = a;
                simplex->num = 1;
                *direction = ao;
            }
        } break;
        case 0x7: {
            // Point A
            simplex->a = a;
            simplex->num = 1;
            *direction = ao;
        } break;
    }

    return false;
}

static bool do_simplex(GJK_Simplex* simplex, Vector3r* direction) {
    switch (simplex->num) {
        case 2: return do_simplex_2(simplex, direction);
        case 3: return do_simplex_3(simplex, direction);
        case 4: return do_simplex_4(simplex, direction);
    }

    assert(false);
    return false;
}

bool gjk_collides(Collider* collider1, Collider* collider2, GJK_Simplex* _simplex) {
    GJK_Simplex simplex;

    simplex.a = support_point_of_minkowski_difference(collider1, collider2, Vector3r(0.0, 0.0, 1.0));
    simplex.num = 1; 

    Vector3r direction = -simplex.a;

    for (unsigned int i = 0; i < 100; ++i) {
        Vector3r next_point = support_point_of_minkowski_difference(collider1, collider2, direction);
        
        if (next_point.dot(direction) < 0.0) {
            // No intersection.
            return false;
        }

        add_to_simplex(&simplex, next_point);

        if (do_simplex(&simplex, &direction)) {
            // Intersection.
            if (_simplex) {
                *_simplex = simplex;
            }
            return true;
        }
    }

    // GJK did not converge
    return false;
}