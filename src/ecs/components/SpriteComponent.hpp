#pragma once
#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"
#include <SDL3/SDL.h>

// Color/texture data only.
// RenderSystem reads this + TransformComponent and draws.
struct SpriteComponent : Component {
  EntityId  owner;
  SDL_Color color{255, 255, 255, 255};

  SpriteComponent(EntityId ownerId, SDL_Color col = {255, 255, 255, 255})
      : owner(ownerId), color(col) {}
};
