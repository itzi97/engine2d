#pragma once
#include "map/TiledMap.hpp"
#include "ecs/Entity.hpp"
#include <vector>

class World;
class TextureManager;

// Spawns ECS entities from a loaded TiledMap.
// Tile layers   → one entity per non-zero GID
//                 (TransformComponent + SpriteComponent with atlas srcRect)
// Object layers → one entity per object
//                 (TransformComponent + TagComponent)
//
// Returns all spawned entity IDs so the caller (Lua binding) can
// hand object-layer entities back to scripts.
struct SpawnResult {
    std::vector<EntityId> tileEntities;   // all tile-layer entities
    std::vector<EntityId> objectEntities; // all object-layer entities, same order as map objects
    // Parallel to objectEntities — the MapObject data for each
    std::vector<MapObject> objects;
};

struct MapSystem {
    static SpawnResult Spawn(const TiledMap& map,
                             World&          world,
                             TextureManager& textures);
};
