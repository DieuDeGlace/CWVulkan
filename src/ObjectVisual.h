#pragma once
// Static-object geometry from docs/specs/worldgen-parity-gaps.md §4.
#include <cstdint>
#include <glm/glm.hpp>

namespace objectvisual {
struct State {
    int type = -1;
    uint8_t turn = 0, closed = 1;
    float scale = 1, elapsedMs = 1000;
};
float visualFactor(int type);
float voxelScale(int type, float objectScale, int voxelWidth);
float openness(const State &state);
// The input mesh is centered in voxel x/y, with its bottom at voxel z=0.
glm::mat4 matrix(const State &state, glm::vec3 position, glm::vec3 voxelDimensions);
} // namespace objectvisual
