#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

class AudioManager;
class FontManager;
class InputManager;
class TextureManager;
class World;

class ScriptingEngine {
public:
  ScriptingEngine();
  ~ScriptingEngine();

  void BindWorld(World *world);
  void BindInput(InputManager *input);
  void BindFonts(FontManager *fonts);
  void BindTextures(TextureManager *textures);
  void BindAudio(AudioManager *audio);

  void ResetOnUpdate();
  [[nodiscard]] std::function<void()> TakePendingScene();

  void CallOnUpdate(float dt);
  bool RunScript(const std::filesystem::path &path);
  bool RunString(std::string_view src, std::string_view chunkName = "?");
};
