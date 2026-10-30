#pragma once
// Static-object geometry from docs/specs/worldgen-parity-gaps.md §4.
#include <cstdint>
#include <array>
#include <glm/glm.hpp>

namespace objectvisual {
struct Key {
    int type = -1, index = -1;
    std::array<int64_t, 3> position{}; // fixed world coordinates, unchanged by the render origin
};
struct State {
    int type = -1;
    uint8_t turn = 0, closed = 1;
    float scale = 1, elapsedMs = 1000;
};
// A complete static-object record, already validated by the zone-list owner.
State read(const uint8_t *record);
float visualFactor(int type);
float voxelScale(int type, float objectScale, int voxelWidth);
float openness(const State &state);
// The input mesh is centered in voxel x/y, with its bottom at voxel z=0.
glm::mat4 matrix(const State &state, glm::vec3 position, glm::vec3 voxelDimensions);
} // namespace objectvisual
