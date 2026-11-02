#pragma once
// Ordinary ground-item geometry and material, from steam-ground-item-scene-rendering.md.
#include "NativeSaves.h"
#include "SceneVisibility.h"

namespace groundscene {
struct Record {
    nativesaves::Item item{};
    std::array<int64_t, 3> position{};
    float angle = 0, delayMs = 0;
    int32_t timerMs = 0, quantity = 0;
    int64_t owner = 0;
};
Record decode(const nativesaves::Record &);
struct Input {
    std::array<int64_t, 3> position{};
    glm::ivec3 dimensions{0};
    float angle = 0, delayMs = 0, worldMs = 0;
    int originalCategory = 0, light = 255;
    bool selected = false;
};
struct Prepared {
    std::array<int64_t, 3> position{}; // hopped anchor; sphere and fade retain the original anchor
    glm::mat4 model{1};                // local corner-origin geometry, before engine mesh centering
    scenevisibility::Sphere sphere;
    glm::vec4 material{1};
    float opacityScale = 1;
};
float itemScale(const nativesaves::Item &);
scenevisibility::Sphere bounds(const Input &, const nativesaves::Item &adapted);
Prepared prepare(const Input &, const nativesaves::Item &adapted);
} // namespace groundscene
