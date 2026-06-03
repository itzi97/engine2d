#include "ecs/World.hpp"
#include "ecs/components/KinematicComponent.hpp"

KinematicComponent &World::AddKinematic(EntityId entity) {
  return GetOrCreateStorage<KinematicComponent>()
      .Add(entity, KinematicComponent{entity, this});
}

void World::Update(float dt) {
  // 1. Update in explicit priority order so dependencies are respected
  //    (e.g. KinematicComponent reads TransformComponent, so Transform goes first)
  for (const auto &key : kUpdateOrder) {
    const auto it = m_storages.find(key);
    if (it != m_storages.end())
      it->second->Update(dt);
  }

  // 2. Update any storages not in the priority list (future component types)
  for (auto &[key, storage] : m_storages) {
    const bool inOrder = std::ranges::find(kUpdateOrder, key) != kUpdateOrder.end();
    if (!inOrder)
      storage->Update(dt);
  }
}

void World::Render(SDL_Renderer *renderer) {
  for (auto &[type, storage] : m_storages)
    storage->Render(renderer, this);
}
