#pragma once
#include "ecs/Component.hpp"
#include "ecs/components/TransformComponent.hpp"
#include <SDL3/SDL.h>

class SpriteComponent : public Component {
public:
  SpriteComponent(TransformComponent *transform,
                  SDL_Color color = SDL_Color{255, 255, 255, 255})
      : m_transform(transform), m_color(color) {}

  void Render(SDL_Renderer *renderer) override {
    if (!m_transform || !renderer)
      return;
    SDL_FRect rect{
        m_transform->position.x,
        m_transform->position.y,
        m_transform->size.x * m_transform->scale.x,
        m_transform->size.y * m_transform->scale.y,
    };

    SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b,
                           m_color.a);
    SDL_RenderFillRect(renderer, &rect);
  }

private:
  TransformComponent *m_transform; // non-owning
  SDL_Color m_color;
};
