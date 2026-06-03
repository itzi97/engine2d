#pragma once
#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"
#include <glm/vec2.hpp>

class World;

// Adds velocity and acceleration to an entity that also has a
// TransformComponent. Only physics-driven entities need this.
//
// Do NOT construct directly -- use World::AddKinematic(entity) which
// injects the World* atomically and prevents the silent-nullptr bug.
struct KinematicComponent : Component {
  glm::vec2 velocity{0.f, 0.f};
  glm::vec2 acceleration{0.f, 0.f};

  void Update(float dt) override;

private:
  friend class World; // only World::AddKinematic may set these
  EntityId owner;
  World   *world{nullptr};

  explicit KinematicComponent(EntityId ownerId, World *w)
      : owner(ownerId), world(w) {}
};
