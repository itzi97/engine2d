#pragma once
#include "ecs/Component.hpp"
#include <SDL3/SDL.h>

// Color/texture data only. Entity identity comes from PackedStorage::entities,
// not from a stored ID inside the component.
struct SpriteComponent : Component {
  SDL_Color color{255, 255, 255, 255};

  explicit SpriteComponent(SDL_Color col = {255, 255, 255, 255})
      : color(col) {}
};
