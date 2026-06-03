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
// Reports all overlapping (TransformComponent, TransformComponent) pairs.
// Does NOT resolve — scripts decide what to do with each collision.
struct CollisionSystem {
  // Run detection and cache results for this frame.
  static void Update(World &world);

  // All pairs that overlapped this frame.
  static const std::vector<Collision> &GetCollisions();

  // All collisions involving a specific entity.
  static std::vector<Collision> GetCollisionsFor(EntityId entity);

  // All collisions where either entity has a given tag.
  static std::vector<Collision> GetCollisionsTagged(World &world,
                                                     const std::string &tag);

private:
  static std::vector<Collision> s_collisions;
};
