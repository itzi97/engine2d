#pragma once
#include "ecs/Component.hpp"
#include <SDL3/SDL.h>

// Color/texture data + draw layer.
// Lower layer values draw first (behind). Same-layer order is insertion order.
//
// Texture mode  : set texture != nullptr.
//                 srcRect.w == 0  →  render the full texture (no atlas slice).
//                 srcRect.w  > 0  →  render the given sub-rect (atlas sprite).
// Color-rect mode: texture == nullptr, color used with SDL_RenderFillRect.
//                  Existing snake / breakout scripts keep working unchanged.
struct SpriteComponent : Component {
  // --- color-rect (always valid as fallback) ---
  SDL_Color color{255, 255, 255, 255};
  int       layer{0};

  // --- texture (optional) ---
  SDL_Texture *texture{nullptr};    // raw observing ptr; owned by TextureManager
  SDL_FRect    srcRect{0, 0, 0, 0}; // source rect on atlas; w==0 → full texture
  SDL_FlipMode flip{SDL_FLIP_NONE};

  explicit SpriteComponent(SDL_Color col = {255, 255, 255, 255}, int l = 0)
      : color(col), layer(l) {}
};
