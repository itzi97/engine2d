#include "ecs/World.hpp"
#include "ecs/Component.hpp"

#include <SDL3/SDL.h>

void World::Update(float deltaTime) {
  for (auto &[type, storage] : m_storages) {
    for (auto &[id, component] : storage) {
      component->Update(deltaTime);
    }
  }
}

void World::Render(SDL_Renderer *renderer) {
  for (auto &[type, storage] : m_storages) {
    for (auto &[id, component] : storage) {
      component->Render(renderer, this);
    }
  }
}
