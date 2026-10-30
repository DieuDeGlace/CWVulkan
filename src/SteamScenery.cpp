#include "SteamScenery.h"
#include <algorithm>
#include <cmath>

namespace steamscenery {
Bounds bounds(std::array<int64_t, 3> position, glm::vec3 voxels, float size, uint32_t flags) {
    position[2] += (int64_t)(voxels.z * 0.5f * size * 65536.0f);
    return {position, std::max(voxels.x, voxels.z) * size, (flags & 8) != 0};
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
    if (!(range > 0))
        return 0;
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
