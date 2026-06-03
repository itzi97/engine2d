#pragma once
#include <cstdint>

// An entity is just a numeric ID. Generation and reset are owned by World,
// so multiple World instances never share or clobber each other's counters.
using EntityId = uint32_t;
