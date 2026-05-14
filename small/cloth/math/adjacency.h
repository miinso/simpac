#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>

namespace physics {

struct EdgeInfo {
    int v0, v1;     // shared edge vertices
    int o0, o1;     // opposite vertices (-1 if boundary)
    int f0, f1;     // face indices (-1 if boundary)
};

inline std::vector<EdgeInfo> build_adjacency(const int* tri_indices, int tri_count) {
    struct HalfEdge {
        int opposite;
        int face;
    };

    // edge key: (min, max) vertex pair
    auto key = [](int a, int b) -> uint64_t {
        int lo = a < b ? a : b;
        int hi = a < b ? b : a;
        return ((uint64_t)lo << 32) | (uint64_t)hi;
    };

    struct EdgeBuild {
        int v0, v1;
        int o0 = -1, o1 = -1;
        int f0 = -1, f1 = -1;
    };

    std::unordered_map<uint64_t, EdgeBuild> edge_map;

    for (int f = 0; f < tri_count; f++) {
        const int i0 = tri_indices[f * 3];
        const int i1 = tri_indices[f * 3 + 1];
        const int i2 = tri_indices[f * 3 + 2];

        // three edges per triangle, opposite vertex is the one not on the edge
        int edges[3][2] = {{i0, i1}, {i1, i2}, {i2, i0}};
        int opp[3] = {i2, i0, i1};

        for (int e = 0; e < 3; e++) {
            uint64_t k = key(edges[e][0], edges[e][1]);
            auto it = edge_map.find(k);
            if (it == edge_map.end()) {
                EdgeBuild eb;
                eb.v0 = edges[e][0];
                eb.v1 = edges[e][1];
                eb.o0 = opp[e];
                eb.f0 = f;
                edge_map[k] = eb;
            } else {
                it->second.o1 = opp[e];
                it->second.f1 = f;
            }
        }
    }

    std::vector<EdgeInfo> result;
    result.reserve(edge_map.size());
    for (auto& [k, eb] : edge_map) {
        result.push_back({eb.v0, eb.v1, eb.o0, eb.o1, eb.f0, eb.f1});
    }
    return result;
}

} // namespace physics
