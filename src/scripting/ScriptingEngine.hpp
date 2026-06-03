#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

class World;
class InputManager;
class FontManager;
class TextureManager;

class ScriptingEngine {
public:
  ScriptingEngine();
  ~ScriptingEngine();

  ScriptingEngine(const ScriptingEngine &)            = delete;
  ScriptingEngine &operator=(const ScriptingEngine &) = delete;

  void BindWorld(World *world);
  void BindInput(InputManager *input);
  void BindFonts(FontManager *fonts);
  void BindTextures(TextureManager *textures);

  void CallOnUpdate(float dt);
  bool RunScript(const std::filesystem::path &path);
  bool RunString(std::string_view src, std::string_view chunkName = "?");

  // Clears the on_update callback. Called before a scene function runs
  // so the old scene's update loop doesn't fire during scene init.
  void ResetOnUpdate();

  // Returns any pending scene function set by engine.load_scene() and clears it.
  [[nodiscard]] std::function<void()> TakePendingScene();

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
