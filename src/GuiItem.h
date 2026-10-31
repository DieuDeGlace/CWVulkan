#pragma once
// Shared native GUI item transforms and shader inputs (steam-gui-item-icons.md).
// Sprite selection/voxel buffers belong to the owner-asset loader, not this geometry.
#include "NativeSaves.h"
#include <glm/glm.hpp>
#include <optional>
#include <vector>

namespace guiitem {
struct Placement {
    glm::vec2 screen{0};
    glm::vec3 euler{0}; // degrees, X then Y then Z
    float size = 0;
    float screenDepth = 0; // projection's homogeneous translation, not shader DepthOffset
    bool darkPass = false;
};
struct Geometry {
    glm::mat4 model{1}, projection{1}, darkView{1}; // native coordinates; renderer adapts Vulkan Y
    float depthOffset = 0;
};
std::optional<Geometry> geometry(int category, glm::ivec3 allocation, glm::ivec2 viewport, const Placement &);
float phase(int32_t guiClockMs);
float shininess(const nativesaves::Item &);
glm::vec3 lightDirection();
// The WORLD values retained between shared item calls after screen-overlay setup.
struct ShaderState {
    glm::vec4 front{0.7f, 0.7f, 0.2f, 1}, back{0.5f, 0.5f, 0.5f, 1}, ambient{0.3f, 0.3f, 0.8f, 1};
    glm::vec3 direction{0};
    float white = 0, glow = 0, shininess = 0;
};
ShaderState overlayState();
struct Request {
    size_t beforeVertex = 0; // immediate location among the Plasma triangles
    uint32_t mesh = 0, cubeMesh = 0;
    glm::ivec3 allocation{0};
    nativesaves::Item item{};
    Placement at;
};
struct Draw {
    uint32_t mesh = 0;
    glm::mat4 model{1}, view{1}, projection{1};
    glm::vec4 material{1};
    ShaderState state;
    float depthOffset = 0;
};
void appendDraws(const Request &, glm::ivec2 viewport, ShaderState &retained, std::vector<Draw> &);
struct SpiritCube {
    glm::mat4 model{1};
    glm::vec4 material{1};
};
std::vector<SpiritCube> spiritCubes(const nativesaves::Item &, const glm::mat4 &weaponModel);
} // namespace guiitem
