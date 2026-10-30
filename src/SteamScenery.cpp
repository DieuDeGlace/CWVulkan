#include "SteamScenery.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace steamscenery {
Bounds bounds(std::array<int64_t, 3> position, glm::vec3 voxels, float size, uint32_t flags) {
    position[2] += (int64_t)(voxels.z * 0.5f * size * 65536.0f);
    return {position, std::max(voxels.x, voxels.z) * size, (flags & 8) != 0};
}

bool earlyWithin(const std::array<int64_t, 3> &position, glm::vec3 eye, float range, bool horizontal) {
    // Unsigned operations preserve the native machine's wrap without signed C++ overflow.
    uint64_t sum = 0;
    for (int axis = 0; axis < (horizontal ? 2 : 3); ++axis) {
        const int64_t camera = (int64_t)((double)eye[axis] * 65536);
        const uint64_t delta = (uint64_t)position[axis] - (uint64_t)camera;
        const uint64_t squareBits = delta * delta;
        int64_t square;
        std::memcpy(&square, &squareBits, sizeof square);
        sum += (uint64_t)(square / 65536);
    }
    int64_t squared;
    std::memcpy(&squared, &sum, sizeof squared);
    const float squareRange = range * range;
    const float fixedRange = squareRange * 65536.0f;
    // CVTTSS2SI's indefinite result for an invalid conversion is INT64_MIN.
    const int64_t threshold = !std::isfinite(fixedRange) || fixedRange >= 9223372036854775808.0f
                                  ? INT64_MIN : (int64_t)fixedRange;
    return squared <= threshold;
}

float distanceSquared(const std::array<int64_t, 3> &position, glm::vec3 eye) {
    glm::vec3 delta;
    for (int axis = 0; axis < 3; ++axis) {
        const int64_t camera = (int64_t)((double)eye[axis] * 65536);
        delta[axis] = (float)(position[axis] - camera) * (1.0f / 65536.0f);
    }
    return delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
}

float fade(float squared, float range) {
    if (squared <= range * 0.809999942779541f * range)
        return 1;
    return std::max(0.0f, 1.0f - (std::sqrt(squared) - range * 0.8999999761581421f) /
                                  (range * 0.10000000149011612f));
}

Queue queue(const Bounds &bounds, float opacity) {
    if (!(opacity > 0))
        return Queue::Hidden;
    if (opacity >= 1)
        return bounds.water ? Queue::WaterOpaque : Queue::OrdinaryOpaque;
    return bounds.water ? Queue::WaterFaded : Queue::OrdinaryFaded;
}
} // namespace steamscenery
