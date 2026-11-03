// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#pragma once
// Alpha's voxel trees (docs/specs/alpha-worldgen-trees.md): the builder that writes a tree's trunk,
// roots, branches and leaf crowns as blocks into the zone being generated, continuing the zone's
// random stream. Section numbers point at the trees spec.
//
// Palms stamp the owner's leaf models using the independently specified mode0 placer.
#include "alphagen/Terrain.h"

namespace alphagen {

constexpr uint8_t WOOD = 7, LEAVES = 8; // block types (§2.1)

class TreeBuilder {
  public:
    TreeBuilder(const Terrain &terrain, Zone &zone, Random &random) : terrain(terrain), zone(zone), random(random) {}

    // §4: a tree of kind k (0..6) at column (x, y) whose first cell above the ground is z,
    // with size s and height t.
    void build(int32_t x, int32_t y, int32_t z, int32_t s, int32_t t, int32_t k);

    // §3.1: how much moss tints a cell (0..1).
    float moss(int32_t i, int32_t j, int32_t k) const;

  private:
    using Colour = glm::vec3; // 0..255, floats
    bool dry(int32_t x, int32_t y, int32_t z) const;
    void write(int32_t x, int32_t y, int32_t z, Colour c, uint8_t typeByte);
    Colour mossy(Colour c, int32_t i, int32_t j, int32_t k) const;
    void ball(int32_t cx, int32_t cy, int32_t cz, float r, Colour c, uint8_t typeByte, bool moss);
    void disc(int32_t cx, int32_t cy, int32_t z, float r, Colour c, uint8_t typeByte);
    void palmLeaf(const CubModel &, int32_t x, int32_t y, int32_t z, int rotation);
    void blob(int32_t x, int32_t y, int32_t z, float rH, float rV, Colour lower, Colour upper, int32_t kind);

    const Terrain &terrain;
    Zone &zone;
    Random &random;
};

} // namespace alphagen
