#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

class ScriptingEngine;

// SceneManager maps scene names to Lua script paths and drives transitions
// through the existing ScriptingEngine::TakePendingScene() flow.
//
// Registration (from scripts/main.lua via scene.register()):
//   scene.register("ski",    "scripts/scenes/ski.lua")
//   scene.register("menu",   "scripts/scenes/menu.lua")
//
// Lua-side transitions:
//   scene.load("ski")
class SceneManager {
public:
  explicit SceneManager(ScriptingEngine &scripting);

  // Register a named scene pointing at a Lua script file.
  void Register(const std::string &name, const std::filesystem::path &scriptPath);

  // Trigger a transition to the named scene next frame.
  // Safe to call from Lua (via BindEngine) or from C++.
  void Load(const std::string &name);

  // Returns the path of the currently active scene (empty if none loaded).
  [[nodiscard]] const std::filesystem::path &ActivePath() const { return m_activePath; }

  // Returns the full name->path registry.
  // Used by Game.cpp to set up hot-reload watchers after scripts/main.lua
  // has called scene.register() for all scenes.
  [[nodiscard]] const std::unordered_map<std::string, std::filesystem::path> &
  AllScenes() const { return m_registry; }

private:
  ScriptingEngine                                        &m_scripting;
  std::unordered_map<std::string, std::filesystem::path> m_registry;
  std::filesystem::path                                  m_activePath;
};
