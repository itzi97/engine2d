#include "rendering/FontManager.hpp"
#include <SDL3/SDL.h>

FontManager::FontManager() {
  if (!TTF_Init())
    SDL_Log("TTF_Init failed: %s", SDL_GetError());
}

FontManager::~FontManager() {
  for (auto &[key, font] : m_cache)
    TTF_CloseFont(font);
  TTF_Quit();
}

TTF_Font *FontManager::Get(const std::string &path, int size) {
  const Key key{path, size};
  const auto it = m_cache.find(key);
  if (it != m_cache.end()) return it->second;

  TTF_Font *font = TTF_OpenFont(path.c_str(), static_cast<float>(size));
  if (!font) {
    SDL_Log("FontManager: failed to open '%s' at size %d: %s",
            path.c_str(), size, SDL_GetError());
    return nullptr;
  }
  m_cache.emplace(key, font);
  return font;
}
