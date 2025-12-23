#pragma once

#include <Eigen/Dense>
#include <cstdint>
#include <raylib.h>
#include <vector>

using namespace Eigen;

struct aabb {
    Vector3f min;
    Vector3f max;


    aabb() : min(Vector3f::Zero()), max(Vector3f::Zero()) {}
    aabb(const Vector3f &min_val, const Vector3f &max_val) : min(min_val), max(max_val) {}

    static bool intersect(const aabb &a, const aabb &b) {
        return (a.min.array() <= b.max.array()).all() && (a.max.array() >= b.min.array()).all();
    }
    bool intersect(const aabb &other) const { return intersect(*this, other); }

    static aabb merge(const aabb &a, const aabb &b) {
        return aabb(a.min.cwiseMin(b.min), a.max.cwiseMax(b.max));
    }
    aabb merge(const aabb &other) const { return merge(*this, other); }

    static bool contains(const aabb &box, const Vector3f &point) {
        return (point.array() >= box.min.array()).all() && (point.array() <= box.max.array()).all();
    }

    static aabb infinite() {
        return aabb(Vector3f::Constant(-std::numeric_limits<float>::infinity()),
                    Vector3f::Constant(std::numeric_limits<float>::infinity()));
    }
};

struct encoding {
    int primitive_index; // object index
    uint32_t code; // encoded object position
};

// https://github.com/mmp/pbrt-v3/blob/master/src/accelerators/bvh.cpp
static void radix_sort(std::vector<encoding> &v) {
    std::vector<encoding> temp(v.size());
    constexpr int bits_per_pass = 6;
    constexpr int num_bits = 30;
    static_assert((num_bits % bits_per_pass) == 0,
                  "Radix sort bits_per_pass must evenly divide num_bits");
    constexpr int num_passes = num_bits / bits_per_pass;

    // starts from LSB to MSB
    for (int pass = 0; pass < num_passes; ++pass) {
        // perfrom one pass of radix sort
        int low_bit = pass * bits_per_pass;

        // set in and out vector pointers for radix sort pass
        // swap when pass is even number
        std::vector<encoding> &in = (pass & 1) ? temp : v;
        std::vector<encoding> &out = (pass & 1) ? v : temp;

        constexpr int num_buckets = 1 << bits_per_pass;
        int bucket_count[num_buckets] = {0};
        constexpr int bit_mask = (1 << bits_per_pass) - 1;
        for (const encoding &mp: in) {
            // histogram
            int bucket = (mp.code >> low_bit) & bit_mask;
            ++bucket_count[bucket];
        }

        // compute starting index in output array for each bucket
        int out_index[num_buckets];
        out_index[0] = 0;
        for (int i = 1; i < num_buckets; ++i) {
            out_index[i] = out_index[i - 1] + bucket_count[i - 1];
        }

        // store sorted values in output array
        for (const encoding &mp: in) {
            int bucket = (mp.code >> low_bit) & bit_mask;
            out[out_index[bucket]++] = mp;
        }
    }

    // copy final result from `temp`, if needed
    if (num_passes & 1) {
        std::swap(v, temp);
    }
}

// struct LinearBVHNode {
//     aabb bounds;
//     union {
//         int primitivesOffset; // leaf
//         int secondChildOffset; // interior
//     };
//     uint16_t nPrimitives; // 0 -> interior node
//     uint8_t axis; // interior node: xyz
//     uint8_t pad[1]; // ensure 32 byte total size
// };

struct thing {
    int id;
    float w, h, d;
    Vector3f pos;
    aabb aabb;
};

struct bvh_node {
    bvh_node *left;
    bvh_node *right;
    int thing_id;
    aabb bounds;

    bvh_node() : left(nullptr), right(nullptr), thing_id(-1), bounds(aabb::infinite()) {}
    bvh_node(int id, const aabb &box) : left(nullptr), right(nullptr), thing_id(id), bounds(box) {}
    bvh_node(bvh_node *l, bvh_node *r, const aabb &box) :
        left(l), right(r), thing_id(-1), bounds(box) {}

    bool is_leaf() const { return thing_id != -1; }
    // bool is_internal() const { return thing_id == -1; }
    ~bvh_node() {
        delete left;
        delete right;
    }
};

inline int find_split(std::vector<encoding> sorted_list, int first, int last) {
    uint32_t first_code = sorted_list[first].code;
    uint32_t last_code = sorted_list[last].code;

    if (first == last) {
        return (first + last) >> 1;
    }

    int common_prefix_length = __builtin_clz(first_code ^ last_code);

    // use binary search to find where the next bit differs
    int split = first; // initial guess
    int step = last - first;

    do {
        step = (step + 1) >> 1; // exponential decrease
        int new_split = split + step;

        if (new_split < last) {
            uint32_t split_code = sorted_list[new_split].code;
            int split_prefix_length = __builtin_clz(first_code ^ split_code);
            if (split_prefix_length > common_prefix_length) {
                split = new_split;
            }
        }
    } while (step > 1);

    return split;
};


static inline uint32_t expand(uint32_t v) {
    v &= 0x000003ff; // v = ---- ---- ---- ---- ---- --98 7654 3210
    v = (v ^ (v << 16)) & 0xff0000ff; // v = ---- --98 ---- ---- ---- ---- 7654 3210
    v = (v ^ (v << 8)) & 0x0300f00f; // v = ---- --98 ---- ---- 7654 ---- ---- 3210
    v = (v ^ (v << 4)) & 0x030c30c3; // v = ---- --98 ---- 76-- --54 ---- 32-- --10
    v = (v ^ (v << 2)) & 0x09249249; // v = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
    return v;
}

static inline uint32_t encode_position_3d(const Vector3f &pos) {
    // expects a normalized position
    uint32_t x = static_cast<uint32_t>(std::max(0.0f, std::min(1023.0f, pos.x() * 1023.0f)));
    uint32_t y = static_cast<uint32_t>(std::max(0.0f, std::min(1023.0f, pos.y() * 1023.0f)));
    uint32_t z = static_cast<uint32_t>(std::max(0.0f, std::min(1023.0f, pos.z() * 1023.0f)));

    return expand(x) | (expand(y) << 1) | (expand(z) << 2);
}

inline bvh_node *create_leaf_node(int thing_id, const thing &thing) {
    return new bvh_node(thing_id, thing.aabb);
}

bvh_node *create_subtree(const std::vector<encoding> &sorted_list, int begin, int end,
                         const std::vector<thing> &things) {
    if (begin == end) {
        int thing_index = sorted_list[begin].primitive_index;

        // create a leaf node
        return create_leaf_node(things[thing_index].id, things[thing_index]);
    } else {
        int split = find_split(sorted_list, begin, end);

        auto *node = new bvh_node();
        node->left = create_subtree(sorted_list, begin, split, things);
        node->right = create_subtree(sorted_list, split + 1, end, things);
        node->bounds = aabb::merge(node->left->bounds, node->right->bounds);

        return node;
    }
}

bvh_node *create_tree(const std::vector<thing> &things) {
    std::vector<encoding> list(things.size());

    for (int i = 0; i < things.size(); ++i) {
        const auto &center = things[i].pos;
        const uint32_t code = encode_position_3d(center);

        list.emplace_back(i, code);
    }

    std::sort(list.begin(), list.end(),
              [](const auto &a, const auto &b) { return a.second < b.second; });

    // radix_sort(list);

    return create_subtree(list, 0, list.size() - 1, things);
}

struct collision {
    int a;
    int b;
};

static inline void find_collisions(int thing_id, thing &t, bvh_node *node,
                                   std::vector<thing> &things, std::vector<collision> &collisions,
                                   int &check_count) {
    check_count++;

    if (!aabb::intersect(t.aabb, node->bounds)) {
        return;
    }

    // if this is a leaf node
    if (node->is_leaf()) {
        // don't check collisions with self
        if (node->thing_id != thing_id) {
            if (aabb::intersect(t.aabb, things[node->thing_id].aabb)) {
                collisions.emplace_back(thing_id, node->thing_id);
            }
        }
    }

    find_collisions(thing_id, t, node->left, things, collisions, check_count);
    find_collisions(thing_id, t, node->right, things, collisions, check_count);
}

static std::vector<collision> check_collision(std::vector<thing> things, bvh_node *bvh_root) {
    std::vector<collision> collisions;
    int check_count = 0;

    for (int i = 0; i < things.size(); ++i) {
        find_collisions(i, things[i], bvh_root, things, collisions, check_count);
    }

    return collisions;
}

// __device__ void traverseIterative( CollisionList& list,
//                                    BVH& bvh, 
//                                    AABB& queryAABB, 
//                                    int queryObjectIdx)
// {
//     // Allocate traversal stack from thread-local memory,
//     // and push NULL to indicate that there are no postponed nodes.
//     NodePtr stack[64];
//     NodePtr* stackPtr = stack;
//     *stackPtr++ = NULL; // push `NULL` to the stack, then increment the pointer

//     // Traverse nodes starting from the root.
//     NodePtr node = bvh.getRoot();
//     do
//     {
//         // Check each child node for overlap.
//         NodePtr childL = bvh.getLeftChild(node);
//         NodePtr childR = bvh.getRightChild(node);
//         bool overlapL = ( checkOverlap(queryAABB, 
//                                        bvh.getAABB(childL)) );
//         bool overlapR = ( checkOverlap(queryAABB, 
//                                        bvh.getAABB(childR)) );

//         // Query overlaps a leaf node => report collision.
//         if (overlapL && bvh.isLeaf(childL))
//             list.add(queryObjectIdx, bvh.getObjectIdx(childL));

//         if (overlapR && bvh.isLeaf(childR))
//             list.add(queryObjectIdx, bvh.getObjectIdx(childR));

//         // Query overlaps an internal node => traverse.
//         bool traverseL = (overlapL && !bvh.isLeaf(childL));
//         bool traverseR = (overlapR && !bvh.isLeaf(childR));

//         if (!traverseL && !traverseR) // we're in a leaf node now
//             node = *--stackPtr; // pop: decrement the pointer, then get the value
//         else
//         {
//             node = (traverseL) ? childL : childR;
//             if (traverseL && traverseR)
//                 *stackPtr++ = childR; // push
//         }
//     }
//     while (node != NULL);
// }