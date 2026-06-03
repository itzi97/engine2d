#pragma once

class World;

// Integrates velocity and acceleration into TransformComponent::position
// for every entity that has both a TransformComponent and a KinematicComponent.
struct PhysicsSystem {
  static void Update(World &world, float dt);
};
