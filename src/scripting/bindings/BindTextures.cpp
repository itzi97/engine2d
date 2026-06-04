#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

#include "scripting/bindings/BindTextures.hpp"
#include "scripting/RawBinding.hpp"
#include "rendering/TextureManager.hpp"

#include <SDL3/SDL.h>

void BindTextures(sol::state &lua, TextureManager *textures) {
  // load_texture returns an SDL_Texture* as lightuserdata.  We use a raw
  // C closure here because sol2 (develop) mis-deduces lambdas that return
  // pointer types aliasing char* (SDL_Texture*, SDL_Surface*) — see RawBinding.hpp.
  lua_State *L = lua.lua_state();
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
}
