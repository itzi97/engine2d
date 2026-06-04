#include "ecs/systems/RenderSystem.hpp"
#include "ecs/World.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

#include <SDL3/SDL.h>
#include <cmath>

// Rotate point (px, py) around pivot (cx, cy) by angle radians.
static inline void RotatePoint(float px, float py,
                                float cx, float cy,
                                float cos_a, float sin_a,
                                float &ox, float &oy) {
  const float dx = px - cx;
  const float dy = py - cy;
  ox = cx + dx * cos_a - dy * sin_a;
  oy = cy + dx * sin_a + dy * cos_a;
}

void RenderSystem::Render(World &world, SDL_Renderer *renderer) {
  // ForEachSorted visits sprites in ascending layer order.
  world.ForEachSorted<SpriteComponent>([&](EntityId entity, const SpriteComponent &s) {
    if (!s.visible) return;  // hidden — skip entirely

    const auto *t = world.GetComponent<TransformComponent>(entity);
    if (!t) return;

    const SDL_FRect dst{
        t->position.x, t->position.y,
        t->size.x * t->scale.x,
        t->size.y * t->scale.y,
    };

    if (s.texture) {
      SDL_SetTextureColorMod(s.texture, s.tint.r, s.tint.g, s.tint.b);
      SDL_SetTextureAlphaMod(s.texture, s.tint.a);

      const SDL_FRect *src = (s.srcRect.w > 0.f) ? &s.srcRect : nullptr;
      SDL_RenderTextureRotated(renderer, s.texture, src, &dst,
                               static_cast<double>(t->rotation),
                               nullptr,
                               s.flip);

      SDL_SetTextureColorMod(s.texture, 255, 255, 255);
      SDL_SetTextureAlphaMod(s.texture, 255);
    } else {
      // Colour-only sprite. Use SDL_RenderFillRect for zero rotation (fast
      // path) and SDL_RenderGeometry for rotated quads so rotation is
      // respected, matching the behaviour of SDL_RenderTextureRotated.
      if (t->rotation == 0.f) {
        SDL_SetRenderDrawColor(renderer, s.color.r, s.color.g, s.color.b, s.color.a);
        SDL_RenderFillRect(renderer, &dst);
      } else {
        // Build the four corners of the rect, then rotate them around the
        // rect's centre (SDL convention: rotation is around the centre).
        const float cx = dst.x + dst.w * 0.5f;
        const float cy = dst.y + dst.h * 0.5f;
        // SDL rotation is clockwise degrees.
        const float rad   = t->rotation * (static_cast<float>(M_PI) / 180.f);
        const float cos_a = std::cos(rad);
        const float sin_a = std::sin(rad);

        float x0, y0, x1, y1, x2, y2, x3, y3;
        RotatePoint(dst.x,          dst.y,          cx, cy, cos_a, sin_a, x0, y0);
        RotatePoint(dst.x + dst.w,  dst.y,          cx, cy, cos_a, sin_a, x1, y1);
        RotatePoint(dst.x + dst.w,  dst.y + dst.h,  cx, cy, cos_a, sin_a, x2, y2);
        RotatePoint(dst.x,          dst.y + dst.h,  cx, cy, cos_a, sin_a, x3, y3);

        const SDL_FColor fc{
            s.color.r / 255.f,
            s.color.g / 255.f,
            s.color.b / 255.f,
            s.color.a / 255.f,
        };

        SDL_Vertex verts[4] = {
            { {x0, y0}, fc, {0.f, 0.f} },
            { {x1, y1}, fc, {1.f, 0.f} },
            { {x2, y2}, fc, {1.f, 1.f} },
            { {x3, y3}, fc, {0.f, 1.f} },
        };
        // Two triangles: (0,1,2) and (0,2,3)
        const int indices[6] = { 0, 1, 2, 0, 2, 3 };
        SDL_RenderGeometry(renderer, nullptr, verts, 4, indices, 6);
      }
    }
  });
}
