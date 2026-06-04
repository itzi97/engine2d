#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/SpatialHash.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TransformComponent.hpp"

// Cell size: ~2× the average entity footprint in world units.
// Tune this if entity sizes change significantly (e.g. large tiles vs small bullets).
static constexpr int kCellSize = 64;

static bool Overlaps(const TransformComponent& a, const TransformComponent& b) {
    const float aw = a.size.x * a.scale.x;
    const float ah = a.size.y * a.scale.y;
    const float bw = b.size.x * b.scale.x;
    const float bh = b.size.y * b.scale.y;
    return a.position.x         < b.position.x + bw &&
           a.position.x + aw    > b.position.x      &&
           a.position.y         < b.position.y + bh &&
           a.position.y + ah    > b.position.y;
}

void CollisionSystem::Update(World& world) {
    world.ClearCollisions();

    // Broadphase: build spatial hash, collect candidate pairs
    SpatialHash hash(kCellSize);
    world.ForEach<TransformComponent>([&](EntityId id, const TransformComponent& tr) {
        hash.Insert(id,
            tr.position.x, tr.position.y,
            tr.size.x * tr.scale.x,
            tr.size.y * tr.scale.y);
    });

    // Narrow-phase: AABB test only on candidates from the same cell
    for (auto [a, b] : hash.Pairs()) {
        const auto* ta = world.GetComponent<TransformComponent>(a);
        const auto* tb = world.GetComponent<TransformComponent>(b);
        if (ta && tb && Overlaps(*ta, *tb))
            world.AddCollision({a, b});
    }
}
