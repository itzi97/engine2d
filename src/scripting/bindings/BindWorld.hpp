#pragma once
#include <sol/sol.hpp>
#include <optional>
#include "map/TiledMap.hpp"

class World;
class TextureManager;

// lastMap receives the TiledMap produced by world.load_tiled_map() so that
// BindMapValidation can inspect it via world.validate_map().
void BindWorld(sol::state &lua, World *world, TextureManager *textures,
               std::optional<TiledMap> &lastMap);
