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
// PackedStorage<T>  --  swap-and-pop flat array, O(1) erase
// ---------------------------------------------------------------------------
template <ComponentType T>
struct PackedStorage final : IComponentStorage {
  template <typename Fn>
  void ForEach(Fn &&fn) {
    // Iterate by index so in-loop Erase via swap-and-pop is safe:
    // we re-visit the swapped element by not advancing past it.
    for (size_t i = 0; i < entities.size(); ) {
      const EntityId e = entities[i];
      fn(e, components[i]);
      // Advance only if fn didn't erase this slot (entity still at slot i).
      if (i < entities.size() && entities[i] == e)
        ++i;
    }
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

  // Direct array access — for systems that need two-storage joins.
  // Prefer ForEach for single-storage iteration.
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

  // ---- Entity lifetime ----------------------------------------------------

  [[nodiscard]] EntityId CreateEntity() {
    if (!m_freeList.empty()) {
      const EntityId id = m_freeList.front();
      m_freeList.pop();
      return id;
    }
    return Entity::Create();
  }

  // Safe to call at any time, including from Lua on_update.
  // Actual erasure is deferred until FlushDestroyQueue().
  void DestroyEntity(EntityId entity) {
    m_pendingDestroy.push_back(entity);
  }

  // Flush all pending DestroyEntity calls.
  // Called by Game::Run after Update + CallOnUpdate + RunCollision,
  // so no system is iterating storage when erasure happens.
  void FlushDestroyQueue() {
    for (const EntityId e : m_pendingDestroy) {
      for (auto &[type, storage] : m_storages)
        storage->Erase(e);
      m_freeList.push(e);
    }
    m_pendingDestroy.clear();
  }

  // ---- Component access ---------------------------------------------------

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

  // Iterate all entities that have component T.
  // Safe to call DestroyEntity inside fn — destruction is deferred.
  template <ComponentType T, typename Fn>
  void ForEach(Fn &&fn) {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return;
    static_cast<PackedStorage<T> *>(it->second.get())->ForEach(std::forward<Fn>(fn));
  }

  // ---- Frame entry points (called by Game::Run) ---------------------------
  void Update(float deltaTime);   // physics only
  void RunCollision();            // AABB detection on final positions
  void Render(SDL_Renderer *renderer);

private:
  std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_storages;
  std::queue<EntityId>  m_freeList;
  std::vector<EntityId> m_pendingDestroy;

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

  // GetStorage<T>() is private. Systems use World::ForEach or GetComponent.
  // TextSystem needs direct storage access for layer sorting — granted via friend.
  template <ComponentType T>
  [[nodiscard]] PackedStorage<T> *GetStorage() {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return nullptr;
    return static_cast<PackedStorage<T> *>(it->second.get());
  }

  friend struct TextSystem;
  friend struct RenderSystem;
  friend struct CollisionSystem;
  friend struct PhysicsSystem;
};
