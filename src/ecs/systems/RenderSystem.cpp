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
      // Apply tint + alpha mod before draw, reset to identity after so
      // subsequent sprites are not affected.
      SDL_SetTextureColorMod(s.texture, s.tint.r, s.tint.g, s.tint.b);
      SDL_SetTextureAlphaMod(s.texture, s.tint.a);

      const SDL_FRect *src = (s.srcRect.w > 0.f) ? &s.srcRect : nullptr;
      SDL_RenderTextureRotated(renderer, s.texture, src, &dst,
                               static_cast<double>(t->rotation),
                               nullptr,
                               s.flip);

      // Reset to identity so other sprites sharing the same SDL_Texture
      // (e.g. two atlas sprites on the same sheet) are not tinted.
      SDL_SetTextureColorMod(s.texture, 255, 255, 255);
      SDL_SetTextureAlphaMod(s.texture, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, s.color.r, s.color.g, s.color.b, s.color.a);
      SDL_RenderFillRect(renderer, &dst);
    }
  });
}
