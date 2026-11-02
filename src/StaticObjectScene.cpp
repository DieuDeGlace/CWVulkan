#include "StaticObjectScene.h"
#include "GameMath.h"
#include "worldgen/StructureTables.h"
#include <algorithm>
#include <bit>

namespace staticobjectscene {
scenevisibility::Sphere sphere(int32_t type, float scale, Point position) {
    const auto size = worldgen::objectSize(type);
    const float x = size.x * scale, y = size.y * scale, z = size.z * scale;
    const int64_t halfHeight = worldgen::trunc64(double(z) * .5 * 65536.0);
    position[2] = gamemath::addFixed(position[2], halfHeight);
    return {position, std::max({x, y, z}) + 1.f};
}

std::array<int32_t, 2> playerZone(const Point &physical) {
    std::array<int32_t, 2> result;
    for (int axis = 0; axis < 2; ++axis) {
        int32_t block = std::bit_cast<int32_t>(uint32_t(physical[axis] / 65536));
        if (physical[axis] < 0 && int64_t(block) * 65536 != physical[axis])
            block = std::bit_cast<int32_t>(uint32_t(block) - 1u);
        result[axis] = block >> 6;
    }
    return result;
}

bool inPlayerSquare(const Point &physical, int32_t zx, int32_t zy) {
    const auto center = playerZone(physical);
    const int64_t dx = int64_t(zx) - center[0], dy = int64_t(zy) - center[1];
    return dx >= -3 && dx <= 3 && dy >= -3 && dy <= 3;
}
} // namespace staticobjectscene
