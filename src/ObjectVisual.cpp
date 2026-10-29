#include "ObjectVisual.h"
#include "worldgen/StructureTables.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace objectvisual {
float visualFactor(int type) {
    switch (type) {
    case 0x41: return 1.8f;
    case 0x42: return 2;
    case 0x54: case 0x55: case 0x56: case 0x57: return 1.1f;
    case 0x58: return 1.25f;
    case 0x5a: return 3;
    case 0x5b: case 0x5d: return 1.3f;
    default: return 1;
    }
}

float voxelScale(int type, float objectScale, int voxelWidth) {
    if (voxelWidth <= 0)
        return 0;
    const auto size = worldgen::objectSize(type);
    return std::max(size.x * objectScale, size.y * objectScale) / (float)voxelWidth * visualFactor(type);
}

float openness(const State &state) {
    if (!state.closed)
        return std::min(1.0f, state.elapsedMs * 0.001f);
    const float rate = state.type == 0x12 || state.type == 0x13 ? 0.01f : 0.001f;
    return std::max(0.0f, 1.0f - state.elapsedMs * rate);
}

glm::mat4 matrix(const State &state, glm::vec3 position, glm::vec3 dimensions) {
    constexpr float RADIANS = 0.01745329238474369f;
    const float scale = voxelScale(state.type, state.scale, (int)dimensions.x);
    const float open = openness(state);
    const float height = worldgen::objectSize(state.type).z * state.scale;
    switch (state.type) {
    case 8: case 9: case 10: case 0x10: position.z += 0.8f * height * open; break;
    case 0x12: position.z -= height * open + 1.0f; break;
    case 0x13: position.z += height * open; break;
    default: break;
    }
    auto result = glm::rotate(glm::translate(glm::mat4(1), position), (float)state.turn * 90.0f * RADIANS, glm::vec3(0, 0, 1));
    result = glm::scale(result, glm::vec3(scale));
    if (state.type >= 3 && state.type <= 6) {
        const glm::vec3 pivot(-dimensions.x * 0.5f, 0, 0);
        result = glm::translate(result, pivot);
        result = glm::rotate(result, 90.0f * open * RADIANS, glm::vec3(0, 0, 1));
        result = glm::translate(result, -pivot);
    }
    return result;
}
} // namespace objectvisual
