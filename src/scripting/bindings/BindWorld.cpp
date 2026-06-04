#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/bindings/BindWorld.hpp"
#include "scripting/RawBinding.hpp"

#include "ecs/World.hpp"
#include "ecs/Entity.hpp"
#include "ecs/components/AnimationComponent.hpp"
#include "ecs/components/KinematicComponent.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "ecs/components/TextComponent.hpp"
#include "ecs/components/TransformComponent.hpp"

#include "map/MapLoader.hpp"
#include "map/MapSystem.hpp"
#include "rendering/TextureManager.hpp"

#include <SDL3/SDL.h>

void BindWorld(sol::state &lua, World *world, TextureManager *textures) {
  auto w = lua.create_named_table("world");

  // --- Entity lifecycle ---------------------------------------------------
  w.set_function("create_entity",  [world]() -> EntityId { return world->CreateEntity(); });
  w.set_function("destroy_entity", [world](EntityId e)   { world->DestroyEntity(e); });

  // --- TransformComponent -------------------------------------------------
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
      [world](EntityId e) -> std::tuple<float, float> {
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

  // --- KinematicComponent -------------------------------------------------
  w.set_function("add_kinematic", [world](EntityId e) { world->AddComponent<KinematicComponent>(e); });
  w.set_function("set_velocity",
      [world](EntityId e, float vx, float vy) {
        if (auto *k = world->GetComponent<KinematicComponent>(e)) k->velocity = {vx, vy};
      });
  w.set_function("get_velocity",
      [world](EntityId e) -> std::tuple<float, float> {
        if (auto *k = world->GetComponent<KinematicComponent>(e))
          return {k->velocity.x, k->velocity.y};
        return {0.f, 0.f};
      });
  w.set_function("set_acceleration",
      [world](EntityId e, float ax, float ay) {
        if (auto *k = world->GetComponent<KinematicComponent>(e)) k->acceleration = {ax, ay};
      });

  // --- SpriteComponent ----------------------------------------------------
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

  {
    lua_State *L = lua.lua_state();
    RegisterRaw(L, "world", "set_sprite_texture",
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
  w.set_function("set_sprite_tint",
      [world](EntityId e, int r, int g, int b, sol::optional<int> a) {
        if (auto *s = world->GetComponent<SpriteComponent>(e))
          s->tint = SDL_Color{
              static_cast<Uint8>(r), static_cast<Uint8>(g),
              static_cast<Uint8>(b), static_cast<Uint8>(a.value_or(255))};
      });

  // --- AnimationComponent -------------------------------------------------
  w.set_function("add_animation",
      [world](EntityId e, sol::table frames, float dur, sol::optional<bool> loop) {
        auto &anim = world->AddComponent<AnimationComponent>(e);
        anim.frameDuration = dur;
        anim.loop = loop.value_or(true);
        anim.frames.clear();
        anim.frames.reserve(frames.size());
        for (std::size_t i = 1; i <= frames.size(); ++i) {
          sol::table f = frames[i];
          anim.frames.push_back(Frame{
            f.get_or("x", 0.f),
            f.get_or("y", 0.f),
            f.get_or("w", 0.f),
            f.get_or("h", 0.f),
          });
        }
        if (!anim.frames.empty())
          if (auto *s = world->GetComponent<SpriteComponent>(e)) {
            const Frame &f0 = anim.frames[0];
            s->srcRect = {f0.x, f0.y, f0.w, f0.h};
          }
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
            if (!a->frames.empty()) {
              const Frame &f0 = a->frames[0];
              s->srcRect = {f0.x, f0.y, f0.w, f0.h};
            }
        }
      });

  // --- TagComponent -------------------------------------------------------
  w.set_function("add_tag",
      [world](EntityId e, const std::string &tag) {
        world->AddComponent<TagComponent>(e).tag = tag;
      });
  w.set_function("get_tag",
      [world](EntityId e) -> std::string {
        if (auto *t = world->GetComponent<TagComponent>(e)) return t->tag;
        return "";
      });

  // --- TextComponent ------------------------------------------------------
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
  w.set_function("measure_text",
      [world](EntityId e) -> std::tuple<int, int> {
        if (const auto *tc = world->GetComponent<TextComponent>(e))
          return {tc->texW, tc->texH};
        return {0, 0};
      });
  w.set_function("set_text_anchor",
      [world](EntityId e, float ax, float ay) {
        if (auto *tc = world->GetComponent<TextComponent>(e)) {
          tc->anchorX = ax;
          tc->anchorY = ay;
        }
      });

  // --- Visibility ---------------------------------------------------------
  // Hides or shows an entity without destroying it or clearing its state.
  // Works on any entity that has a SpriteComponent, a TextComponent, or both.
  // Silently no-ops if the entity has neither (safe to call on any entity).
  //
  //   world.set_visible(id, false)  -- hide
  //   world.set_visible(id, true)   -- show
  w.set_function("set_visible",
      [world](EntityId e, bool v) {
        if (auto *s  = world->GetComponent<SpriteComponent>(e)) s->visible  = v;
        if (auto *tc = world->GetComponent<TextComponent>(e))   tc->visible = v;
      });

  // --- Collision queries --------------------------------------------------
  w.set_function("get_collisions_for",
      [&lua, world](EntityId entity) -> sol::table {
        auto tbl = lua.create_table(); int i = 1;
        for (const auto &c : world->GetCollisionsFor(entity)) {
          auto row = lua.create_table();
          row["a"] = c.a; row["b"] = c.b;
          tbl[i++] = row;
        }
        return tbl;
      });
  w.set_function("get_collisions_tagged",
      [&lua, world](const std::string &tag) -> sol::table {
        auto tbl = lua.create_table(); int i = 1;
        for (const auto &c : world->GetCollisionsTagged(tag)) {
          auto row = lua.create_table();
          row["a"] = c.a; row["b"] = c.b;
          tbl[i++] = row;
        }
        return tbl;
      });

  // --- Tiled map loading --------------------------------------------------
  w.set_function("load_tiled_map",
      [&lua, world, textures](const std::string &path) -> sol::table {
        TiledMap map;
        try {
            map = MapLoader::Load(path);
        } catch (const std::exception &e) {
            SDL_Log("world.load_tiled_map: %s", e.what());
            return lua.create_table();
        }

        SpawnResult result = MapSystem::Spawn(map, *world, *textures);

        auto tbl = lua.create_table();
        for (std::size_t i = 0; i < result.objectEntities.size(); ++i) {
            const EntityId  e   = result.objectEntities[i];
            const MapObject &mo = result.objects[i];

            auto obj = lua.create_table();
            obj["entity"] = e;
            obj["x"]      = mo.x;
            obj["y"]      = mo.y;
            obj["w"]      = mo.w;
            obj["h"]      = mo.h;
            obj["name"]   = mo.name;
            obj["type"]   = mo.type;

            auto props = lua.create_table();
            for (const auto &[k, v] : mo.properties)
                props[k] = v;
            obj["properties"] = props;

            const std::string key = mo.name.empty()
                ? ("object_" + std::to_string(i + 1))
                : mo.name;
            tbl[key] = obj;
        }
        return tbl;
      });
}
