#pragma once
#include "ecs/Component.hpp"
#include <SDL3/SDL.h>

// Color/texture data + draw layer.
// Lower layer values draw first (behind). Same-layer order is insertion order.
struct SpriteComponent : Component {
  SDL_Color color{255, 255, 255, 255};
  int       layer{0};

  explicit SpriteComponent(SDL_Color col = {255, 255, 255, 255}, int l = 0)
      : color(col), layer(l) {}
};
