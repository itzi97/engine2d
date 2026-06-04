#pragma once
#include "ecs/Entity.hpp"
#include <vector>

// Extend the type-erased base to expose the entity list so
// World::GetAllEntities() can collect IDs without knowing T.
struct IComponentStorageBase {
  virtual ~IComponentStorageBase() = default;
  virtual void Erase(EntityId entity) = 0;
  virtual void Clear() = 0;
  virtual std::vector<EntityId> Entities() const = 0;
};
