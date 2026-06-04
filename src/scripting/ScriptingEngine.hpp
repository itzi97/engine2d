#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

class World;
class InputManager;
class FontManager;
class SceneManager;
class TextureManager;
class AudioManager;
struct SDL_Window;

class ScriptingEngine {
public:
  ScriptingEngine();
  ~ScriptingEngine();

  void BindWorld(World *world, TextureManager *textures);
  void BindInput(InputManager *input, SDL_Window *window,
                 SceneManager *scenes = nullptr);
  void BindFonts(FontManager *fonts);
  void BindTextures(TextureManager *textures);
  void BindAudio(AudioManager *audio);

  bool RunScript(const std::filesystem::path &path);
  bool RunString(std::string_view src, std::string_view chunkName = "<string>");

  void CallOnUpdate(float dt);
  void ResetOnUpdate();

  void QueueScene(std::function<void()> fn);
  std::function<void()> TakePendingScene();

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
