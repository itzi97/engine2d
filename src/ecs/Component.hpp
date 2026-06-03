#pragma once

#include <SDL3/SDL.h>

class World;

class Component {
public:
  virtual ~Component() = default;

  // World is passed in so components can look up sibling components.
  virtual void Update(float /*deltaTime*/) {}
  virtual void Render(SDL_Renderer * /*renderer*/, World * /*world*/) {}
};
