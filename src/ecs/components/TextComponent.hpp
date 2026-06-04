#pragma once
#include "ecs/Component.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

// Text rendered via SDL_ttf.
// TextSystem owns the cached texture; Lua sets text/color and marks it dirty.
struct TextComponent : Component {
  std::string  text;
  int          fontSize{24};
  SDL_Color    color{255, 255, 255, 255};
  int          layer{10};  // default above sprites

  // Anchor point in [0,1] x [0,1].
  // (0,0) = top-left (default, matches legacy behaviour)
  // (0.5,0.5) = centre  (0,1) = bottom-left  (1,0) = top-right  etc.
  // TextSystem subtracts (anchorX*texW, anchorY*texH) from the draw position.
  float anchorX{0.f};
  float anchorY{0.f};

  // Cache managed by TextSystem — do not set from Lua.
  SDL_Texture *texture{nullptr};
  int          texW{0};
  int          texH{0};
  bool         dirty{true};  // true = texture needs rebuild

  TextComponent() = default;
  TextComponent(std::string t, int size, SDL_Color col, int l = 10)
      : text(std::move(t)), fontSize(size), color(col), layer(l) {}
};
