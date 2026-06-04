#pragma once

#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"

#include <algorithm>
#include <memory>
#include <numeric>
#include <queue>
#include <span>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

// ---------------------------------------------------------------------------
// Collision result
// ---------------------------------------------------------------------------
struct Collision {
  EntityId a;
  EntityId b;
};

// ---------------------------------------------------------------------------
// Type-erased storage interface
// ---------------------------------------------------------------------------
struct IComponentStorage {
  virtual ~IComponentStorage() = default;
  virtual void Erase(EntityId entity) = 0;
  virtual void Clear() = 0;
};

// ---------------------------------------------------------------------------
// PackedStorage<T>
// ---------------------------------------------------------------------------
template <ComponentType T>
struct PackedStorage final : IComponentStorage {
  template <typename Fn>
  void ForEach(Fn &&fn) {
    for (size_t i = 0; i < entities.size(); ) {
      const EntityId e = entities[i];
      fn(e, components[i]);
      if (i < entities.size() && entities[i] == e) ++i;
    }
  }

  template <typename Fn>
  void ForEach(Fn &&fn) const {
    for (size_t i = 0; i < entities.size(); ++i)
      fn(entities[i], components[i]);
  }

  template <typename Fn>
  void ForEachSorted(Fn &&fn) {
    const size_t n = entities.size();
    if (n == 0) return;

    if (sortDirty) {
      sortOrder.resize(n);
      std::iota(sortOrder.begin(), sortOrder.end(), 0);
      std::stable_sort(sortOrder.begin(), sortOrder.end(), [&](size_t a, size_t b) {
        return components[a].layer < components[b].layer;
      });
      sortDirty = false;
    }

    for (const size_t i : sortOrder)
      fn(entities[i], components[i]);
  }

  void MarkSortDirty() { sortDirty = true; }

  T &Add(EntityId entity, T &&component) {
    index[entity] = components.size();
    entities.push_back(entity);
    components.push_back(std::move(component));
    sortDirty = true;
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
    sortDirty = true;
  }

  void Clear() override {
    components.clear();
    entities.clear();
    index.clear();
    sortOrder.clear();
    sortDirty = true;
  }

  std::vector<T>                       components;
  std::vector<EntityId>                entities;
  std::unordered_map<EntityId, size_t> index;
  std::vector<size_t>                  sortOrder;
  bool                                 sortDirty = true;
};

// ---------------------------------------------------------------------------
// ComponentView<T>
// ---------------------------------------------------------------------------
template <ComponentType T>
struct ComponentView {
  std::span<const T>        components;
  std::span<const EntityId> entities;
  [[nodiscard]] size_t size()  const noexcept { return entities.size(); }
  [[nodiscard]] bool   empty() const noexcept { return entities.empty(); }
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
    return m_nextId++;
  }

  void DestroyEntity(EntityId entity) {
    m_pendingDestroy.push_back(entity);
  }

  void FlushDestroyQueue() {
    for (const EntityId e : m_pendingDestroy) {
      for (auto &[type, storage] : m_storages)
        storage->Erase(e);
      m_freeList.push(e);
    }
    m_pendingDestroy.clear();
  }

  void ClearAll() {
    for (auto &[type, storage] : m_storages)
      storage->Clear();
    while (!m_freeList.empty()) m_freeList.pop();
    m_pendingDestroy.clear();
    m_collisions.clear();
    m_nextId = 0;
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

  template <ComponentType T, typename Fn>
  void ForEachSorted(Fn &&fn) {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return;
    static_cast<PackedStorage<T> *>(it->second.get())->ForEachSorted(std::forward<Fn>(fn));
  }

  template <ComponentType T>
  void MarkSortDirty() {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return;
    static_cast<PackedStorage<T> *>(it->second.get())->MarkSortDirty();
  }

  template <ComponentType T>
  [[nodiscard]] ComponentView<T> View() {
    const auto it = m_storages.find(std::type_index(typeid(T)));
    if (it == m_storages.end()) return {};
    auto *s = static_cast<PackedStorage<T> *>(it->second.get());
    return { std::span<const T>{s->components},
             std::span<const EntityId>{s->entities} };
  }

  void ClearCollisions() { m_collisions.clear(); }
  void AddCollision(Collision c) { m_collisions.push_back(c); }

  [[nodiscard]] const std::vector<Collision> &GetCollisions() const {
    return m_collisions;
  }

  [[nodiscard]] std::vector<Collision> GetCollisionsFor(EntityId entity) const {
    std::vector<Collision> result;
    for (const auto &c : m_collisions)
      if (c.a == entity || c.b == entity)
        result.push_back(c);
    return result;
  }

  // Implemented in World.cpp (requires TagComponent.hpp)
  [[nodiscard]] std::vector<Collision>
  GetCollisionsTagged(const std::string &tag) const;

  // Returns all entity IDs that have a TagComponent matching tag.
  [[nodiscard]] std::vector<EntityId>
  GetEntitiesTagged(const std::string &tag) const;

  // Returns every live entity ID (has at least one component).
  [[nodiscard]] std::vector<EntityId> GetAllEntities() const;

  void Update(float deltaTime);
  void RunCollision();
  void Render(SDL_Renderer *renderer);

private:
  std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_storages;
  std::queue<EntityId>   m_freeList;
  std::vector<EntityId>  m_pendingDestroy;
  std::vector<Collision> m_collisions;
  EntityId               m_nextId = 0;

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
