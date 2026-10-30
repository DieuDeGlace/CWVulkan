#pragma once
// Steam's generated light shafts: worldgen-parity-gaps.md §2.1. No model asset.
#include "Mesher.h"
#include <array>
#include <optional>

namespace lightbeam {
struct Placement {
    std::array<int64_t, 3> position{}; // fixed units, in the engine's render space
    float height = 0, width = 0;
};
struct Quad {
    std::array<glm::vec3, 4> corners;
    uint8_t alpha = 0;
    double distanceSquared = 0;
};
// angleDegrees is the camera's horizontal width direction, not the model's width.
// daylight is the process CRT setting, not the game clock's day/night factor.
std::optional<Quad> sample(const Placement &, glm::vec3 eye, float angleDegrees, float range, int daylight);
Mesh mesh(); // four vertex identifiers; the shader reads the sampled corners
} // namespace lightbeam
