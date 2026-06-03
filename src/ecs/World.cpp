#include "ecs/World.hpp"
#include "ecs/systems/AnimationSystem.hpp"
#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/systems/PhysicsSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"

// Update order:
//   1. PhysicsSystem   -- integrates velocity into position
//   2. AnimationSystem -- advances animation frames, writes srcRect
//   3. (Lua on_update runs here, via ScriptingEngine::CallOnUpdate in Game::Run)
//   4. CollisionSystem -- detects overlaps on fully-committed positions
//
// CollisionSystem::Update is called by Game::Run AFTER ScriptingEngine::CallOnUpdate
// so that manual set_position calls in Lua are visible to collision detection.
// World::Update therefore only runs physics + animation; Game drives the rest.
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
