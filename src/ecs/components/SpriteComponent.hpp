#pragma once
#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"
#include <SDL3/SDL.h>

class World;

struct SpriteComponent : Component {
  EntityId  owner;
  SDL_Color color{255, 255, 255, 255};

  SpriteComponent(EntityId owner, SDL_Color color = {255, 255, 255, 255})
      : owner(owner), color(color) {}

  void Render(SDL_Renderer *renderer, World *world) override;
};
