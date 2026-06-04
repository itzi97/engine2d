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
//
// Tint / alpha  : tint is multiplied into the texture during rendering via
//                 SDL_SetTextureColorMod + SDL_SetTextureAlphaMod.
//                 {255,255,255,255} == no tint (identity).
//                 Works in both texture mode and is ignored in color-rect mode.
//
// Visibility    : when visible == false the sprite is skipped entirely by
//                 RenderSystem. Use world.set_visible(id, false) from Lua.
struct SpriteComponent : Component {
  // --- color-rect (always valid as fallback) ---
  SDL_Color color{255, 255, 255, 255};
  int       layer{0};

  // --- texture (optional) ---
  SDL_Texture *texture{nullptr};    // raw observing ptr; owned by TextureManager
  SDL_FRect    srcRect{0, 0, 0, 0}; // source rect on atlas; w==0 → full texture
  SDL_FlipMode flip{SDL_FLIP_NONE};

  // --- tint (optional, texture mode only) ---
  // Multiplied into the texture colour channel-by-channel before blending.
  // Default {255,255,255,255} is identity (no tint, full opacity).
  SDL_Color tint{255, 255, 255, 255};

  // --- visibility ---
  bool visible{true};  // false = skipped by RenderSystem entirely

  explicit SpriteComponent(SDL_Color col = {255, 255, 255, 255}, int l = 0)
      : color(col), layer(l) {}
};
