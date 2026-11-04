#pragma once
// Original-source-only layout: steam-creature-appearance-record.md.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace captureabi {
using AppearanceTriple = std::array<float, 3>;
class CreatureAppearance {
    const uint8_t *creature;
    template <class T> T read(size_t offset) const {
        T value;
        std::memcpy(&value, creature + offset, sizeof value);
        return value;
    }

  public:
    // Borrowed scalar state: caller retains Creature lifetime and applicable
    // protection. No model ownership, defaults reconstruction or atomic snapshot.
    explicit CreatureAppearance(const void *record) : creature(static_cast<const uint8_t *>(record)) {}
    std::array<uint8_t, 3> tint() const { return read<std::array<uint8_t, 3>>(0x7a); }
    uint32_t flags() const { return read<uint32_t>(0x80); }
    AppearanceTriple extents() const { return read<AppearanceTriple>(0x84); }
    std::array<int16_t, 8> modelIds() const { return read<std::array<int16_t, 8>>(0x90); }
    std::array<float, 8> modelScalars() const { return read<std::array<float, 8>>(0xa0); }
    float scalarC0() const { return read<float>(0xc0); } // Complete purpose unresolved.
    std::array<float, 7> poseControls() const { return read<std::array<float, 7>>(0xc4); }
    std::array<AppearanceTriple, 6> placementInputs() const { return read<std::array<AppearanceTriple, 6>>(0xe0); }
};
static_assert(sizeof(AppearanceTriple) == 12);
} // namespace captureabi
