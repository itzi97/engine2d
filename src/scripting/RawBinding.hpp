#pragma once
#include <lua.hpp>

// ---------------------------------------------------------------------------
// RawBinding — register a lua_CFunction closure with a single void* upvalue
// into an already-existing global Lua table.
//
// WHY THIS EXISTS
// sol2 (develop branch, GCC 16) fatally misdeduces lambdas that return raw
// pointers whose type aliases char* (e.g. SDL_Texture*, SDL_Surface*).  The
// internal wrapper<char*> instantiation lacks function_pointer_type and fails
// to compile.  The workaround is to register those functions as plain
// lua_CFunction closures, capturing the needed C++ object as a lightuserdata
// upvalue.  This helper keeps that pattern in one place so future bindings
// with the same constraint don't repeat the stack gymnastics.
//
// USAGE
//   RegisterRaw(L, "engine", "load_texture",
//     [](lua_State *L_) -> int { ... },
//     static_cast<void*>(myManager));
// ---------------------------------------------------------------------------
inline void RegisterRaw(lua_State *L,
                        const char *table,
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
