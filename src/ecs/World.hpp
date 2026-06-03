#pragma once

#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>

using ComponentStorage =
    std::unordered_map<EntityId, std::unique_ptr<Component>>;

class World {
public:
  World() = default;
  ~World() = default;

  World(const World &) = delete;
  World &operator=(const World &) = delete;

  [[nodiscard]] EntityId CreateEntity() { return Entity::Create(); }
  void DestroyEntity(EntityId entity) {
    for (auto &[type, storage] : m_storages) {
      storage.erase(entity);
    }
  }

  template <typename T, typename... Args>
  T &AddComponent(EntityId entity, Args &&...args) {
    static_assert(std::is_base_of_v<Component, T>,
                  "T must derive from Component");
    auto &storage = m_storages[std::type_index(typeid(T))];
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T &ref = *ptr;
    storage[entity] = std::move(ptr);
    return ref;
  }

  template <typename T> [[nodiscard]] T *GetComponent(EntityId entity) {
    static_assert(std::is_base_of_v<Component, T>,
                  "T must derive from Component");
    const auto sIt = m_storages.find(std::type_index(typeid(T)));
    if (sIt == m_storages.end())
      return nullptr;
    const auto cIt = sIt->second.find(entity);
    if (cIt == sIt->second.end())
      return nullptr;
    return static_cast<T *>(cIt->second.get());
  }

  template <typename T> void RemoveComponent(EntityId entity) {
    const auto sIt = m_storages.find(std::type_index(typeid(T)));
    if (sIt == m_storages.end())
      return;
    sIt->second.erase(entity);
  }

  void Update(float deltaTime);
  void Render(SDL_Renderer *renderer);

private:
  std::unordered_map<std::type_index, ComponentStorage> m_storages;
};
