#pragma once
// The values a world draws from its seed when it is set up (docs/specs/worldgen-terrain-zone.md
// §1, the game's 0x2b5f20): 64 town layouts (part B) and 77 numbers 0..32767, among them the
// zone seed that seeds every zone's rand() and the two offsets of the sand colour's noise.
#include <array>
#include <cstdint>
#include <vector>

namespace worldgen {

struct WorldValues {
    int32_t seed = 0;
    // The 64 town layouts (world+0x4a0): each a shuffle of [2..11, 0 x 6].
    std::vector<std::array<int32_t, 16>> townLayouts;
    // The 77 numbers, by their offset in the game's world record (0x224 .. 0x357).
    int32_t value(int offset) const;
    int32_t zoneSeed() const { return value(ZONE_SEED); }
    // The sand colour's noise offsets.
    int32_t sandX() const { return value(SAND_X); }
    int32_t sandY() const { return value(SAND_Y); }

    static constexpr int ZONE_SEED = 0x244;
    static constexpr int SAND_X = 0x330;
    static constexpr int SAND_Y = 0x334;
    static constexpr int FIRST = 0x224, LAST = 0x354; // the first and last number's offsets

    std::array<int32_t, (LAST - FIRST) / 4 + 1> numbers{};
};

// The values of a world with seed `seed`.
WorldValues worldValues(int32_t seed);

// The rand() seed of zone (zx, zy) (0x2ae0c0): zy * 1024 + zone seed + zx, wrapping.
uint32_t zoneRandomSeed(const WorldValues &world, int32_t zx, int32_t zy);

} // namespace worldgen
