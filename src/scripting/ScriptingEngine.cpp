#define SOL_ALL_SAFETIES_ON 1
#include "scripting/ScriptingEngine.hpp"

#include "scripting/bindings/BindAudio.hpp"
#include "scripting/bindings/BindEngine.hpp"
#include "scripting/bindings/BindFonts.hpp"
#include "scripting/bindings/BindTextures.hpp"
#include "scripting/bindings/BindWorld.hpp"

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

ScriptingEngine::ScriptingEngine() {
  m_lua.open_libraries(
      sol::lib::base, sol::lib::math,
      sol::lib::table, sol::lib::string,
      sol::lib::io,    sol::lib::package);
}

void ScriptingEngine::BindWorld(World *world, TextureManager *textures) {
  BindWorldLua(m_lua, world, textures);
}

void ScriptingEngine::BindInput(InputManager *input, SDL_Window *window,
                                SDL_Renderer *renderer, SceneManager *scenes) {
  BindEngine(m_lua, input, window, renderer,
             m_onUpdate, m_pendingScene, scenes,
             nullptr /* world set later via BindWorld */);
}

void ScriptingEngine::BindFonts(FontManager *fonts) {
  BindFontsLua(m_lua, fonts);
}

void ScriptingEngine::BindTextures(TextureManager *textures) {
  BindTexturesLua(m_lua, textures);
}

void ScriptingEngine::BindAudio(AudioManager *audio) {
  BindAudioLua(m_lua, audio);
}

bool ScriptingEngine::RunString(const char *src, const char *chunkName) {
  auto result = m_lua.safe_script(src, chunkName);
  if (!result.valid()) {
    sol::error err = result;
    std::cerr << "[ScriptingEngine] RunString error (" << chunkName
              << "): " << err.what() << '\n';
    return false;
  }
  return true;
}

bool ScriptingEngine::RunFile(const std::string &path) {
  std::ifstream f(path);
  if (!f) {
    std::cerr << "[ScriptingEngine] Cannot open: " << path << '\n';
    return false;
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return RunString(ss.str().c_str(), path.c_str());
}

void ScriptingEngine::CallOnUpdate(float dt) {
  if (!m_onUpdate.valid()) {
    static int warnCount = 0;
    if (warnCount++ < 3)
      std::cerr << "[ScriptingEngine] frame-" << warnCount
                << " on_update.valid() = 0\n";
    return;
  }
  auto result = m_onUpdate(dt);
  if (!result.valid()) {
    sol::error err = result;
    std::cerr << "[ScriptingEngine] on_update error: " << err.what() << '\n';
  }
}

void ScriptingEngine::ResetOnUpdate() {
  m_onUpdate = sol::function{};
}

std::optional<std::function<void()>> ScriptingEngine::TakePendingScene() {
  if (!m_pendingScene) return std::nullopt;
  auto fn = std::move(m_pendingScene);
  m_pendingScene = nullptr;
  return fn;
}
