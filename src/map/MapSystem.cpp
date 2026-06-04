#include "map/MapSystem.hpp"

#include "ecs/World.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "ecs/components/SpriteComponent.hpp"
#include "ecs/components/TagComponent.hpp"
#include "rendering/TextureManager.hpp"

#include <SDL3/SDL.h>
#include <cmath>

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

// Source rect on the atlas, respecting per-tile spacing.
static SDL_FRect GidToSrcRect(const Tileset& ts, int gid) {
    const int localId = gid - ts.firstGid;
    const int col     = localId % ts.columns;
    const int row     = localId / ts.columns;
    return SDL_FRect{
        static_cast<float>(col * (ts.tileW + ts.spacing)),
        static_cast<float>(row * (ts.tileH + ts.spacing)),
        static_cast<float>(ts.tileW),
        static_cast<float>(ts.tileH)
    };
}

SpawnResult MapSystem::Spawn(const TiledMap& map,
                             World&          world,
                             TextureManager& textures) {
    SpawnResult result;

    for (const auto& layer : map.tileLayers) {
        for (int row = 0; row < layer.height; ++row) {
            for (int col = 0; col < layer.width; ++col) {
                const int idx = row * layer.width + col;
                if (idx >= static_cast<int>(layer.data.size())) continue;
                const int gid = layer.data[static_cast<std::size_t>(idx)];
                if (gid == 0) continue;

                const Tileset* ts = FindTileset(map, gid);
                if (!ts) continue;

                const EntityId e = world.CreateEntity();

                auto& t = world.AddComponent<TransformComponent>(e);
                t.position = {
                    static_cast<float>(col * map.tileW),
                    static_cast<float>(row * map.tileH)
                };
                // Render size matches source art — scaling is the game's responsibility.
                t.size = {
                    static_cast<float>(ts->tileW),
                    static_cast<float>(ts->tileH)
                };

                auto& s   = world.AddComponent<SpriteComponent>(e);
                s.layer   = 0;
                s.texture = textures.Load(ts->imagePath);
                s.srcRect = GidToSrcRect(*ts, gid);

                result.tileEntities.push_back(e);
            }
        }
    }

    for (const auto& layer : map.objectLayers) {
        for (const auto& obj : layer.objects) {
            const EntityId e = world.CreateEntity();

            auto& t = world.AddComponent<TransformComponent>(e);
            t.position = {obj.x, obj.y};
            t.size     = {obj.w, obj.h};

            auto& tag = world.AddComponent<TagComponent>(e);
            tag.tag   = obj.type.empty() ? obj.name : obj.type;

            result.objectEntities.push_back(e);
            result.objects.push_back(obj);
        }
    }

    return result;
}
