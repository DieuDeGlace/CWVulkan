#pragma once
// Original-item value assignment (0x9cd0), from steam-selected-vehicle.md and
// steam-use-creature-collection.md section 5.2. Padding retains its prior bytes.
#include <cstdint>
#include <cstring>

namespace itemvalue {
inline void assign(uint8_t *destination, const uint8_t *source) {
    destination[0] = source[0];
    std::memmove(destination + 4, source + 4, 0x11);
    std::memmove(destination + 0x18, source + 0x18, 0x86);
}
} // namespace itemvalue
