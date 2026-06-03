#pragma once
#include <SDL3/SDL.h>
#include <string>

class World;
class FontManager;

// Rebuilds dirty TextComponent textures and draws them each frame.
// Must be called after RenderSystem so text draws on top by default.
struct TextSystem {
  static void Render(World &world, SDL_Renderer *renderer,
                     FontManager &fonts, const std::string &defaultFont);
};
