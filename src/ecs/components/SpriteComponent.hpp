#pragma once
#include "ecs/Component.hpp"
#include "ecs/Entity.hpp"
#include <SDL3/SDL.h>

// Forward declare to avoid circular includes at render time
class World;

class SpriteComponent : public Component {
public:
  SpriteComponent(EntityId owner,
                  SDL_Color color = SDL_Color{255, 255, 255, 255})
      : m_owner(owner), m_color(color) {}

  // World pointer injected at render time by World::Render()
  void Render(SDL_Renderer *renderer, World *world) override;

private:
  EntityId  m_owner;
  SDL_Color m_color;
};
