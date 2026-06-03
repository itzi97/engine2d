#pragma once
#include "ecs/Component.hpp"
#include <glm/vec2.hpp>

struct TransformComponent : Component {
  glm::vec2 position{0.f, 0.f};
  glm::vec2 size{32.f, 32.f};
  glm::vec2 velocity{0.f, 0.f};
  glm::vec2 scale{1.f, 1.f};
  float     rotation{0.f};

  // Integrate velocity each frame
  void Update(float dt) override {
    position += velocity * dt;
  }
};
