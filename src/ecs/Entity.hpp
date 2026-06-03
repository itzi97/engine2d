#pragma once
#include <cstdint>

using EntityId = uint32_t;

struct Entity {
  [[nodiscard]] static EntityId Create() { return s_next++; }

  // Reset the entity counter. Only call from World::ClearAll(),
  // after all storages have been wiped, so recycled IDs are safe.
  static void Reset() { s_next = 0; }

private:
  static inline EntityId s_next = 0;
};
