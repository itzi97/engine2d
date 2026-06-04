#include "ecs/systems/TextSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TextComponent.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "rendering/FontManager.hpp"

#include <SDL3_ttf/SDL_ttf.h>

void TextSystem::Render(World &world, SDL_Renderer *renderer,
                        FontManager &fonts, const std::string &defaultFont) {
  // ForEachSorted visits text components in ascending layer order.
  world.ForEachSorted<TextComponent>([&](EntityId entity, TextComponent &tc) {
    if (tc.text.empty()) return;

    if (tc.dirty) {
      if (tc.texture) {
        SDL_DestroyTexture(tc.texture);
        tc.texture = nullptr;
      }
      TTF_Font *font = fonts.Get(defaultFont, tc.fontSize);
      if (!font) return;

      SDL_Surface *surf = TTF_RenderText_Blended(
          font, tc.text.c_str(), tc.text.size(), tc.color);
      if (!surf) return;

      tc.texture = SDL_CreateTextureFromSurface(renderer, surf);
      tc.texW    = surf->w;
      tc.texH    = surf->h;
      SDL_DestroySurface(surf);
      tc.dirty = false;
    }

    if (!tc.texture) return;

    float x = 0.f, y = 0.f;
    if (const auto *t = world.GetComponent<TransformComponent>(entity)) {
      x = t->position.x;
      y = t->position.y;
    }

    // Apply anchor offset: anchor (0,0) = top-left, (0.5,0.5) = centre, etc.
    x -= tc.anchorX * static_cast<float>(tc.texW);
    y -= tc.anchorY * static_cast<float>(tc.texH);

    const SDL_FRect dst{x, y,
                        static_cast<float>(tc.texW),
                        static_cast<float>(tc.texH)};
    SDL_RenderTexture(renderer, tc.texture, nullptr, &dst);
  });
}
