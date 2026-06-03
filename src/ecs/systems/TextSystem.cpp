#include "ecs/systems/TextSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/TextComponent.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "rendering/FontManager.hpp"

#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <numeric>

void TextSystem::Render(World &world, SDL_Renderer *renderer,
                        FontManager &fonts, const std::string &defaultFont) {
  auto *storage = world.GetStorage<TextComponent>();
  if (!storage || storage->entities.empty()) return;

  // Sort by layer, same as RenderSystem.
  const size_t n = storage->entities.size();
  std::vector<size_t> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
    return storage->components[a].layer < storage->components[b].layer;
  });

  for (const size_t i : order) {
    const EntityId  entity = storage->entities[i];
    TextComponent  &tc     = storage->components[i];

    if (tc.text.empty()) continue;

    // Rebuild texture if dirty.
    if (tc.dirty) {
      if (tc.texture) {
        SDL_DestroyTexture(tc.texture);
        tc.texture = nullptr;
      }

      TTF_Font *font = fonts.Get(defaultFont, tc.fontSize);
      if (!font) continue;

      SDL_Surface *surf = TTF_RenderText_Blended(
          font, tc.text.c_str(), tc.text.size(), tc.color);
      if (!surf) continue;

      tc.texture = SDL_CreateTextureFromSurface(renderer, surf);
      tc.texW    = surf->w;
      tc.texH    = surf->h;
      SDL_DestroySurface(surf);
      tc.dirty = false;
    }

    if (!tc.texture) continue;

    // Position from TransformComponent if present, else (0,0).
    float x = 0.f, y = 0.f;
    if (const auto *t = world.GetComponent<TransformComponent>(entity)) {
      x = t->position.x;
      y = t->position.y;
    }

    const SDL_FRect dst{x, y,
                        static_cast<float>(tc.texW),
                        static_cast<float>(tc.texH)};
    SDL_RenderTexture(renderer, tc.texture, nullptr, &dst);
  }
}
