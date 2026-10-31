#pragma once
// Native customization projection and object-space picking, from the independent edit-view spec.
#include "CubModel.h"
#include "NativeSaves.h"
#include <glm/glm.hpp>
#include <optional>

namespace voxeledit {
struct Frame {
    glm::mat4 model{1}, projection{1};
};
std::optional<Frame> frame(glm::ivec3 allocation, glm::ivec2 viewport, const glm::mat4 &root, glm::vec2 pivot, glm::vec2 parentSize,
                           glm::vec3 euler);
struct Ray {
    glm::vec3 origin{0}, direction{0};
};
std::optional<Ray> pointerRay(const Frame &, glm::ivec2 viewport, glm::vec2 pointer);
struct Pick {
    bool hit = false;
    glm::ivec3 cell{0}, normal{0};
    float distance = 0;
    std::optional<uint8_t> attributeMaterial;
};
// Attribute equality uses coordinate bytes, including matches outside the base allocation.
Pick trace(const CubModel &, const nativesaves::Item &, const Ray &, std::optional<glm::ivec3> carried = std::nullopt);
} // namespace voxeledit
