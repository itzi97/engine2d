#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct Tileset {
    int         firstGid  = 1;
    int         tileW     = 16;
    int         tileH     = 16;
    int         spacing   = 0;   // px gap between tiles in the atlas image
    int         columns   = 1;
    std::string imagePath;
};

struct TileLayer {
    std::string      name;
    int              width  = 0;
    int              height = 0;
    std::vector<int> data;
};

struct MapObject {
    int         id = 0;
    std::string name;
    std::string type;
    float       x = 0, y = 0, w = 0, h = 0;
    std::unordered_map<std::string, std::string> properties;
};

struct ObjectLayer {
    std::string           name;
    std::vector<MapObject> objects;
};

struct TiledMap {
    int                    tileW = 16;
    int                    tileH = 16;
    std::vector<Tileset>   tilesets;
    std::vector<TileLayer> tileLayers;
    std::vector<ObjectLayer> objectLayers;
};
