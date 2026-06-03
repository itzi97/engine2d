#pragma once

#include "ecs/Entity.hpp"
#include <string>
#include <vector>

class World;

struct Collision {
  EntityId a;
  EntityId b;
};

// Broad-phase AABB collision detection.
// Purely stateless: results are written into World::m_collisions via Update().
// Query helpers live on World; scripts call world.get_collisions_for() etc.
struct CollisionSystem {
  static void Update(World &world);
};
