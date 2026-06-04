#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

class ScriptingEngine;

// SceneManager maps scene names to Lua script paths and drives transitions
// through the existing ScriptingEngine::TakePendingScene() flow.
//
// Registration:
//   scenes.Register("menu",   "scripts/scenes/menu.lua");
//   scenes.Register("ski",    "scripts/scenes/ski.lua");
//   scenes.Register("finish", "scripts/scenes/finish.lua");
//
// Lua-side (via engine.scene.load):
//   engine.scene.load("ski")
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

private:
  ScriptingEngine                                        &m_scripting;
  std::unordered_map<std::string, std::filesystem::path> m_registry;
  std::filesystem::path                                  m_activePath;
};
