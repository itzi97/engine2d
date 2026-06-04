#include "core/SceneManager.hpp"
#include "scripting/ScriptingEngine.hpp"

#include <iostream>

SceneManager::SceneManager(ScriptingEngine &scripting)
    : m_scripting(scripting) {}

void SceneManager::Register(const std::string &name,
                            const std::filesystem::path &scriptPath) {
  m_registry[name] = scriptPath;
}

void SceneManager::Load(const std::string &name) {
  const auto it = m_registry.find(name);
  if (it == m_registry.end()) {
    std::cerr << "[SceneManager] Unknown scene: '" << name << "'\n";
    return;
  }

  const std::filesystem::path path = it->second;
  m_activePath = path;

  // Queue a scene transition through the existing pending-scene flow.
  // Game::Update calls TakePendingScene() which clears the world and
  // resets on_update before calling this lambda.
  m_scripting.QueueScene([this, path]() {
    if (!m_scripting.RunScript(path)) {
      std::cerr << "[SceneManager] Failed to run scene: " << path << '\n';
    }
  });
}
