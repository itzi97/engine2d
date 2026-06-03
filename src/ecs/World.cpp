#include "ecs/World.hpp"
#include "ecs/components/KinematicComponent.hpp"

KinematicComponent &World::AddKinematic(EntityId entity) {
  return GetOrCreateStorage<KinematicComponent>()
      .Add(entity, KinematicComponent{entity, this});
}

void World::Update(float deltaTime) {
  for (auto &[type, storage] : m_storages)
    storage->Update(deltaTime);
}

void World::Render(SDL_Renderer *renderer) {
  for (auto &[type, storage] : m_storages)
    storage->Render(renderer, this);
}
