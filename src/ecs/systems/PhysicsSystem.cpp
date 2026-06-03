#include "ecs/systems/PhysicsSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/KinematicComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

void PhysicsSystem::Update(World &world, float dt) {
  world.ForEach<KinematicComponent>([&](EntityId entity, KinematicComponent &k) {
    auto *t = world.GetComponent<TransformComponent>(entity);
    if (!t) return;
    k.velocity  += k.acceleration * dt;
    t->position += k.velocity     * dt;
  });
}
