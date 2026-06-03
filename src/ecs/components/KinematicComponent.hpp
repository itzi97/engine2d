#pragma once
#include "ecs/Component.hpp"
#include <glm/vec2.hpp>

// Velocity/acceleration data only.
// PhysicsSystem reads this + TransformComponent and integrates.
struct KinematicComponent : Component {
  glm::vec2 velocity{0.f, 0.f};
  glm::vec2 acceleration{0.f, 0.f};
};
