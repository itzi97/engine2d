#include "map/MapSystem.hpp"

#include "ecs/World.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "rendering/TextureManager.hpp"

#include <SDL3/SDL.h>
#include <cmath>

// Find the tileset that owns a given GID.
// Tilesets are sorted by firstGid ascending (Tiled guarantees this).
static const Tileset* FindTileset(const TiledMap& map, int gid) {
    const Tileset* result = nullptr;
    for (const auto& ts : map.tilesets) {
        if (ts.firstGid <= gid)
            result = &ts;
        else
            break;
    }
    return result;
}

// Compute the source rect on the tileset image for a given GID.
// Always uses the tileset's own tileW/tileH for the source sample size.
static SDL_FRect GidToSrcRect(const Tileset& ts, int gid) {
    const int localId = gid - ts.firstGid;           // 0-based index in this tileset
    const int col     = localId % ts.columns;
    const int row     = localId / ts.columns;
    return SDL_FRect{
        static_cast<float>(col * ts.tileW),
        static_cast<float>(row * ts.tileH),
        static_cast<float>(ts.tileW),
        static_cast<float>(ts.tileH)
    };
}

SpawnResult MapSystem::Spawn(const TiledMap& map,
                             World&          world,
                             TextureManager& textures) {
    SpawnResult result;

    // ---- Tile layers --------------------------------------------------------
    for (const auto& layer : map.tileLayers) {
        for (int row = 0; row < layer.height; ++row) {
            for (int col = 0; col < layer.width; ++col) {
                const int idx = row * layer.width + col;
                if (idx >= static_cast<int>(layer.data.size())) continue;
                const int gid = layer.data[static_cast<std::size_t>(idx)];
                if (gid == 0) continue;  // empty cell

                const Tileset* ts = FindTileset(map, gid);
                if (!ts) continue;

                const EntityId e = world.CreateEntity();

                // Position: top-left of this tile in world space
                // Spacing uses map.tileW/H so tiles are placed on the map grid.
                auto& t = world.AddComponent<TransformComponent>(e);
                t.position = {
                    static_cast<float>(col * map.tileW),
                    static_cast<float>(row * map.tileH)
                };
                // Render size uses map.tileW/H (the desired display size),
                // NOT ts->tileW/H (the source art size). This allows the art
                // to be scaled up (e.g. 16px source rendered at 32px) without
                // gaps or overlaps between tiles.
                t.size = {
                    static_cast<float>(map.tileW),
                    static_cast<float>(map.tileH)
                };

                auto& s  = world.AddComponent<SpriteComponent>(e);
                s.layer  = 0;  // tile layers always behind game entities
                s.texture = textures.Load(ts->imagePath);
                s.srcRect = GidToSrcRect(*ts, gid);

                result.tileEntities.push_back(e);
            }
        }
    }

    // ---- Object layers ------------------------------------------------------
    for (const auto& layer : map.objectLayers) {
        for (const auto& obj : layer.objects) {
            const EntityId e = world.CreateEntity();

            auto& t = world.AddComponent<TransformComponent>(e);
            t.position = {obj.x, obj.y};
            t.size     = {obj.w, obj.h};

            // Tag with the object type ("player", "block", "spawn", ...)
            // Falls back to the object name if type is empty.
            auto& tag = world.AddComponent<TagComponent>(e);
            tag.tag   = obj.type.empty() ? obj.name : obj.type;

            result.objectEntities.push_back(e);
            result.objects.push_back(obj);
        }
    }

    return result;
}
