#include "AlphaScenery.h"
#include <bit>
#include <cmath>

namespace alphascenery {
bool Fade::afterWater() const {
    return squared > full * full;
}

float Fade::opacity() const {
    if (!afterWater())
        return 1;
    const float distance = (float)std::sqrt((double)squared);
    return 1.f - (distance - full) / (end - full);
}

Fade sample(const Placement &placement, const Point &camera, float renderRadius, float fogRange) {
    std::array<float, 3> delta;
    for (int axis = 0; axis < 3; ++axis) {
        const auto bits = (uint64_t)placement.point[axis] - (uint64_t)camera[axis];
        delta[axis] = (float)std::bit_cast<int64_t>(bits) * (1.f / 65536.f);
    }
    const float squared = (delta[0] * delta[0] + delta[1] * delta[1]) + delta[2] * delta[2];
    const float end = placement.kind == Kind::Model ? renderRadius * .2f : (fogRange + 22.4f) * .3f;
    return {squared, end, end * .9f};
}
} // namespace alphascenery
