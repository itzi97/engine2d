// src/etc/components/TransformerComponent.hpp
#pragma once

#include "ecs/Component.hpp"
#include <glm/vec2.hpp>

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

  void Update(float dt) override {
    // TODO: Uncomment when snake is handled better
    // velocity += acceleration * dt;
    // position += velocity * dt;
  }
  void Render(SDL_Renderer *) override {} // transform doesn't render itself
};
