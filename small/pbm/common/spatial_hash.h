#pragma once

#include "Eigen/Dense"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

struct SpatialHash {
    float spacing = 1.0f;
    int tableSize = 1;
    std::vector<int> cellStart;
    std::vector<int> cellEntries;
    std::vector<int> queryIds;
    int querySize = 0;
    bool initialized = false;

    SpatialHash() = default;

    inline void init(float spacing_, int maxNumObjects, int tableSize_ = 0) {
        spacing = spacing_;
        tableSize = tableSize_ > 0 ? tableSize_ : 2 * maxNumObjects;
        querySize = 0;
        cellStart.resize(tableSize + 1, 0);
        cellEntries.resize(maxNumObjects, 0);
        queryIds.resize(maxNumObjects, 0);
        initialized = true;
    }

    inline int hashCoords(int xi, int yi, int zi) const {
        // Well-known spatial hashing primes from Teschner et al.
        uint32_t h = static_cast<uint32_t>(xi) * 73856093u
                   ^ static_cast<uint32_t>(yi) * 19349663u
                   ^ static_cast<uint32_t>(zi) * 83492791u;

        // MurmurHash3 finalization mix for better avalanche
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;

        return h % tableSize;
    }

    inline int intCoord(float coord) const {
        return static_cast<int>(std::floor(coord / spacing));
    }

    inline int hashPos(const Eigen::Vector3f& pos) const {
        return hashCoords(intCoord(pos.x()), intCoord(pos.y()), intCoord(pos.z()));
    }

    inline void create(const std::vector<Eigen::Vector3f>& positions) {
        int numObjects = static_cast<int>(positions.size());

        // Resize if needed
        if (numObjects > static_cast<int>(cellEntries.size())) {
            cellEntries.resize(numObjects);
            queryIds.resize(numObjects);
            tableSize = 2 * numObjects;
            cellStart.resize(tableSize + 1);
        }

        // Reset counts
        std::fill(cellStart.begin(), cellStart.end(), 0);

        // Step 1: Count objects per cell
        for (int i = 0; i < numObjects; ++i) {
            int h = hashPos(positions[i]);
            ++cellStart[h];
        }

        // Step 2: Compute prefix sums (cumulative count)
        int start = 0;
        for (int i = 0; i < tableSize; ++i) {
            int count = cellStart[i];
            cellStart[i] = start;
            start += count;
        }
        cellStart[tableSize] = start; // guard

        // Step 3: Fill in object IDs
        std::vector<int> cellCurrent(cellStart.begin(), cellStart.begin() + tableSize);
        for (int i = 0; i < numObjects; ++i) {
            int h = hashPos(positions[i]);
            cellEntries[cellCurrent[h]] = i;
            ++cellCurrent[h];
        }
    }

    inline void query(const Eigen::Vector3f& pos, float maxDist) {
        int x0 = intCoord(pos.x() - maxDist);
        int y0 = intCoord(pos.y() - maxDist);
        int z0 = intCoord(pos.z() - maxDist);

        int x1 = intCoord(pos.x() + maxDist);
        int y1 = intCoord(pos.y() + maxDist);
        int z1 = intCoord(pos.z() + maxDist);

        querySize = 0;
        int maxQuery = static_cast<int>(queryIds.size());

        for (int xi = x0; xi <= x1; ++xi) {
            for (int yi = y0; yi <= y1; ++yi) {
                for (int zi = z0; zi <= z1; ++zi) {
                    int h = hashCoords(xi, yi, zi);
                    int start = cellStart[h];
                    int end = cellStart[h + 1];

                    for (int i = start; i < end && querySize < maxQuery; ++i) {
                        queryIds[querySize++] = cellEntries[i];
                    }
                }
            }
        }
    }
};

// ============================================================================
// TriangleSpatialHash - For triangle mesh collision detection
// ============================================================================

struct TriangleInfo {
    size_t meshId;
    size_t triangleIndex;
};

struct TriangleSpatialHash {
    float cellSize = 0.5f;
    std::unordered_map<size_t, std::vector<TriangleInfo>> grid;

    TriangleSpatialHash() = default;
    explicit TriangleSpatialHash(float cellSize_) : cellSize(cellSize_) {}

    inline int intCoord(float coord) const {
        return static_cast<int>(std::floor(coord / cellSize));
    }

    // Same hash primes as SpatialHash for consistency
    inline size_t hashCoords(int xi, int yi, int zi) const {
        uint32_t h = static_cast<uint32_t>(xi) * 73856093u
                   ^ static_cast<uint32_t>(yi) * 19349663u
                   ^ static_cast<uint32_t>(zi) * 83492791u;

        // MurmurHash3 finalization mix
        h ^= h >> 16;
        h *= 0x85ebca6bu;
        h ^= h >> 13;
        h *= 0xc2b2ae35u;
        h ^= h >> 16;

        return h;
    }

    inline size_t hashPos(const Eigen::Vector3f& pos) const {
        return hashCoords(intCoord(pos.x()), intCoord(pos.y()), intCoord(pos.z()));
    }

    // Insert triangle spanning its AABB cells
    inline void insert(const Eigen::Vector3f& v0, const Eigen::Vector3f& v1,
                       const Eigen::Vector3f& v2, size_t meshId, size_t triangleIndex) {
        Eigen::Vector3f min = v0.cwiseMin(v1).cwiseMin(v2);
        Eigen::Vector3f max = v0.cwiseMax(v1).cwiseMax(v2);

        int x0 = intCoord(min.x()), x1 = intCoord(max.x());
        int y0 = intCoord(min.y()), y1 = intCoord(max.y());
        int z0 = intCoord(min.z()), z1 = intCoord(max.z());

        for (int x = x0; x <= x1; ++x) {
            for (int y = y0; y <= y1; ++y) {
                for (int z = z0; z <= z1; ++z) {
                    size_t key = hashCoords(x, y, z);
                    grid[key].push_back({meshId, triangleIndex});
                }
            }
        }
    }

    // Query triangles at a position
    inline std::vector<TriangleInfo> query(const Eigen::Vector3f& pos) const {
        auto it = grid.find(hashPos(pos));
        return (it != grid.end()) ? it->second : std::vector<TriangleInfo>();
    }

    // Query triangles along a path (for swept collision)
    inline std::vector<TriangleInfo> queryPath(const Eigen::Vector3f& start,
                                               const Eigen::Vector3f& end) const {
        Eigen::Vector3f min = start.cwiseMin(end);
        Eigen::Vector3f max = start.cwiseMax(end);

        std::vector<TriangleInfo> result;
        for (float x = min.x(); x <= max.x(); x += cellSize) {
            for (float y = min.y(); y <= max.y(); y += cellSize) {
                for (float z = min.z(); z <= max.z(); z += cellSize) {
                    auto triangles = query(Eigen::Vector3f(x, y, z));
                    result.insert(result.end(), triangles.begin(), triangles.end());
                }
            }
        }
        return result;
    }

    inline void clear() { grid.clear(); }
};
