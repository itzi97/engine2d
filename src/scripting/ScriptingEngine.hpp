#pragma once
#include <filesystem>
#include <memory>
#include <string_view>

class World;
class InputManager;
class FontManager;

class ScriptingEngine {
public:
  ScriptingEngine();
  ~ScriptingEngine();

  ScriptingEngine(const ScriptingEngine &)            = delete;
  ScriptingEngine &operator=(const ScriptingEngine &) = delete;

  void BindWorld(World *world);
  void BindInput(InputManager *input);
  void BindFonts(FontManager *fonts);

  void CallOnUpdate(float dt);
  bool RunScript(const std::filesystem::path &path);
  bool RunString(std::string_view src, std::string_view chunkName = "?");

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
