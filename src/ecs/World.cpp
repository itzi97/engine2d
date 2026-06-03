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
