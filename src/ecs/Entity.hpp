#pragma once

#include <cstdint>

using EntityId = std::uint32_t;

namespace Entity
{
    inline constexpr EntityId kNull = 0u;

    /// Returns a unique EntityId each call (monotonically increasing from 1).
    inline EntityId Create() noexcept
    {
        static EntityId s_next = 1u;
        return s_next++;
    }
}
