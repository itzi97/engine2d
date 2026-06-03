#include "ecs/systems/RenderSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

void RenderSystem::Render(World &world, SDL_Renderer *renderer) {
  // ForEachSorted visits sprites in ascending layer order.
  world.ForEachSorted<SpriteComponent>([&](EntityId entity, const SpriteComponent &s) {
    const auto *t = world.GetComponent<TransformComponent>(entity);
    if (!t) return;

    const SDL_FRect dst{
        t->position.x, t->position.y,
        t->size.x * t->scale.x,
        t->size.y * t->scale.y,
    };

    if (s.texture) {
      // srcRect.w == 0 means "use the full texture" (no atlas slice).
      const SDL_FRect *src = (s.srcRect.w > 0.f) ? &s.srcRect : nullptr;
      SDL_RenderTextureRotated(renderer, s.texture, src, &dst,
                               static_cast<double>(t->rotation),
                               nullptr,   // center == nullptr → rotate around dst centre
                               s.flip);
    } else {
      SDL_SetRenderDrawColor(renderer, s.color.r, s.color.g, s.color.b, s.color.a);
      SDL_RenderFillRect(renderer, &dst);
    }
  });
}
