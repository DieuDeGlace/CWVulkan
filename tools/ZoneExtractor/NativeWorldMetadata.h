#pragma once
// Original fields/coordinate ABI: steam-world-metadata-and-coordinate-abi.md.
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace captureabi {
// Native modular arithmetic: worldgen-terrain-zone.md section 2.
inline uint32_t zoneGenerationSeed(int32_t x, int32_t y, int32_t derived) {
    return uint32_t(y) * 1024u + uint32_t(derived) + uint32_t(x);
}
struct CoordinatePair {
    int32_t x = 0, y = 0;
};
static_assert(sizeof(CoordinatePair) == 8 && offsetof(CoordinatePair, y) == 4);
class WorldMetadata {
    uint8_t *world;
    template <class T> T read(size_t offset) const {
        T value;
        std::memcpy(&value, world + offset, sizeof value);
        return value;
    }

  public:
    // Requires a live, initialized World and caller-held applicable protection.
    explicit WorldMetadata(void *record) : world(static_cast<uint8_t *>(record)) {}
    uint32_t seedBits() const { return read<uint32_t>(0x220); }
    int32_t seedLabel() const { return read<int32_t>(0x220); } // Project signed path/cache convention.
    int32_t derivedGenerationValue() const { return read<int32_t>(0x244); }
    int32_t sandX() const { return read<int32_t>(0x330); }
    int32_t sandY() const { return read<int32_t>(0x334); }
    int32_t day() const { return read<int32_t>(8); }
    float time() const { return read<float>(0x0c); }
    void setTime(float value) { std::memcpy(world + 0x0c, &value, sizeof value); }
};
} // namespace captureabi
