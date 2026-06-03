#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

std::vector<Collision> CollisionSystem::s_collisions;

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
  s_collisions.clear();

  auto *storage = world.GetStorage<TransformComponent>();
  if (!storage) return;

  const size_t n = storage->entities.size();
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) {
      if (Overlaps(storage->components[i], storage->components[j]))
        s_collisions.push_back({storage->entities[i], storage->entities[j]});
    }
  }
}

const std::vector<Collision> &CollisionSystem::GetCollisions() {
  return s_collisions;
}

std::vector<Collision> CollisionSystem::GetCollisionsFor(EntityId entity) {
  std::vector<Collision> result;
  for (const auto &c : s_collisions)
    if (c.a == entity || c.b == entity)
      result.push_back(c);
  return result;
}

std::vector<Collision> CollisionSystem::GetCollisionsTagged(World        &world,
                                                             const std::string &tag) {
  std::vector<Collision> result;
  for (const auto &c : s_collisions) {
    auto *ta = world.GetComponent<TagComponent>(c.a);
    auto *tb = world.GetComponent<TagComponent>(c.b);
    if ((ta && ta->tag == tag) || (tb && tb->tag == tag))
      result.push_back(c);
  }
  return result;
}
