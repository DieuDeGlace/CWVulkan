#include "MapControls.h"
#include "GameMath.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace mapcontrol {
namespace {
int64_t signedWord(uint64_t value) {
    return std::bit_cast<int64_t>(value);
}
int64_t displace(int64_t value, int64_t destination, int64_t gain) {
    const uint64_t difference = uint64_t(destination) - uint64_t(value);
    const int64_t product = signedWord(difference * uint64_t(gain));
    return signedWord(uint64_t(value) + uint64_t(product / 65536));
}
Position pan(Position target, glm::vec3 input, float yawDegrees) {
    const auto rotation = glm::rotate(glm::mat4(1), glm::radians(yawDegrees), glm::vec3(0, 0, 1));
    const auto moved = glm::mat3(rotation) * input;
    for (size_t axis = 0; axis < target.size(); ++axis)
        target[axis] = signedWord(uint64_t(target[axis]) + uint64_t(int64_t(moved[(int)axis] * 65536.f)));
    return target;
}
} // namespace

Position fromBlocks(glm::dvec3 value) {
    return {int64_t(value.x * 65536.0), int64_t(value.y * 65536.0), int64_t(value.z * 65536.0)};
}
glm::dvec3 blocks(const Position &value) {
    return {double(value[0]) / 65536.0, double(value[1]) / 65536.0, double(value[2]) / 65536.0};
}
Position terrainHeight(Position value, const std::function<float(int32_t, int32_t)> &height) {
    const int32_t x = (int32_t)(uint32_t)(value[0] / 65536), y = (int32_t)(uint32_t)(value[1] / 65536);
    value[2] = int64_t(height(x, y) * 65536.f);
    return value;
}
Position ease(const Position &drawn, const Position &target, double frameMilliseconds) {
    const int64_t weight = int64_t(gamemath::blendFactor(std::min(frameMilliseconds, 1000.0), .01f) * 65536.f);
    Position result;
    for (size_t axis = 0; axis < result.size(); ++axis)
        result[axis] = displace(drawn[axis], target[axis], weight);
    return result;
}
Position wheelCenter(Position target, const Position &cursor, bool zoomedIn) {
    for (size_t axis = 0; axis < 2; ++axis)
        target[axis] = displace(target[axis], cursor[axis], zoomedIn ? 32768 : -65536);
    return target;
}
Position mousePan(Position target, glm::vec2 guiDelta, int layer, float displayedYawDegrees) {
    const float factor = std::pow(2.f, float(layer));
    return pan(target, {guiDelta.x * factor, -guiDelta.y * factor, 0}, displayedYawDegrees);
}
Position padPan(Position target, glm::vec2 filteredStick, int layer, float displayedYawDegrees, double frameMilliseconds) {
    const float factor = (std::pow(2.f, float(layer)) * .5f) * float(std::min(frameMilliseconds, 1000.0));
    return pan(target, {-filteredStick.x * factor, -filteredStick.y * factor, 0}, displayedYawDegrees);
}
int wheelLayer(int oldLayer, int signedDelta) {
    const int direction = (signedDelta > 0) - (signedDelta < 0);
    return std::clamp(oldLayer - direction, 0, MAX_WHEEL_LAYER);
}
int mappedZoom(int oldLayer, bool inEdge, bool outEdge) {
    if (inEdge)
        oldLayer = std::clamp(oldLayer - 1, 0, MAX_LAYER);
    if (outEdge)
        oldLayer = std::clamp(oldLayer + 1, 0, MAX_LAYER);
    return oldLayer;
}
bool movementCloses(bool fullMap, bool lastDevicePad, bool digitalMovement, bool chatOpen, glm::vec2 filteredStick) {
    constexpr float closeThreshold = 0.010000000707805157f; // Native word0x3c23d70b.
    return !(fullMap && lastDevicePad) && ((digitalMovement && !chatOpen) || glm::dot(filteredStick, filteredStick) > closeThreshold);
}
} // namespace mapcontrol
