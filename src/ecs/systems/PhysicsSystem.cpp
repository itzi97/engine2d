#include "ecs/systems/PhysicsSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/KinematicComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

void PhysicsSystem::Update(World &world, float dt) {
  auto *kinStorage = world.GetStorage<KinematicComponent>();
  if (!kinStorage) return;

  for (size_t i = 0; i < kinStorage->entities.size(); ++i) {
    const EntityId entity = kinStorage->entities[i];
    KinematicComponent &k = kinStorage->components[i];

    auto *t = world.GetComponent<TransformComponent>(entity);
    if (!t) continue;

    k.velocity  += k.acceleration * dt;
    t->position += k.velocity     * dt;
  }
}
