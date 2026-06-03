#pragma once
#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"
#include <glm/vec2.hpp>

class World;

// Adds velocity and acceleration to an entity that also has a
// TransformComponent. Only physics-driven entities need this —
// scripted/static objects (e.g. Snake segments) should not have it.
struct KinematicComponent : Component {
  glm::vec2 velocity{0.f, 0.f};
  glm::vec2 acceleration{0.f, 0.f};
  EntityId  owner;

  explicit KinematicComponent(EntityId ownerId) : owner(ownerId) {}

  // Injects world ptr at add-time so Update can reach TransformComponent.
  // Called by the Lua binding after AddComponent.
  World *world{nullptr};

  void Update(float dt) override;
};
