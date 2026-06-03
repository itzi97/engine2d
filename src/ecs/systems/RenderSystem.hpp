#pragma once

#include <SDL3/SDL.h>

class World;

// Draws every entity that has both a TransformComponent and a SpriteComponent.
struct RenderSystem {
  static void Render(World &world, SDL_Renderer *renderer);
};
