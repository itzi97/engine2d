#pragma once
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "ecs/Entity.hpp"

// Uniform-grid spatial hash for broadphase collision culling.
//
// Usage (each frame):
//   SpatialHash hash(64);          // cell size ~2× average entity
//   hash.Insert(id, x, y, w, h);   // one call per entity
//   for (auto [a, b] : hash.Pairs()) { /* narrow-phase test */ }
//
// Entities that share no cell are never returned as candidates,
// reducing the collision workload from O(n²) to roughly O(n)
// for typical scenes where entities are spread out.

class SpatialHash {
public:
    explicit SpatialHash(float cellSize) : m_cell(cellSize) {}

    void Clear() { m_cells.clear(); }

    void Insert(EntityId id, float x, float y, float w, float h) {
        int x0 = (int)std::floor(x / m_cell);
        int y0 = (int)std::floor(y / m_cell);
        int x1 = (int)std::floor((x + w) / m_cell);
        int y1 = (int)std::floor((y + h) / m_cell);
        for (int cx = x0; cx <= x1; ++cx)
            for (int cy = y0; cy <= y1; ++cy)
                m_cells[CellKey(cx, cy)].push_back(id);
    }

    // Returns deduplicated candidate pairs that share at least one cell.
    // Each pair is returned exactly once regardless of how many cells
    // the two entities share.
    std::vector<std::pair<EntityId, EntityId>> Pairs() const {
        std::unordered_set<uint64_t> seen;
        std::vector<std::pair<EntityId, EntityId>> out;
        for (auto& [k, bucket] : m_cells) {
            for (size_t i = 0; i < bucket.size(); ++i)
                for (size_t j = i + 1; j < bucket.size(); ++j) {
                    EntityId a = std::min(bucket[i], bucket[j]);
                    EntityId b = std::max(bucket[i], bucket[j]);
                    uint64_t pairKey = ((uint64_t)a << 32) | (uint32_t)b;
                    if (seen.insert(pairKey).second)
                        out.push_back({a, b});
                }
        }
        return out;
    }

private:
    float m_cell;
    std::unordered_map<uint64_t, std::vector<EntityId>> m_cells;

    // Pack two signed cell coords into one 64-bit key.
    // Cast through uint32_t so negative coords wrap correctly.
    static uint64_t CellKey(int cx, int cy) {
        return ((uint64_t)(uint32_t)cx << 32) | (uint32_t)cy;
    }
};
