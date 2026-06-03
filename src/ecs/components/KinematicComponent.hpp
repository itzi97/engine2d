#pragma once
#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TransformComponent.hpp"
#include <glm/vec2.hpp>

// Adds velocity (and optionally acceleration) to an entity that also has a
// TransformComponent. Only entities that need physics-driven movement should
// have this component — static/scripted objects (e.g. Snake segments) don't.
struct KinematicComponent : Component {
  glm::vec2 velocity{0.f, 0.f};
  glm::vec2 acceleration{0.f, 0.f};

  EntityId owner;

  explicit KinematicComponent(EntityId owner) : owner(owner) {}

  void Update(float dt) override;
};
