#pragma once

#include <SDL3/SDL.h>
#include <concepts>

class World;

// Base component interface. All components must be default-constructible
// and movable so PackedStorage<T> can manage them in a contiguous vector.
struct Component {
  virtual ~Component() = default;
  virtual void Update([[maybe_unused]] float dt) {}
  virtual void Render([[maybe_unused]] SDL_Renderer *renderer,
                      [[maybe_unused]] World *world) {}
};

// C++20 concept: T is a valid ECS component
template <typename T>
concept ComponentType = std::derived_from<T, Component>
                     && std::move_constructible<T>;
