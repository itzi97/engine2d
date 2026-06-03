#include "ecs/systems/RenderSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

#include <algorithm>
#include <numeric>

void RenderSystem::Render(World &world, SDL_Renderer *renderer) {
  auto *storage = world.GetStorage<SpriteComponent>();
  if (!storage || storage->entities.empty()) return;

  // Build a sorted index list by layer. Sorting indices avoids moving
  // component data and keeps PackedStorage layout stable.
  const size_t n = storage->entities.size();
  std::vector<size_t> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return storage->components[a].layer < storage->components[b].layer;
  });

  for (const size_t i : order) {
    const EntityId        entity = storage->entities[i];
    const SpriteComponent &s     = storage->components[i];

    const auto *t = world.GetComponent<TransformComponent>(entity);
    if (!t) continue;

    const SDL_FRect rect{
        t->position.x,
        t->position.y,
        t->size.x * t->scale.x,
        t->size.y * t->scale.y,
    };

    SDL_SetRenderDrawColor(renderer, s.color.r, s.color.g, s.color.b, s.color.a);
    SDL_RenderFillRect(renderer, &rect);
  }
}
