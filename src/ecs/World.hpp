#pragma once

#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"
#include "ecs/components/KinematicComponent.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

// ---------------------------------------------------------------------------
// Type-erased storage interface
// ---------------------------------------------------------------------------
struct IComponentStorage {
  virtual ~IComponentStorage() = default;
  virtual void Erase(EntityId entity) = 0;
};

// ---------------------------------------------------------------------------
// PackedStorage<T>
// ---------------------------------------------------------------------------
template <ComponentType T>
struct PackedStorage final : IComponentStorage {
  template <typename Fn>
  void ForEach(Fn &&fn) {
    for (size_t i = 0; i < entities.size(); ++i)
      fn(entities[i], components[i]);
  }

  template <typename Fn>
  void ForEach(Fn &&fn) const {
    for (size_t i = 0; i < entities.size(); ++i)
      fn(entities[i], components[i]);
  }

  T &Add(EntityId entity, T &&component) {
    index[entity] = components.size();
    entities.push_back(entity);
    components.push_back(std::move(component));
    return components.back();
  }

  T *Get(EntityId entity) {
    const auto it = index.find(entity);
    if (it == index.end()) return nullptr;
    return &components[it->second];
  }

  void Erase(EntityId entity) override {
    const auto it = index.find(entity);
    if (it == index.end()) return;

    const size_t slot = it->second;
    const size_t last = components.size() - 1;

    if (slot != last) {
      components[slot]      = std::move(components[last]);
      entities[slot]        = entities[last];
      index[entities[slot]] = slot;
    }

    components.pop_back();
    entities.pop_back();
    index.erase(entity);
  }

  // Raw arrays — only use when ForEach is insufficient (e.g. two-storage joins).
  std::vector<T>                       components;
  std::vector<EntityId>                entities;
  std::unordered_map<EntityId, size_t> index;
};

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------
class World {
public:
  World()  = default;
  ~World() = default;

  World(const World &)            = delete;
  World &operator=(const World &) = delete;

  [[nodiscard]] EntityId CreateEntity() {
    if (!m_freeList.empty()) {
      const EntityId id = m_freeList.front();
      m_freeList.pop();
      return id;
    }
    return Entity::Create();
  }

  void DestroyEntity(EntityId entity) {
    for (auto &[type, storage] : m_storages)
      storage->Erase(entity);
    m_freeList.push(entity);
  }

  template <ComponentType T, typename... Args>
  T &AddComponent(EntityId entity, Args &&...args) {
    return GetOrCreateStorage<T>().Add(entity, T{std::forward<Args>(args)...});
  }

  template <ComponentType T>
  [[nodiscard]] T *GetComponent(EntityId entity) {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return nullptr;
    return static_cast<PackedStorage<T> *>(it->second.get())->Get(entity);
  }

  template <ComponentType T>
  void RemoveComponent(EntityId entity) {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return;
    it->second->Erase(entity);
  }

  template <ComponentType T, typename Fn>
  void ForEach(Fn &&fn) {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return;
    static_cast<PackedStorage<T> *>(it->second.get())->ForEach(std::forward<Fn>(fn));
  }

  template <ComponentType T>
  [[nodiscard]] PackedStorage<T> *GetStorage() {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return nullptr;
    return static_cast<PackedStorage<T> *>(it->second.get());
  }

  // Called by Game::Run in order: Update -> CallOnUpdate -> RunCollision -> Render
  void Update(float deltaTime);  // physics only
  void RunCollision();           // AABB detection on final positions
  void Render(SDL_Renderer *renderer);

private:
  std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_storages;
  std::queue<EntityId> m_freeList;

  template <ComponentType T>
  PackedStorage<T> &GetOrCreateStorage() {
    const auto key = std::type_index(typeid(T));
    const auto it  = m_storages.find(key);
    if (it != m_storages.end())
      return *static_cast<PackedStorage<T> *>(it->second.get());
    auto  storage = std::make_unique<PackedStorage<T>>();
    auto *ptr     = storage.get();
    m_storages.emplace(key, std::move(storage));
    return *ptr;
  }
};
