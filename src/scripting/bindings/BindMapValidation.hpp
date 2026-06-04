#pragma once
#include <sol/sol.hpp>
#include <optional>
#include "map/TiledMap.hpp"

// Registers world.validate_map into the existing "world" table.
// Call after BindWorld so the table already exists.
// Captures a pointer-to-optional so the lambda always resolves through
// the live optional — safe even when load_tiled_map runs after bind time.
void BindMapValidation(sol::state &lua, std::optional<TiledMap> *lastMap);
