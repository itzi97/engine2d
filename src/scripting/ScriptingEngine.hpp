#pragma once
#include <sol/sol.hpp>
#include <functional>
#include <optional>
#include <string>

class AudioManager;
class FontManager;
class InputManager;
class SceneManager;
class TextureManager;
class World;
struct SDL_Window;
struct SDL_Renderer;

class ScriptingEngine {
public:
  ScriptingEngine();

  // Bind subsystems — call once during Game::Initialize before RunString.
  void BindWorld   (World *world, TextureManager *textures);
  void BindInput   (InputManager *input, SDL_Window *window,
                    SDL_Renderer *renderer, SceneManager *scenes);
  void BindFonts   (FontManager *fonts);
  void BindTextures(TextureManager *textures);
  void BindAudio   (AudioManager *audio);

  bool RunString(const char *src, const char *chunkName);
  bool RunFile  (const std::string &path);

  void CallOnUpdate(float dt);
  void ResetOnUpdate();

  std::optional<std::function<void()>> TakePendingScene();

private:
  sol::state             m_lua;
  sol::function          m_onUpdate;
  std::function<void()>  m_pendingScene;
};
