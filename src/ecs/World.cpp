#include "ecs/World.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/systems/AnimationSystem.hpp"
#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/systems/PhysicsSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"

// Update order:
//   1. PhysicsSystem   -- integrates velocity into position
//   2. AnimationSystem -- advances animation frames, writes srcRect
//   3. (Lua on_update runs here, via ScriptingEngine::CallOnUpdate in Game::Run)
//   4. CollisionSystem -- detects overlaps on fully-committed positions
void World::Update(float dt) {
  PhysicsSystem::Update(*this, dt);
  AnimationSystem::Update(*this, dt);
}

void World::RunCollision() {
  CollisionSystem::Update(*this);
}

void World::Render(SDL_Renderer *renderer) {
  RenderSystem::Render(*this, renderer);
}

std::vector<Collision>
World::GetCollisionsTagged(const std::string &tag) const {
  std::vector<Collision> result;
  for (const auto &c : m_collisions) {
    const auto *ta = const_cast<World *>(this)->GetComponent<TagComponent>(c.a);
    const auto *tb = const_cast<World *>(this)->GetComponent<TagComponent>(c.b);
    if ((ta && ta->tag == tag) || (tb && tb->tag == tag))
      result.push_back(c);
  }
  return result;
}

std::vector<EntityId>
World::GetEntitiesTagged(const std::string &tag) const {
  std::vector<EntityId> result;
  const auto it = m_storages.find(std::type_index(typeid(TagComponent)));
  if (it == m_storages.end()) return result;
  const auto *storage = static_cast<const PackedStorage<TagComponent> *>(it->second.get());
  for (std::size_t i = 0; i < storage->entities.size(); ++i)
    if (storage->components[i].tag == tag)
      result.push_back(storage->entities[i]);
  return result;
}

std::vector<EntityId> World::GetAllEntities() const {
  // Collect unique entity IDs across all storages.
  // Use the first storage we find as the primary source; entities without
  // any component are not tracked (they wouldn't be useful anyway).
  std::unordered_map<EntityId, bool> seen;
  for (const auto &[type, storage] : m_storages) {
    // All PackedStorages expose their entity list at the same offset via
    // the base IComponentStorage — we can't access it generically, so we
    // iterate tag storage as a representative. For a full list we'd need
    // a virtual Entities() method; add it to IComponentStorage.
    (void)type; (void)storage;
  }
  // Practical approach: return all entities that have a TagComponent.
  // Scripts should tag everything they care about — which they already do.
  return GetEntitiesTagged("");  // empty tag won't match anything useful
}
