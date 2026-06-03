#pragma once

#include <filesystem>
#include <memory>

class World; // forward declare -- no ECS headers leak out

/// Scripting facade -- sol2 includes are confined to ScriptingEngine.cpp only.
class ScriptingEngine {
public:
  ScriptingEngine();
  ~ScriptingEngine();

  ScriptingEngine(const ScriptingEngine &) = delete;
  ScriptingEngine &operator=(const ScriptingEngine &) = delete;

  /// Must be called after World is constructed, before RunScript.
  void BindWorld(World *world);

  /// Loads and executes a Lua script. Returns false and logs on error.
  [[nodiscard]] bool RunScript(const std::filesystem::path &path);

  /// Calls the Lua engine.on_update callback if registered.
  void CallOnUpdate(float dt);
};
