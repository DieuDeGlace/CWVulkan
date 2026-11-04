#include "SteamZoneDemand.h"
#include "GameMath.h"
#include <bit>

namespace zonedemand {
namespace {
int32_t floorZone(int32_t block) {
    const int64_t value = block;
    return int32_t(value >= 0 ? value / 64 : (value - 63) / 64);
}
int32_t signedBits(uint32_t bits) { return std::bit_cast<int32_t>(bits); }
}
std::optional<BlockPoint> eagleAnchor(uint8_t ability, const BlockPoint &destination) {
    return ability == 0x9c ? std::optional<BlockPoint>(destination) : std::nullopt;
}
BlockPoint preparationCenter(const std::array<int64_t, 3> &physical, const std::optional<BlockPoint> &anchor) {
    return anchor ? BlockPoint{floorZone((*anchor)[0]), floorZone((*anchor)[1])}
                  : BlockPoint{gamemath::floorFixedZone(physical[0]), gamemath::floorFixedZone(physical[1])};
}
ClientWindow clientWindow(const std::array<int64_t, 3> &physical, int32_t renderDistance,
                          const std::optional<BlockPoint> &anchor) {
    int32_t radius = signedBits(uint32_t(renderDistance) * 1920u) / 100;
    if (radius <= 64)
        radius = 64;
    ClientWindow result;
    const uint32_t zones = uint32_t(radius / 64 + 1);
    result.radiusSquared = signedBits(zones * zones);
    for (int axis = 0; axis < 2; ++axis) {
        // Source demand truncates and narrows; it has no mesh/camera negative correction.
        const int32_t block = anchor ? (*anchor)[axis] : signedBits(uint32_t(physical[axis] / 65536));
        result.center[axis] = floorZone(block);
        result.minimum[axis] = floorZone(signedBits(uint32_t(block) - uint32_t(radius)));
        result.maximum[axis] = floorZone(signedBits(uint32_t(block) + uint32_t(radius)));
    }
    return result;
}
bool withinRetention(int64_t dx, int64_t dy, int32_t sceneRadius) {
    const double x = double(dx), y = double(dy), radius = double(int64_t(sceneRadius) + 1);
    return x * x + y * y <= radius * radius;
}
bool ClientWindow::contains(int64_t x, int64_t y) const {
    if (x < minimum[0] || x > maximum[0] || y < minimum[1] || y > maximum[1])
        return false;
    const double dx = double(x - center[0]), dy = double(y - center[1]);
    return dx * dx + dy * dy <= double(radiusSquared);
}
} // namespace zonedemand
