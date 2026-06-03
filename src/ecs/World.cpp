#include "ecs/World.hpp"
#include "ecs/systems/PhysicsSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"

void World::Update(float dt) {
  PhysicsSystem::Update(*this, dt);
}

void World::Render(SDL_Renderer *renderer) {
  RenderSystem::Render(*this, renderer);
}
