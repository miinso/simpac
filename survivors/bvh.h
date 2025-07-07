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
    aabb bbox;

    bvh_node() : left(nullptr), right(nullptr), thing_id(-1), bbox(aabb::infinite()) {}
    bvh_node(int id, const aabb &box) : left(nullptr), right(nullptr), thing_id(id), bbox(box) {}
    bvh_node(bvh_node *l, bvh_node *r, const aabb &box) :
        left(l), right(r), thing_id(-1), bbox(box) {}

    bool is_leaf() const { return thing_id != -1; }
    bool is_internal() const { return thing_id == -1; }
    ~bvh_node() {
        delete left;
        delete right;
    }
};

inline int find_split(std::vector<std::pair<int, uint32_t>> list, int first, int last) {
    uint32_t first_code = list[first].second;
    uint32_t last_code = list[last].second;

    if (first == last) {
        return (first + last) >> 1;
    }

    int common_prefix_length = __builtin_clz(first_code ^ last_code);

    int split = first; // initial guess
    int step = last - first;

    do {
        step = (step + 1) >> 1; // exponential decrease
        int new_split = split + step;

        if (new_split < last) {
            uint32_t split_code = list[new_split].second;
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

inline bvh_node create_leaf_node(int thing_id, const thing &thing) {
    return bvh_node(thing_id, thing.aabb);
}

bvh_node *create_subtree(const std::vector<std::pair<int, uint32_t>> &list, int begin, int end,
                         const std::vector<thing> &things) {
    if (begin == end) {
        int thing_index = list[begin].first;
        return new bvh_node(things[thing_index].id, things[thing_index].aabb);
    } else {
        int split = find_split(list, begin, end);
        auto *node = new bvh_node();
        node->left = create_subtree(list, begin, split, things);
        node->right = create_subtree(list, split + 1, end, things);
        node->bbox = aabb::merge(node->left->bbox, node->right->bbox);
        return node;
    }
}

bvh_node *create_tree(const std::vector<thing> &things) {
    std::vector<std::pair<int, uint32_t>> list;
    list.reserve(things.size());

    for (int i = 0; i < things.size(); ++i) {
        const auto &center = things[i].pos;
        const uint32_t code = encode_position_3d(center);

        list.emplace_back(i, code);
    }

    std::sort(list.begin(), list.end(),
              [](const auto &a, const auto &b) { return a.second < b.second; });

    return create_subtree(list, 0, list.size() - 1, things);
}
