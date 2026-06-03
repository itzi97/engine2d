// -- All sol2 includes MUST stay in this translation unit ------------------
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
// --------------------------------------------------------------------------

#include "scripting/ScriptingEngine.hpp"

#include "ecs/World.hpp"
#include "ecs/Entity.hpp"
#include "ecs/components/AnimationComponent.hpp"
#include "ecs/components/KinematicComponent.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/components/TextComponent.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "input/InputManager.hpp"
#include "rendering/FontManager.hpp"
#include "rendering/TextureManager.hpp"

#include <SDL3/SDL.h>
#include <iostream>

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

// ---------------------------------------------------------------------------
// Raw-API helper: register a lua_CFunction closure into a named table field.
static void SetRawFunction(lua_State *L, const char *table,
                           const char *field,
                           lua_CFunction fn,
                           void *upvalue) {
  lua_pushlightuserdata(L, upvalue);
  lua_pushcclosure(L, fn, 1);
  lua_getglobal(L, table);
  lua_insert(L, -2);
  lua_setfield(L, -2, field);
  lua_pop(L, 1);
}
// ---------------------------------------------------------------------------

struct ScriptingEngine::Impl {
  sol::state    lua;
  sol::function onUpdateFn;
  std::function<void()> pendingScene;
  World        *m_world{nullptr};

  Impl() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                       sol::lib::table, sol::lib::io, sol::lib::os);
    lua.set_function("log", [](const std::string &msg) {
      std::cout << "[Lua] " << msg << '\n';
    });
  }

  void BindWorld(World *world) {
    m_world = world;
    auto w = lua.create_named_table("world");

    w.set_function("create_entity",  [world]() -> EntityId { return world->CreateEntity(); });
    w.set_function("destroy_entity", [world](EntityId e)   { world->DestroyEntity(e); });

    w.set_function("add_transform",
        [world](EntityId e, float x, float y, float w_, float h_) {
          auto &t = world->AddComponent<TransformComponent>(e);
          t.position = {x, y}; t.size = {w_, h_};
        });
    w.set_function("set_position",
        [world](EntityId e, float x, float y) {
          if (auto *t = world->GetComponent<TransformComponent>(e)) t->position = {x, y};
        });
    w.set_function("get_position",
        [world](EntityId e) -> std::tuple<float,float> {
          if (auto *t = world->GetComponent<TransformComponent>(e))
            return {t->position.x, t->position.y};
          return {0.f, 0.f};
        });
    w.set_function("set_rotation",
        [world](EntityId e, float deg) {
          if (auto *t = world->GetComponent<TransformComponent>(e)) t->rotation = deg;
        });
    w.set_function("get_rotation",
        [world](EntityId e) -> float {
          if (auto *t = world->GetComponent<TransformComponent>(e)) return t->rotation;
          return 0.f;
        });

    w.set_function("add_kinematic", [world](EntityId e) { world->AddComponent<KinematicComponent>(e); });
    w.set_function("set_velocity",
        [world](EntityId e, float vx, float vy) {
          if (auto *k = world->GetComponent<KinematicComponent>(e)) k->velocity = {vx, vy};
        });
    w.set_function("get_velocity",
        [world](EntityId e) -> std::tuple<float,float> {
          if (auto *k = world->GetComponent<KinematicComponent>(e))
            return {k->velocity.x, k->velocity.y};
          return {0.f, 0.f};
        });
    w.set_function("set_acceleration",
        [world](EntityId e, float ax, float ay) {
          if (auto *k = world->GetComponent<KinematicComponent>(e)) k->acceleration = {ax, ay};
        });

    w.set_function("add_sprite",
        [world](EntityId e, int r, int g, int b, int a, sol::optional<int> layer) {
          world->AddComponent<SpriteComponent>(
              e,
              SDL_Color{static_cast<Uint8>(r), static_cast<Uint8>(g),
                        static_cast<Uint8>(b), static_cast<Uint8>(a)},
              layer.value_or(0));
        });
    w.set_function("set_layer",
        [world](EntityId e, int layer) {
          if (auto *s = world->GetComponent<SpriteComponent>(e)) s->layer = layer;
        });

    // -- Texture API ----------------------------------------------------------
    {
      lua_State *L = lua.lua_state();
      SetRawFunction(L, "world", "set_sprite_texture",
        [](lua_State *L_) -> int {
          auto *world_ = static_cast<World *>(lua_touserdata(L_, lua_upvalueindex(1)));
          auto  e      = static_cast<EntityId>(luaL_checkinteger(L_, 1));
          if (!lua_islightuserdata(L_, 2))
            return luaL_error(L_, "set_sprite_texture: arg 2 must be a texture (lightuserdata)");
          auto *tex = static_cast<SDL_Texture *>(lua_touserdata(L_, 2));
          float sx = lua_isnoneornil(L_, 3) ? 0.f : static_cast<float>(luaL_checknumber(L_, 3));
          float sy = lua_isnoneornil(L_, 4) ? 0.f : static_cast<float>(luaL_checknumber(L_, 4));
          float sw = lua_isnoneornil(L_, 5) ? 0.f : static_cast<float>(luaL_checknumber(L_, 5));
          float sh = lua_isnoneornil(L_, 6) ? 0.f : static_cast<float>(luaL_checknumber(L_, 6));
          if (auto *s = world_->GetComponent<SpriteComponent>(e)) {
            s->texture = tex;
            s->srcRect = {sx, sy, sw, sh};
          }
          return 0;
        },
        static_cast<void *>(world));
    }

    w.set_function("set_sprite_src",
        [world](EntityId e, float sx, float sy, float sw, float sh) {
          if (auto *s = world->GetComponent<SpriteComponent>(e))
            s->srcRect = {sx, sy, sw, sh};
        });
    w.set_function("set_sprite_flip",
        [world](EntityId e, bool flipX, bool flipY) {
          if (auto *s = world->GetComponent<SpriteComponent>(e)) {
            const int f = (flipX ? SDL_FLIP_HORIZONTAL : 0)
                        | (flipY ? SDL_FLIP_VERTICAL   : 0);
            s->flip = static_cast<SDL_FlipMode>(f);
          }
        });

    // -- Animation API --------------------------------------------------------
    // world.add_animation(e, frames_table, frame_duration [, loop])
    // frames_table: array of {x, y, w, h} sub-tables
    w.set_function("add_animation",
        [world](EntityId e, sol::table frames, float dur, sol::optional<bool> loop) {
          auto &anim = world->AddComponent<AnimationComponent>(e);
          anim.frameDuration = dur;
          anim.loop = loop.value_or(true);
          anim.frames.clear();
          anim.frames.reserve(frames.size());
          for (std::size_t i = 1; i <= frames.size(); ++i) {
            sol::table f = frames[i];
            anim.frames.push_back(SDL_FRect{
              f.get_or("x", 0.f),
              f.get_or("y", 0.f),
              f.get_or("w", 0.f),
              f.get_or("h", 0.f),
            });
          }
          // Initialise SpriteComponent srcRect to frame 0 immediately
          if (!anim.frames.empty())
            if (auto *s = world->GetComponent<SpriteComponent>(e))
              s->srcRect = anim.frames[0];
        });

    w.set_function("set_animation_playing",
        [world](EntityId e, bool playing) {
          if (auto *a = world->GetComponent<AnimationComponent>(e))
            a->playing = playing;
        });

    w.set_function("reset_animation",
        [world](EntityId e) {
          if (auto *a = world->GetComponent<AnimationComponent>(e)) {
            a->currentFrame = 0;
            a->timer = 0.f;
            a->playing = true;
            if (auto *s = world->GetComponent<SpriteComponent>(e))
              if (!a->frames.empty()) s->srcRect = a->frames[0];
          }
        });
    // -------------------------------------------------------------------------

    w.set_function("add_tag",
        [world](EntityId e, const std::string &tag) {
          world->AddComponent<TagComponent>(e).tag = tag;
        });
    w.set_function("get_tag",
        [world](EntityId e) -> std::string {
          if (auto *t = world->GetComponent<TagComponent>(e)) return t->tag;
          return "";
        });

    w.set_function("add_text",
        [world](EntityId e, const std::string &text, int size,
                int r, int g, int b, sol::optional<int> layer) {
          world->AddComponent<TextComponent>(
              e, text, size,
              SDL_Color{static_cast<Uint8>(r), static_cast<Uint8>(g),
                        static_cast<Uint8>(b), 255},
              layer.value_or(10));
        });
    w.set_function("set_text",
        [world](EntityId e, const std::string &text) {
          if (auto *tc = world->GetComponent<TextComponent>(e)) {
            if (tc->text != text) { tc->text = text; tc->dirty = true; }
          }
        });
    w.set_function("set_text_color",
        [world](EntityId e, int r, int g, int b, int a) {
          if (auto *tc = world->GetComponent<TextComponent>(e)) {
            tc->color = {static_cast<Uint8>(r), static_cast<Uint8>(g),
                         static_cast<Uint8>(b), static_cast<Uint8>(a)};
            tc->dirty = true;
          }
        });

    w.set_function("get_collisions_for",
        [this, world](EntityId entity) -> sol::table {
          auto tbl = lua.create_table(); int i = 1;
          for (const auto &c : world->GetCollisionsFor(entity)) {
            auto row = lua.create_table();
            row["a"] = c.a; row["b"] = c.b;
            tbl[i++] = row;
          }
          return tbl;
        });
    w.set_function("get_collisions_tagged",
        [this, world](const std::string &tag) -> sol::table {
          auto tbl = lua.create_table(); int i = 1;
          for (const auto &c : world->GetCollisionsTagged(tag)) {
            auto row = lua.create_table();
            row["a"] = c.a; row["b"] = c.b;
            tbl[i++] = row;
          }
          return tbl;
        });
  }

  void BindEngine(InputManager *input) {
    auto eng = lua.create_named_table("engine");
    eng.set_function("on_update", [this](sol::function fn) { onUpdateFn = fn; });
    eng.set_function("is_key_pressed",
        [input](const std::string &k) { return input->IsKeyPressed(KeycodeFromString(k)); });
    eng.set_function("is_key_just_pressed",
        [input](const std::string &k) { return input->IsKeyJustPressed(KeycodeFromString(k)); });
    eng.set_function("is_key_just_released",
        [input](const std::string &k) { return input->IsKeyJustReleased(KeycodeFromString(k)); });
    eng.set_function("is_mouse_pressed",      [input](int b) { return input->IsMousePressed(b); });
    eng.set_function("is_mouse_just_pressed",  [input](int b) { return input->IsMouseJustPressed(b); });
    eng.set_function("is_mouse_just_released", [input](int b) { return input->IsMouseJustReleased(b); });
    eng.set_function("mouse_position",
        [input]() -> std::tuple<float,float> { return {input->MouseX(), input->MouseY()}; });

    eng.set_function("load_scene", [this](sol::function fn) {
      pendingScene = [fn]() mutable {
        auto result = fn();
        if (!result.valid()) {
          sol::error err = result;
          std::cerr << "[ScriptingEngine] load_scene error: " << err.what() << '\n';
        }
      };
    });

    eng.set_function("quit", [](){ SDL_Event e; e.type = SDL_EVENT_QUIT; SDL_PushEvent(&e); });
  }

  void BindTextures(TextureManager *textures) {
    lua_State *L = lua.lua_state();
    SetRawFunction(L, "engine", "load_texture",
      [](lua_State *L_) -> int {
        auto *mgr = static_cast<TextureManager *>(lua_touserdata(L_, lua_upvalueindex(1)));
        const char *path = luaL_checkstring(L_, 1);
        SDL_Texture *tex = mgr->Load(std::string(path));
        if (tex)
          lua_pushlightuserdata(L_, static_cast<void *>(tex));
        else
          lua_pushnil(L_);
        return 1;
      },
      static_cast<void *>(textures));
  }
};

ScriptingEngine::ScriptingEngine() : m_impl(std::make_unique<Impl>()) {}
ScriptingEngine::~ScriptingEngine() = default;

void ScriptingEngine::BindWorld(World *world)        { m_impl->BindWorld(world); }
void ScriptingEngine::BindInput(InputManager *input) { m_impl->BindEngine(input); }
void ScriptingEngine::BindFonts(FontManager *)       { /* fonts accessed via FONT_PATH in TextSystem */ }
void ScriptingEngine::BindTextures(TextureManager *textures) { m_impl->BindTextures(textures); }

void ScriptingEngine::ResetOnUpdate() {
  m_impl->onUpdateFn = sol::function{};
}

std::function<void()> ScriptingEngine::TakePendingScene() {
  return std::exchange(m_impl->pendingScene, nullptr);
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
