#include "ecs/components/SpriteComponent.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TransformComponent.hpp"

void SpriteComponent::Render(SDL_Renderer *renderer, World *world) {
  if (!renderer || !world)
    return;
  auto *t = world->GetComponent<TransformComponent>(owner);
  if (!t)
    return;

  SDL_FRect rect{
      t->position.x,
      t->position.y,
      t->size.x * t->scale.x,
      t->size.y * t->scale.y,
  };

  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_RenderFillRect(renderer, &rect);
}
