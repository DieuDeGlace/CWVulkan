#pragma once
// Ordinary Steam zone-model visibility and queue selection (parity audit §9.5–9.6).
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

namespace steamscenery {
struct Bounds {
    std::array<int64_t, 3> center{}; // fixed render XY/game Z, raised by half the scaled height
    float radius = 0;
    bool water = false;
};
Bounds bounds(std::array<int64_t, 3> position, glm::vec3 voxels, float size, uint32_t modelFlags);
// Early native gates divide each signed squared fixed delta before summing.
bool earlyWithin(const std::array<int64_t, 3> &position, glm::vec3 eye, float range, bool horizontal = false);
float distanceSquared(const std::array<int64_t, 3> &position, glm::vec3 eye);
float fade(float distanceSquared, float range);
enum class Queue { Hidden, OrdinaryOpaque, WaterOpaque, WaterFaded, OrdinaryFaded };
Queue queue(const Bounds &, float opacity);
} // namespace steamscenery
