#include "map/MapLoader.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

static std::string ResolveRelative(const fs::path& base, const std::string& rel) {
    // base is the directory containing the .tmj file
    return (base / rel).lexically_normal().string();
}

TiledMap MapLoader::Load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("MapLoader: cannot open " + path);

    json j;
    try { f >> j; }
    catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("MapLoader: JSON parse error: ") + e.what());
    }

    const fs::path mapDir = fs::path(path).parent_path();

    TiledMap map;
    map.tileW = j.value("tilewidth",  16);
    map.tileH = j.value("tileheight", 16);

    // ---- Tilesets -------------------------------------------------------
    for (const auto& ts : j.value("tilesets", json::array())) {
        Tileset tileset;
        tileset.firstGid  = ts.value("firstgid", 1);
        tileset.tileW     = ts.value("tilewidth",  map.tileW);
        tileset.tileH     = ts.value("tileheight", map.tileH);
        tileset.columns   = ts.value("columns", 1);

        // Tiled can embed the tileset inline or reference an external .tsj file.
        // We support both: inline has an "image" key; external needs loading.
        if (ts.contains("image")) {
            tileset.imagePath = ResolveRelative(mapDir, ts["image"].get<std::string>());
        } else if (ts.contains("source")) {
            // External tileset — load the .tsj and pull image path from it
            const std::string tsjPath = ResolveRelative(mapDir, ts["source"].get<std::string>());
            std::ifstream tsjf(tsjPath);
            if (!tsjf.is_open())
                throw std::runtime_error("MapLoader: cannot open external tileset " + tsjPath);
            json tsj;
            tsjf >> tsj;
            tileset.tileW   = tsj.value("tilewidth",  tileset.tileW);
            tileset.tileH   = tsj.value("tileheight", tileset.tileH);
            tileset.columns = tsj.value("columns", tileset.columns);
            if (tsj.contains("image"))
                tileset.imagePath = ResolveRelative(
                    fs::path(tsjPath).parent_path(),
                    tsj["image"].get<std::string>());
        }

        if (!tileset.imagePath.empty())
            map.tilesets.push_back(std::move(tileset));
    }

    // ---- Layers ---------------------------------------------------------
    for (const auto& layer : j.value("layers", json::array())) {
        const std::string type = layer.value("type", "");

        if (type == "tilelayer") {
            TileLayer tl;
            tl.name   = layer.value("name",   "");
            tl.width  = layer.value("width",  0);
            tl.height = layer.value("height", 0);
            if (layer.contains("data"))
                tl.data = layer["data"].get<std::vector<int>>();
            map.tileLayers.push_back(std::move(tl));
        }
        else if (type == "objectgroup") {
            ObjectLayer ol;
            ol.name = layer.value("name", "");
            for (const auto& obj : layer.value("objects", json::array())) {
                MapObject mo;
                mo.id   = obj.value("id",   0);
                mo.name = obj.value("name", "");
                // Tiled ≥1.9 uses "class"; older versions use "type"
                mo.type = obj.contains("class") ? obj["class"].get<std::string>()
                                                 : obj.value("type", "");
                mo.x = static_cast<float>(obj.value("x", 0.0));
                mo.y = static_cast<float>(obj.value("y", 0.0));
                mo.w = static_cast<float>(obj.value("width",  0.0));
                mo.h = static_cast<float>(obj.value("height", 0.0));
                // Custom properties
                for (const auto& prop : obj.value("properties", json::array())) {
                    const std::string name  = prop.value("name",  "");
                    const std::string value = prop.value("value", json{}).dump();
                    if (!name.empty()) mo.properties[name] = value;
                }
                ol.objects.push_back(std::move(mo));
            }
            map.objectLayers.push_back(std::move(ol));
        }
        // Ignore group / imagelayer for now
    }

    return map;
}
