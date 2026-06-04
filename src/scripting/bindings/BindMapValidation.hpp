#pragma once
#include <sol/sol.hpp>
#include "map/TiledMap.hpp"

// Registers world.validate_map into the existing "world" table.
// Call after BindWorld so the table already exists.
void BindMapValidation(sol::state &lua, const TiledMap *lastMap);
