#include "ecs/World.hpp"

void World::Update(float deltaTime) {
  for (auto &[type, storage] : m_storages)
    storage->Update(deltaTime);
}

void World::Render(SDL_Renderer *renderer) {
  for (auto &[type, storage] : m_storages)
    storage->Render(renderer, this);
}
