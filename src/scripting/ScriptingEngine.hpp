#pragma once
#include <sol/sol.hpp>
#include <functional>
#include <optional>
#include <string>
#include <filesystem>

#include "map/TiledMap.hpp"

class AudioManager;
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
  void BindTextures(TextureManager *textures);
  void BindAudio   (AudioManager *audio);

  // Run Lua source or file.
  bool RunString(const char *src, const char *chunkName);
  bool RunFile  (const std::string &path);
  // Overload used by SceneManager (takes filesystem::path directly).
  bool RunScript(const std::filesystem::path &path);

  // Queue a C++ callable to run at the next scene-transition point.
  void QueueScene(std::function<void()> fn);

  void CallOnUpdate(float dt);
  void ResetOnUpdate();

  std::optional<std::function<void()>> TakePendingScene();

private:
  sol::state             m_lua;
  sol::function          m_onUpdate;
  std::function<void()>  m_pendingScene;
  std::optional<TiledMap> m_lastMap;
};
