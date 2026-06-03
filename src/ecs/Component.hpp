#pragma once

#include <SDL3/SDL.h>

class World; // forward declared — no circular include

struct Component {
  virtual ~Component() = default;

  // Called every frame by PackedStorage<T>::Update
  virtual void Update([[maybe_unused]] float dt) {}

  // Called every frame by PackedStorage<T>::Render
  virtual void Render([[maybe_unused]] SDL_Renderer *renderer,
                      [[maybe_unused]] World *world) {}
};
