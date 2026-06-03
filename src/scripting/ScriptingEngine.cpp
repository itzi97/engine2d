// -- All sol2 includes MUST stay in this translation unit ------------------
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
// --------------------------------------------------------------------------

#include "scripting/ScriptingEngine.hpp"

#include "ecs/World.hpp"
#include "ecs/Entity.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

#include <SDL3/SDL.h>
#include <iostream>

struct ScriptingEngine::Impl {
  sol::state lua;
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
                     auto &t = world->AddComponent<TransformComponent>(e);
                     t.position = {x, y};
                     t.size = {w_, h_};
                     t.velocity = {0.f, 0.f};
                   });

    w.set_function("set_position",
                   [world](EntityId e, float x, float y) {
                     if (auto *t =
                             world->GetComponent<TransformComponent>(e))
                       t->position = {x, y};
                   });

    w.set_function("get_position",
                   [world](EntityId e) -> std::tuple<float, float> {
                     if (auto *t =
                             world->GetComponent<TransformComponent>(e))
                       return {t->position.x, t->position.y};
                     return {0.f, 0.f};
                   });

    w.set_function("set_velocity",
                   [world](EntityId e, float vx, float vy) {
                     if (auto *t =
                             world->GetComponent<TransformComponent>(e))
                       t->velocity = {vx, vy};
                   });

    w.set_function("get_velocity",
                   [world](EntityId e) -> std::tuple<float, float> {
                     if (auto *t =
                             world->GetComponent<TransformComponent>(e))
                       return {t->velocity.x, t->velocity.y};
                     return {0.f, 0.f};
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
      if (auto *t = world->GetComponent<TagComponent>(e))
        return t->tag;
      return "";
    });
  }

  void BindEngine() {
    auto eng = lua.create_named_table("engine");

    eng.set_function("on_update", [this](sol::function fn) {
      onUpdateFn = fn;
    });

    eng.set_function("is_key_pressed", [](const std::string &key) -> bool {
      const bool *keys = SDL_GetKeyboardState(nullptr);
      if (key == "UP")     return keys[SDL_SCANCODE_UP];
      if (key == "DOWN")   return keys[SDL_SCANCODE_DOWN];
      if (key == "LEFT")   return keys[SDL_SCANCODE_LEFT];
      if (key == "RIGHT")  return keys[SDL_SCANCODE_RIGHT];
      if (key == "W")      return keys[SDL_SCANCODE_W];
      if (key == "S")      return keys[SDL_SCANCODE_S];
      if (key == "A")      return keys[SDL_SCANCODE_A];
      if (key == "D")      return keys[SDL_SCANCODE_D];
      if (key == "ESCAPE") return keys[SDL_SCANCODE_ESCAPE];
      return false;
    });
  }
};

ScriptingEngine::ScriptingEngine() : m_impl(std::make_unique<Impl>()) {}
ScriptingEngine::~ScriptingEngine() = default;

void ScriptingEngine::BindWorld(World *world) {
  m_impl->BindWorld(world);
  m_impl->BindEngine();
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
  auto result = m_impl->lua.safe_script(
      src, chunkName, sol::script_pass_on_error);
  if (!result.valid()) {
    const sol::error err = result;
    std::cerr << "[ScriptingEngine] Error in '" << chunkName
              << "': " << err.what() << '\n';
    return false;
  }
  return true;
}
