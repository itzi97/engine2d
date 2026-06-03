#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>

// Loads and caches SDL_Texture* by path.
// Requires the SDL_Renderer* used at construction — the same renderer must be
// passed for all Load() calls. Manager must outlive any SpriteComponent that
// holds a raw observing pointer obtained from it.
class TextureManager {
public:
  explicit TextureManager(SDL_Renderer *renderer);
  ~TextureManager();

  TextureManager(const TextureManager &)            = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  // Returns a cached texture, loading it on first request.
  // Returns nullptr and logs on failure.
  [[nodiscard]] SDL_Texture *Load(const std::string &path);

  // Destroy all cached textures (call before renderer recreation).
  void Clear();

private:
  SDL_Renderer *m_renderer;
  std::unordered_map<std::string, SDL_Texture *> m_cache;
};
