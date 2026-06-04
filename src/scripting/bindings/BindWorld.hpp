#pragma once

// Forward-declare sol::state without pulling in sol/sol.hpp.
namespace sol { class state; }
class World;

// Populates the global "world" Lua table with all ECS bindings.
void BindWorld(sol::state &lua, World *world);
