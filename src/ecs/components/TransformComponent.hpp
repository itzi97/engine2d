#pragma once
#include "ecs/Component.hpp"
#include <glm/vec2.hpp>

// Stores position, size, scale and rotation.
// Does NOT integrate velocity — use KinematicComponent for moving objects.
struct TransformComponent : Component {
  glm::vec2 position{0.f, 0.f};
  glm::vec2 size{32.f, 32.f};
  glm::vec2 scale{1.f, 1.f};
  float     rotation{0.f};
};
