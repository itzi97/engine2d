// src/ecs/components/TransformComponent.hpp
#pragma once

#include "ecs/Component.hpp"
#include <glm/vec2.hpp>

class World;

struct TransformComponent : Component {
  glm::vec2 position{0.0f, 0.0f};
  glm::vec2 scale{1.0f, 1.0f};
  glm::vec2 size{0.0f, 0.0f};

  glm::vec2 velocity{0.0f, 0.0f};
  glm::vec2 acceleration{0.0f, 0.0f};

  float rotation{0.0f}; // degrees

  TransformComponent() = default;
  TransformComponent(glm::vec2 pos, glm::vec2 scl = {1, 1}, float rot = 0.0f)
      : position(pos), scale(scl), rotation(rot) {}

  void Update([[maybe_unused]] float dt) override {
    // velocity += acceleration * dt;
    // position += velocity * dt;
  }

  // Transform doesn't render itself; match the updated base signature.
  void Render(SDL_Renderer *, World *) override {}
};
