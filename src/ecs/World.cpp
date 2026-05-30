#include "ecs/World.hpp"

void World::Update(float deltaTime)
{
    for (auto& [typeIdx, storage] : m_storages)
        for (auto& [entity, comp] : storage)
            comp->Update(deltaTime);
}

void World::Render()
{
    for (auto& [typeIdx, storage] : m_storages)
        for (auto& [entity, comp] : storage)
            comp->Render();
}
