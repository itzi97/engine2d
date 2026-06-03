#pragma once

#include <concepts>

// Pure data base. No Update, no Render — systems own that logic.
struct Component {
  virtual ~Component() = default;
};

// C++20 concept: T is a valid ECS component
template <typename T>
concept ComponentType = std::derived_from<T, Component>
                     && std::move_constructible<T>;
