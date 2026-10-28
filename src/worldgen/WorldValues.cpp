// A world's values from its seed, written from docs/specs/worldgen-terrain-zone.md §1 and §2.
#include "worldgen/WorldValues.h"

#include "worldgen/Noise.h"

namespace worldgen {
namespace {

// The runtime library's std::shuffle (0x298d40) with rand() as its source: for each position
// i from 1 up, a draw in 0..i (re-drawn while it falls in the uneven top of rand()'s range),
// swapped into place.
void shuffle(std::array<int32_t, 16> &list, CrtRandom &rng) {
    for (uint32_t i = 1; i < list.size(); i++) {
        const uint32_t n = i + 1;
        uint32_t r;
        do
            r = (uint32_t)rng.next();
        while (r / n >= 32767u / n && 32767u % n != n - 1);
        std::swap(list[i], list[r % n]);
    }
}

} // namespace

int32_t WorldValues::value(int offset) const { return numbers[(size_t)(offset - FIRST) / 4]; }

WorldValues worldValues(int32_t seed) {
    WorldValues w;
    w.seed = seed;
    CrtRandom rng;
    rng.seed((uint32_t)seed);
    for (int i = 0; i < 64; i++) {
        std::array<int32_t, 16> layout = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0, 0, 0, 0, 0, 0};
        shuffle(layout, rng);
        w.townLayouts.push_back(layout);
    }
    // 77 numbers, in the order the game draws them.
    auto draw = [&](int offset) { w.numbers[(size_t)(offset - WorldValues::FIRST) / 4] = rng.next() % 100000; };
    draw(WorldValues::ZONE_SEED);
    for (int o = 0x224; o <= 0x230; o += 4)
        draw(o);
    for (int o = 0x248; o <= 0x344; o += 4)
        draw(o);
    for (int o = 0x234; o <= 0x240; o += 4)
        draw(o);
    for (int o = 0x348; o <= 0x354; o += 4)
        draw(o);
    return w;
}

uint32_t zoneRandomSeed(const WorldValues &world, int32_t zx, int32_t zy) {
    return (uint32_t)zy * 1024u + (uint32_t)world.zoneSeed() + (uint32_t)zx;
}

} // namespace worldgen
