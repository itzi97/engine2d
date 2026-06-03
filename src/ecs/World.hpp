#pragma once

#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include <SDL3/SDL.h>

// ---------------------------------------------------------------------------
// Type-erased storage interface
// ---------------------------------------------------------------------------
struct IComponentStorage {
  virtual ~IComponentStorage() = default;
  virtual void Erase(EntityId entity)                    = 0;
  virtual void Update(float dt)                          = 0;
  virtual void Render(SDL_Renderer *renderer, World *world) = 0;
};

// ---------------------------------------------------------------------------
// PackedStorage<T>: contiguous vector + O(1) lookup + swap-and-pop removal
// ---------------------------------------------------------------------------
template <typename T>
struct PackedStorage final : IComponentStorage {
  std::vector<T>                        components; // tightly packed data
  std::vector<EntityId>                 entities;   // parallel: entities[i] owns components[i]
  std::unordered_map<EntityId, size_t>  index;      // entity -> slot

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
      // Swap with last element to keep the array packed
      components[slot]         = std::move(components[last]);
      entities[slot]           = entities[last];
      index[entities[slot]]    = slot;
    }

    components.pop_back();
    entities.pop_back();
    index.erase(entity);
  }

  void Update(float dt) override {
    for (auto &c : components)
      c.Update(dt);
  }

  void Render(SDL_Renderer *renderer, World *world) override {
    for (auto &c : components)
      c.Render(renderer, world);
  }
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

  [[nodiscard]] EntityId CreateEntity() { return Entity::Create(); }

  void DestroyEntity(EntityId entity) {
    for (auto &[type, storage] : m_storages)
      storage->Erase(entity);
  }

  template <typename T, typename... Args>
  T &AddComponent(EntityId entity, Args &&...args) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    return GetOrCreateStorage<T>().Add(entity, T{std::forward<Args>(args)...});
  }

  template <typename T>
  [[nodiscard]] T *GetComponent(EntityId entity) {
    static_assert(std::is_base_of_v<Component, T>, "T must derive from Component");
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return nullptr;
    return static_cast<PackedStorage<T> *>(it->second.get())->Get(entity);
  }

  template <typename T>
  void RemoveComponent(EntityId entity) {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return;
    it->second->Erase(entity);
  }

  void Update(float deltaTime);
  void Render(SDL_Renderer *renderer);

private:
  std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_storages;

  template <typename T>
  PackedStorage<T> &GetOrCreateStorage() {
    const auto key = std::type_index(typeid(T));
    const auto it  = m_storages.find(key);
    if (it != m_storages.end())
      return *static_cast<PackedStorage<T> *>(it->second.get());
    auto storage = std::make_unique<PackedStorage<T>>();
    auto *ptr    = storage.get();
    m_storages.emplace(key, std::move(storage));
    return *ptr;
  }
};
