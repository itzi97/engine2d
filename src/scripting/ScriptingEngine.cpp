#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/ScriptingEngine.hpp"

#include "scripting/bindings/BindAudio.hpp"
#include "scripting/bindings/BindEngine.hpp"
#include "scripting/bindings/BindMapValidation.hpp"
#include "scripting/bindings/BindTextures.hpp"
#include "scripting/bindings/BindWorld.hpp"

#include "audio/AudioManager.hpp"
#include "input/InputManager.hpp"
#include "map/TiledMap.hpp"
#include "rendering/FontManager.hpp"
#include "rendering/TextureManager.hpp"
#include "ecs/World.hpp"
#include "core/SceneManager.hpp"

#include <SDL3/SDL.h>
#include <iostream>
#include <optional>

struct ScriptingEngine::Impl {
  sol::state            lua;
  sol::function         onUpdateFn;
  std::function<void()> pendingScene;
  bool                  loggedOnUpdateCheck = false;
  std::optional<TiledMap> lastMap;

  Impl() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                       sol::lib::table, sol::lib::io, sol::lib::os);
    lua.set_function("log", [](const std::string &msg) {
      std::cout << "[Lua] " << msg << '\n';
    });
  }
};

ScriptingEngine::ScriptingEngine() : m_impl(std::make_unique<Impl>()) {}
ScriptingEngine::~ScriptingEngine() = default;

void ScriptingEngine::BindWorld(World *world, TextureManager *textures) {
  ::BindWorld(m_impl->lua, world, textures, m_impl->lastMap);
  ::BindMapValidation(m_impl->lua, &m_impl->lastMap);
}
void ScriptingEngine::BindInput(InputManager *input, SDL_Window *window,
                                SceneManager *scenes) {
  ::BindEngine(m_impl->lua, input, window, m_impl->onUpdateFn,
               m_impl->pendingScene, scenes);
}
void ScriptingEngine::BindFonts(FontManager *) {}
void ScriptingEngine::BindTextures(TextureManager *textures) {
  ::BindTextures(m_impl->lua, textures);
}
void ScriptingEngine::BindAudio(AudioManager *audio) {
  ::BindAudio(m_impl->lua, audio);
}

void ScriptingEngine::ResetOnUpdate() {
  m_impl->onUpdateFn          = sol::function{};
  m_impl->loggedOnUpdateCheck = false;
}

void ScriptingEngine::QueueScene(std::function<void()> fn) {
  m_impl->pendingScene = std::move(fn);
}

std::function<void()> ScriptingEngine::TakePendingScene() {
  return std::exchange(m_impl->pendingScene, nullptr);
}

void ScriptingEngine::CallOnUpdate(float dt) {
  if (!m_impl->loggedOnUpdateCheck) {
    m_impl->loggedOnUpdateCheck = true;
    std::cout << "[ScriptingEngine] frame-1 on_update.valid() = "
              << m_impl->onUpdateFn.valid() << '\n';
  }
  if (m_impl->onUpdateFn.valid()) {
    auto result = m_impl->onUpdateFn(dt);
    if (!result.valid()) {
      sol::error err = result;
      std::cerr << "[ScriptingEngine] on_update error: " << err.what() << '\n';
    }
  }
}

bool ScriptingEngine::RunScript(const std::filesystem::path &path) {
  auto result = m_impl->lua.safe_script_file(path.string(), sol::script_pass_on_error);
  if (!result.valid()) {
    sol::error err = result;
    std::cerr << "[ScriptingEngine] Error in '" << path << "': " << err.what() << '\n';
    return false;
  }
  return true;
}

bool ScriptingEngine::RunString(std::string_view src, std::string_view chunkName) {
  lua_State *L = m_impl->lua.lua_state();
  const std::string name = "@" + std::string(chunkName);
  const int loadErr = luaL_loadbuffer(L, src.data(), src.size(), name.c_str());
  if (loadErr != LUA_OK) {
    std::cerr << "[ScriptingEngine] Compile error in '" << chunkName
              << "': " << lua_tostring(L, -1) << '\n';
    lua_pop(L, 1);
    return false;
  }
  sol::protected_function chunk(L, -1);
  lua_pop(L, 1);
  auto result = chunk();
  if (!result.valid()) {
    sol::error err = result;
    std::cerr << "[ScriptingEngine] Runtime error in '" << chunkName
              << "': " << err.what() << '\n';
    return false;
  }
  return true;
}
