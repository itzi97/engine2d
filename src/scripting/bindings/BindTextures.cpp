#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/bindings/BindTextures.hpp"
#include "scripting/RawBinding.hpp"
#include "rendering/TextureManager.hpp"

#include <SDL3/SDL.h>

void BindTextures(sol::state &lua, TextureManager *textures) {
  lua_State *L = lua.lua_state();

  // engine.load_texture(path) -> lightuserdata (SDL_Texture*) | nil
  RegisterRaw(L, "engine", "load_texture",
    [](lua_State *L_) -> int {
      auto *mgr  = static_cast<TextureManager *>(lua_touserdata(L_, lua_upvalueindex(1)));
      const char *path = luaL_checkstring(L_, 1);
      SDL_Texture *tex = mgr->Load(std::string(path));
      if (tex)
        lua_pushlightuserdata(L_, static_cast<void *>(tex));
      else
        lua_pushnil(L_);
      return 1;
    },
    static_cast<void *>(textures));

  // engine.texture_size(tex) -> width, height
  // Returns the pixel dimensions of a texture previously loaded with
  // engine.load_texture.  Useful for debugging atlas layout.
  RegisterRaw(L, "engine", "texture_size",
    [](lua_State *L_) -> int {
      if (!lua_islightuserdata(L_, 1)) {
        lua_pushnumber(L_, 0);
        lua_pushnumber(L_, 0);
        return 2;
      }
      auto *tex = static_cast<SDL_Texture *>(lua_touserdata(L_, 1));
      float w = 0.f, h = 0.f;
      SDL_GetTextureSize(tex, &w, &h);
      lua_pushnumber(L_, static_cast<lua_Number>(w));
      lua_pushnumber(L_, static_cast<lua_Number>(h));
      return 2;
    },
    nullptr);
}
