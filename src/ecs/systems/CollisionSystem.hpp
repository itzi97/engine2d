#pragma once

// Collision struct and result storage live in World.
// CollisionSystem::Update is a pure function that writes into World.

class World;

struct CollisionSystem {
  static void Update(World &world);
};
