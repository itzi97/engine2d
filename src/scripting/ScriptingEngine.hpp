#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

class World;
class InputManager;

/// Scripting facade -- sol2 includes are confined to ScriptingEngine.cpp only.
class ScriptingEngine {
public:
  ScriptingEngine();
  ~ScriptingEngine();

  ScriptingEngine(const ScriptingEngine &)            = delete;
  ScriptingEngine &operator=(const ScriptingEngine &) = delete;

  /// Must be called after World is constructed, before RunScript/RunString.
  void BindWorld(World *world);

  /// Must be called after BindWorld, before RunScript/RunString.
  void BindInput(InputManager *input);

  /// Loads and executes a Lua script from disk. Returns false on error.
  [[nodiscard]] bool RunScript(const std::filesystem::path &path);

  /// Executes Lua source from an in-memory string (e.g. embedded header).
  [[nodiscard]] bool RunString(std::string_view src,
                               std::string_view chunkName = "embedded");

  /// Calls the Lua engine.on_update callback if registered.
  void CallOnUpdate(float dt);

private:
  struct Impl;
  std::unique_ptr<Impl> m_impl;
};
