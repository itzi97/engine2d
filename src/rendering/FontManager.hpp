#pragma once
#include <SDL3_ttf/SDL_ttf.h>
#include <memory>
#include <string>
#include <unordered_map>

// Loads and caches TTF_Font* by {path, size}.
// FontManager must outlive any TextComponent textures that reference its fonts.
class FontManager {
public:
  FontManager();
  ~FontManager();

  FontManager(const FontManager &)            = delete;
  FontManager &operator=(const FontManager &) = delete;

  // Returns a cached font, loading it on first request.
  // Returns nullptr and logs on failure.
  [[nodiscard]] TTF_Font *Get(const std::string &path, int size);

private:
  struct Key {
    std::string path;
    int         size;
    bool operator==(const Key &o) const {
      return size == o.size && path == o.path;
    }
  };
  struct KeyHash {
    size_t operator()(const Key &k) const {
      return std::hash<std::string>{}(k.path) ^ (std::hash<int>{}(k.size) << 16);
    }
  };

  std::unordered_map<Key, TTF_Font *, KeyHash> m_cache;
};
