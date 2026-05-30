// Shim: redirect <lua.hpp> to the fetched Lua 5.4 headers.
// This file must live in a directory that appears before /usr/include
// on the compiler search path so it shadows the system lua.hpp.
extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}
