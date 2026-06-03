#include "rendering/TextureManager.hpp"
#include <SDL3_image/SDL_image.h>
#include <SDL3/SDL.h>

TextureManager::TextureManager(SDL_Renderer *renderer) : m_renderer(renderer) {}

TextureManager::~TextureManager() {
  Clear();
}

SDL_Texture *TextureManager::Load(const std::string &path) {
  const auto it = m_cache.find(path);
  if (it != m_cache.end()) return it->second;

  SDL_Surface *surf = IMG_Load(path.c_str());
  if (!surf) {
    SDL_Log("TextureManager: failed to load '%s': %s", path.c_str(), SDL_GetError());
    return nullptr;
  }

  SDL_Texture *tex = SDL_CreateTextureFromSurface(m_renderer, surf);
  SDL_DestroySurface(surf);

  if (!tex) {
    SDL_Log("TextureManager: SDL_CreateTextureFromSurface failed for '%s': %s",
            path.c_str(), SDL_GetError());
    return nullptr;
  }

  // Enable alpha blending so sprites with transparency composite correctly.
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

  m_cache.emplace(path, tex);
  return tex;
}

void TextureManager::Clear() {
  for (auto &[path, tex] : m_cache)
    SDL_DestroyTexture(tex);
  m_cache.clear();
}
