#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

class World;
class InputManager;
class FontManager;
class TextureManager;
class AudioManager;
struct SDL_Window;

class ScriptingEngine {
public:
  ScriptingEngine();
  ~ScriptingEngine();

  // Bind subsystems — call once during engine init, before RunScript.
  void BindWorld(World *world, TextureManager *textures);
  void BindInput(InputManager *input, SDL_Window *window);
  void BindFonts(FontManager *fonts);
  void BindTextures(TextureManager *textures);
  void BindAudio(AudioManager *audio);

  // Run a Lua file or inline string. Returns false on error (error logged).
  bool RunScript(const std::filesystem::path &path);
  bool RunString(std::string_view src, std::string_view chunkName = "<string>");

  // Called every frame from the main loop.
  void CallOnUpdate(float dt);

  // Clear engine.on_update (called on scene transitions).
  void ResetOnUpdate();

  // Returns and clears any pending load_scene callback.
  std::function<void()> TakePendingScene();

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
