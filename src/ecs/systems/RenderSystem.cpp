#include "ecs/systems/RenderSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

void RenderSystem::Render(World &world, SDL_Renderer *renderer) {
  auto *sprStorage = world.GetStorage<SpriteComponent>();
  if (!sprStorage) return;

  for (size_t i = 0; i < sprStorage->entities.size(); ++i) {
    const EntityId      entity = sprStorage->entities[i];
    const SpriteComponent &s   = sprStorage->components[i];

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
