#pragma once
#include "map/TiledMap.hpp"
#include <string>

// Parses a Tiled JSON (.tmj / .json) file into a TiledMap.
// Tileset image paths are resolved relative to the directory that contains
// the .tmj file, so the caller never needs to know the layout convention.
struct MapLoader {
    // Returns a populated TiledMap, or throws std::runtime_error on failure.
    static TiledMap Load(const std::string& path);
};
