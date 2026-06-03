#include "ecs/components/KinematicComponent.hpp"

void KinematicComponent::Update(float dt) {
  velocity  += acceleration * dt;
  if (auto *t = owner_world ? owner_world->GetComponent<TransformComponent>(owner) : nullptr)
    t->position += velocity * dt;
}
