// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#include "alphagen/Zone.h"
#include <algorithm>

namespace alphagen {

namespace {
constexpr Block WATER_BLOCK{255, 255, 255, 0x82};
constexpr Block AIR_BLOCK{255, 255, 255, AIR};
} // namespace

uint64_t lightKey(int32_t x, int32_t y, int32_t z) {
    // Columns are within one zone of the world (x, y < 2^24); z fits 16 bits around sea level.
    return ((uint64_t)(uint32_t)x << 40) | ((uint64_t)(uint32_t)y << 16) | (uint16_t)(int16_t)z;
}

Block Zone::read(int32_t x, int32_t y, int32_t z) const {
    const Column &c = column(x, y);
    if (z < c.bottom)
        return IMPLICIT_SOLID;
    if (z >= c.top())
        return z > 0 ? AIR_BLOCK : WATER_BLOCK;
    const Block b = c.blocks[z - c.bottom];
    if (b.type() == AIR && z <= 0 && !(b.typeByte & DRY_AIR))
        return WATER_BLOCK;
    return b;
}

void Zone::place(int32_t x, int32_t y, int32_t z, Block b, const std::function<Block(int32_t)> &gapFill) {
    Column &c = column(x, y);
    if (b.typeByte == AIR && z >= c.top())
        return; // air above the top is already air
    if (z <= 0 && (b.typeByte & 0x5f) == 0)
        b = WATER_BLOCK;
    if (c.blocks.empty()) {
        c.bottom = z;
        c.blocks.push_back(b);
        return;
    }
    if (z < c.bottom) {
        // Below the old bottom: the new block, then the gap up to the old bottom filled.
        std::vector<Block> below;
        below.push_back(b);
        for (int32_t gz = z + 1; gz < c.bottom; gz++)
            below.push_back(gapFill ? gapFill(gz) : IMPLICIT_SOLID);
        c.blocks.insert(c.blocks.begin(), below.begin(), below.end());
        c.bottom = z;
        return;
    }
    if (z >= c.top())
        c.blocks.resize(z - c.bottom + 1, AIR_BLOCK);
    Block &slot = c.blocks[z - c.bottom];
    if (slot.typeByte & KEEP) { // terrain §1.3: a "keep" block only takes non-air, and stays kept
        if (b.type() != AIR) {
            slot = b;
            slot.typeByte |= KEEP;
        }
        return;
    }
    slot = b;
}

uint8_t Zone::light(int32_t x, int32_t y, int32_t z) const {
    const auto key = lightKey(x, y, z);
    const auto it = std::lower_bound(shadedLight.begin(), shadedLight.end(), std::make_pair(key, (uint8_t)0));
    return it != shadedLight.end() && it->first == key ? it->second : 255;
}

} // namespace alphagen
