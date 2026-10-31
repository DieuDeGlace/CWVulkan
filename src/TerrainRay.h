#pragma once
// Independent steam-positive-terrain-ray.md and gameplay-camera §§6.3–6.4.
#include <array>
#include <cstdint>
#include <functional>

namespace terrainray {
struct Cell {
    uint8_t type = 1, flags = 0;
};
struct World {
    // Existing loaded zones only. Valid zone headers agree with their lookup keys.
    std::function<bool(int32_t zoneX, int32_t zoneY)> hasZone;
    std::function<Cell(int32_t x, int32_t y, int32_t z)> block;
};
enum class Material { TypeOnly, HonorFlag40 };
// Native0xfd80 conversion, including narrowing before the negative exactness test.
int32_t blockCoordinate(int64_t fixed);
// Finite positive-limit contract. Direction is used as stored, without normalization.
float trace(const World &world, const std::array<int64_t, 3> &origin, const std::array<float, 3> &direction, float maximum,
            Material material);
// Fixed-coordinate visibility wrapper: double range rejection, float direction,
// then a full-length ray compared with the separately rounded length minus .01.
bool visible(const World &world, const std::array<int64_t, 3> &start, const std::array<int64_t, 3> &end, float maximum, Material material);
} // namespace terrainray
