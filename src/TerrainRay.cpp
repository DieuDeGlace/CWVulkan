#include "TerrainRay.h"
#include "GameMath.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace terrainray {
namespace {
int32_t signedBits(uint32_t bits) {
    int32_t value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
int32_t increment(int32_t value, bool positive) {
    return signedBits(uint32_t(value) + (positive ? 1u : uint32_t(-1)));
}
int32_t zoneCoordinate(int32_t block) {
    return block / 64 - (block < 0 && block % 64 != 0 ? 1 : 0);
}
} // namespace

int32_t blockCoordinate(int64_t fixed) {
    const int64_t quotient = fixed / 65536;
    int32_t narrowed = signedBits(uint32_t(quotient));
    if (fixed < 0 && int64_t(narrowed) * 65536 != fixed)
        narrowed = increment(narrowed, false);
    return narrowed;
}

float trace(const World &world, const std::array<int64_t, 3> &origin, const std::array<float, 3> &direction, float maximum,
            Material material) {
    std::array<int32_t, 3> cell{blockCoordinate(origin[0]), blockCoordinate(origin[1]), blockCoordinate(origin[2])};
    int32_t zoneX = zoneCoordinate(cell[0]), zoneY = zoneCoordinate(cell[1]);
    bool available = world.hasZone(zoneX, zoneY);
    float traveled = 0;
    int crossings = 0;
    for (;;) {
        if (!available)
            break;
        const auto block = world.block(cell[0], cell[1], cell[2]);
        if ((block.type & 0xfd) || (material == Material::HonorFlag40 && (block.flags & 0x40)))
            break;
        // The final cell is still read before the501-crossing budget rejects an advance.
        if (crossings > 500)
            break;
        ++crossings;
        float best = 10.0f;
        int axis = 0;
        for (int candidateAxis = 0; candidateAxis < 3; ++candidateAxis) {
            const float component = direction[candidateAxis];
            if (component * component < 0.000001f)
                continue;
            const float traveledBlocks = component * traveled;
            const int64_t point = gamemath::addFixed(origin[candidateAxis], int64_t(traveledBlocks * 65536.0f));
            const int32_t boundaryCell = component > 0 ? increment(cell[candidateAxis], true) : cell[candidateAxis];
            const int64_t boundary = int64_t(boundaryCell) * 65536;
            const float difference = float(gamemath::subtractFixed(boundary, point));
            const int64_t quantized = int64_t(difference / component);
            const float distance = float(quantized) * (1.0f / 65536.0f);
            if (distance < best) {
                best = distance;
                axis = candidateAxis;
            }
        }
        cell[axis] = increment(cell[axis], direction[axis] > 0);
        const int32_t nextX = zoneCoordinate(cell[0]), nextY = zoneCoordinate(cell[1]);
        if (nextX != zoneX || nextY != zoneY) {
            zoneX = nextX;
            zoneY = nextY;
            available = world.hasZone(zoneX, zoneY);
        }
        // Resolve the crossed zone first, even when this increment reaches the range cap.
        traveled += best;
        if (traveled >= maximum)
            break;
    }
    return std::min(maximum, traveled);
}
bool visible(const World &world, const std::array<int64_t, 3> &start, const std::array<int64_t, 3> &end, float maximum, Material material) {
    // The range test and direction use separate double/float conversions. Reusing
    // a rounded float norm here would admit points just outside the native limit.
    std::array<double, 3> range{};
    for (int axis = 0; axis < 3; ++axis)
        range[axis] = double(gamemath::subtractFixed(start[axis], end[axis])) * (1.0 / 65536.0);
    if (std::abs(range[0]) > double(maximum) || std::abs(range[1]) > double(maximum))
        return false;
    const float maximumSquared = maximum * maximum;
    const double rangeSquared = (range[1] * range[1] + range[0] * range[0]) + range[2] * range[2];
    if (rangeSquared > double(maximumSquared))
        return false;
    std::array<float, 3> direction{};
    for (int axis = 0; axis < 3; ++axis)
        direction[axis] = gamemath::fixedBlocksBetween(end[axis], start[axis]);
    const float lengthSquared = (direction[0] * direction[0] + direction[1] * direction[1]) + direction[2] * direction[2];
    if (lengthSquared < .0001f)
        return true;
    const float length = std::sqrt(lengthSquared);
    for (float &component : direction)
        component /= length;
    return trace(world, start, direction, length, material) >= length - .01f;
}
} // namespace terrainray
