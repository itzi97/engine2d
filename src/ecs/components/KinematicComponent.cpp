#include "ecs/components/KinematicComponent.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TransformComponent.hpp"

void KinematicComponent::Update(float dt) {
  if (!world) return;
  velocity += acceleration * dt;
  if (auto *t = world->GetComponent<TransformComponent>(owner))
    t->position += velocity * dt;
}
