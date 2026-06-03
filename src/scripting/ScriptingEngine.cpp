// -- All sol2 includes MUST stay in this translation unit ------------------
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
// --------------------------------------------------------------------------

#include "scripting/ScriptingEngine.hpp"

#include "ecs/World.hpp"
#include "ecs/Entity.hpp"
#include "ecs/components/KinematicComponent.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "input/InputManager.hpp"

#include <SDL3/SDL.h>
#include <iostream>

// Map string key names to SDL_Keycode
static SDL_Keycode KeycodeFromString(const std::string &key) {
  if (key == "UP")     return SDLK_UP;
  if (key == "DOWN")   return SDLK_DOWN;
  if (key == "LEFT")   return SDLK_LEFT;
  if (key == "RIGHT")  return SDLK_RIGHT;
  if (key == "W")      return SDLK_W;
  if (key == "S")      return SDLK_S;
  if (key == "A")      return SDLK_A;
  if (key == "D")      return SDLK_D;
  if (key == "SPACE")  return SDLK_SPACE;
  if (key == "RETURN") return SDLK_RETURN;
  if (key == "ESCAPE") return SDLK_ESCAPE;
  if (key == "R")      return SDLK_R;
  if (key == "P")      return SDLK_P;
  if (key == "F")      return SDLK_F;
  return SDLK_UNKNOWN;
}

struct ScriptingEngine::Impl {
  sol::state   lua;
  sol::function onUpdateFn;

  Impl() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                       sol::lib::table, sol::lib::io, sol::lib::os);
    RegisterBaseBindings();
  }

  void RegisterBaseBindings() {
    lua.set_function("log", [](const std::string &msg) {
      std::cout << "[Lua] " << msg << '\n';
    });
  }

  void BindWorld(World *world) {
    auto w = lua.create_named_table("world");

    w.set_function("create_entity", [world]() -> EntityId {
      return world->CreateEntity();
    });
    w.set_function("destroy_entity", [world](EntityId e) {
      world->DestroyEntity(e);
    });
    w.set_function("add_transform",
                   [world](EntityId e, float x, float y, float w_, float h_) {
                     auto &t    = world->AddComponent<TransformComponent>(e);
                     t.position = {x, y};
                     t.size     = {w_, h_};
                   });
    w.set_function("set_position",
                   [world](EntityId e, float x, float y) {
                     if (auto *t = world->GetComponent<TransformComponent>(e))
                       t->position = {x, y};
                   });
    w.set_function("get_position",
                   [world](EntityId e) -> std::tuple<float, float> {
                     if (auto *t = world->GetComponent<TransformComponent>(e))
                       return {t->position.x, t->position.y};
                     return {0.f, 0.f};
                   });
    w.set_function("add_kinematic",
                   [world](EntityId e) {
                     auto &k = world->AddComponent<KinematicComponent>(e, e);
                     k.world = world;
                   });
    w.set_function("set_velocity",
                   [world](EntityId e, float vx, float vy) {
                     if (auto *k = world->GetComponent<KinematicComponent>(e))
                       k->velocity = {vx, vy};
                   });
    w.set_function("get_velocity",
                   [world](EntityId e) -> std::tuple<float, float> {
                     if (auto *k = world->GetComponent<KinematicComponent>(e))
                       return {k->velocity.x, k->velocity.y};
                     return {0.f, 0.f};
                   });
    w.set_function("set_acceleration",
                   [world](EntityId e, float ax, float ay) {
                     if (auto *k = world->GetComponent<KinematicComponent>(e))
                       k->acceleration = {ax, ay};
                   });
    w.set_function("add_sprite",
                   [world](EntityId e, int r, int g, int b, int a) {
                     world->AddComponent<SpriteComponent>(
                         e, e,
                         SDL_Color{static_cast<Uint8>(r), static_cast<Uint8>(g),
                                   static_cast<Uint8>(b),
                                   static_cast<Uint8>(a)});
                   });
    w.set_function("add_tag",
                   [world](EntityId e, const std::string &tag) {
                     world->AddComponent<TagComponent>(e).tag = tag;
                   });
    w.set_function("get_tag", [world](EntityId e) -> std::string {
      if (auto *t = world->GetComponent<TagComponent>(e)) return t->tag;
      return "";
    });
  }

  void BindEngine(InputManager *input) {
    auto eng = lua.create_named_table("engine");

    eng.set_function("on_update", [this](sol::function fn) {
      onUpdateFn = fn;
    });

    // Polling (held)
    eng.set_function("is_key_pressed",
                     [input](const std::string &key) -> bool {
                       return input->IsKeyPressed(KeycodeFromString(key));
                     });

    // Single-frame down
    eng.set_function("is_key_just_pressed",
                     [input](const std::string &key) -> bool {
                       return input->IsKeyJustPressed(KeycodeFromString(key));
                     });

    // Single-frame up
    eng.set_function("is_key_just_released",
                     [input](const std::string &key) -> bool {
                       return input->IsKeyJustReleased(KeycodeFromString(key));
                     });

    // Mouse
    eng.set_function("is_mouse_pressed",
                     [input](int btn) -> bool {
                       return input->IsMousePressed(btn);
                     });
    eng.set_function("is_mouse_just_pressed",
                     [input](int btn) -> bool {
                       return input->IsMouseJustPressed(btn);
                     });
    eng.set_function("is_mouse_just_released",
                     [input](int btn) -> bool {
                       return input->IsMouseJustReleased(btn);
                     });
    eng.set_function("mouse_position",
                     [input]() -> std::tuple<float, float> {
                       return {input->MouseX(), input->MouseY()};
                     });
  }
};

ScriptingEngine::ScriptingEngine() : m_impl(std::make_unique<Impl>()) {}
ScriptingEngine::~ScriptingEngine() = default;

void ScriptingEngine::BindWorld(World *world) {
  m_impl->BindWorld(world);
}

void ScriptingEngine::BindInput(InputManager *input) {
  m_impl->BindEngine(input);
}

void ScriptingEngine::CallOnUpdate(float dt) {
  if (m_impl->onUpdateFn.valid()) {
    auto result = m_impl->onUpdateFn(dt);
    if (!result.valid()) {
      sol::error err = result;
      std::cerr << "[ScriptingEngine] on_update error: " << err.what() << '\n';
    }
  }
}

bool ScriptingEngine::RunScript(const std::filesystem::path &path) {
  auto result =
      m_impl->lua.safe_script_file(path.string(), sol::script_pass_on_error);
  if (!result.valid()) {
    const sol::error err = result;
    std::cerr << "[ScriptingEngine] Error in '" << path
              << "': " << err.what() << '\n';
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
    const sol::error err = result;
    std::cerr << "[ScriptingEngine] Runtime error in '" << chunkName
              << "': " << err.what() << '\n';
    return false;
  }
  return true;
}
