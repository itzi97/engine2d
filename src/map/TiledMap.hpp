#pragma once
#include <string>
#include <vector>
#include <map>

// ---------------------------------------------------------------------------
// Pure data structs — no SDL, no nlohmann, no sol2 in this header.
// MapLoader fills these; MapSystem consumes them.
// ---------------------------------------------------------------------------

struct Tileset {
    int         firstGid{1};     // GID of the first tile in this tileset
    int         tileW{0};        // width  of one tile in pixels
    int         tileH{0};        // height of one tile in pixels
    int         columns{1};      // tiles per row in the source image
    std::string imagePath;       // resolved absolute (or cwd-relative) path
};

struct TileLayer {
    std::string      name;
    int              width{0};   // layer width  in tiles
    int              height{0};  // layer height in tiles
    std::vector<int> data;       // flat, row-major; 0 = empty cell
};

struct MapObject {
    int         id{0};
    std::string name;
    std::string type;             // Tiled "class" field (Tiled ≥1.9) or "type"
    float       x{0}, y{0};
    float       w{0}, h{0};
    std::map<std::string, std::string> properties; // all custom props as strings
};

struct ObjectLayer {
    std::string           name;
    std::vector<MapObject> objects;
};

struct TiledMap {
    int tileW{0};   // map-level tile size
    int tileH{0};
    std::vector<Tileset>     tilesets;
    std::vector<TileLayer>   tileLayers;
    std::vector<ObjectLayer> objectLayers;
};
