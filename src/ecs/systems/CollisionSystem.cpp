#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TransformComponent.hpp"

static bool Overlaps(const TransformComponent &a, const TransformComponent &b) {
  const float aw = a.size.x * a.scale.x;
  const float ah = a.size.y * a.scale.y;
  const float bw = b.size.x * b.scale.x;
  const float bh = b.size.y * b.scale.y;
  return a.position.x < b.position.x + bw &&
         a.position.x + aw > b.position.x &&
         a.position.y < b.position.y + bh &&
         a.position.y + ah > b.position.y;
}

void CollisionSystem::Update(World &world) {
  world.ClearCollisions();
  const auto view = world.View<TransformComponent>();
  const size_t n  = view.size();
  for (size_t i = 0; i < n; ++i)
    for (size_t j = i + 1; j < n; ++j)
      if (Overlaps(view.components[i], view.components[j]))
        world.AddCollision({view.entities[i], view.entities[j]});
}
