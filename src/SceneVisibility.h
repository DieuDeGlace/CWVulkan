#pragma once
// Steam mesh-cache prepass and model-zone gates, from steam-scene-lighting.md §1.1
// and worldgen-parity-gaps.md §9.7. Geometry readiness is supplied by the mesh owner.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <glm/glm.hpp>

namespace scenevisibility {
struct Column {
    int32_t base = 0, end = 0; // exclusive stored end
};
struct Bounds {
    std::array<int64_t, 3> minimum{}, maximum{}, center{};
    float radius = 0;
    int32_t lowest = 0, upper = 1, plannedBands = 1;
    std::optional<int32_t> triangleCost; // native unmerged face cost; absent for legacy meshes
    bool meshed = false;
};
using Columns = std::function<std::optional<Column>(int64_t x, int64_t y)>;
using BlockType = std::function<uint8_t(int64_t x, int64_t y, int64_t z)>;
Bounds build(int32_t zx, int32_t zy, const Columns &, const BlockType &);
struct Projection {
    std::array<int64_t, 3> offset{};
    glm::mat4 view{1}, perspective{1}; // native positive-Z, without Vulkan Y flip
};
struct Sphere { std::array<int64_t,3> centre{}; float radius = 0; };
using Planes = std::array<glm::vec4, 6>;
Planes planes(const glm::mat4 &clip);
bool sphere(const std::array<int64_t, 3> &center, float radius, glm::vec3 eye, const Planes &, float range);
bool box(const Bounds &, glm::vec3 eye, const Planes &, float radius);
bool visible(const Bounds &, glm::vec3 eye, const Planes &, float terrainRange, float modelRange);
} // namespace scenevisibility
