#pragma once
// Host layouts shared by scene and immediate GUI rendering. Keep common.glsl in sync.
#include <glm/glm.hpp>
namespace rendergpu {
// std140-compatible: only mat4 and vec4. Must match shaders/common.glsl.
struct FrameUniforms {
    glm::mat4 viewProj, invViewProj, lightFar, lightNear;
    glm::vec4 eye, lightDir, front, back, ambient, fog, sky, shadowFar, shadowNear, wind, focus, sunDisk, moonDisk, steamSky[6];
    glm::vec4 alphaSky1, alphaSky2, alphaFog, alphaFront, alphaBack, alphaAmbient, alphaMaterial, alphaSun, alphaView, sunColour,
        alphaMoonFront, alphaMoonBack, alphaNight;
    glm::mat4 skyProj, view;
    glm::vec4 pointLights[64], pointColours[64];
    glm::vec4 steamSun, steamMaterial; // direction/ambient alpha, frame material RGBA
};
struct NativeGpuPoint {
    glm::vec4 position, colour;
};
struct PushConstants {
    glm::mat4 model;
    glm::vec4 params;
    glm::vec4 look;    // Alpha look: light where the object stands, hit flash, shimmer
    glm::uvec4 lights; // Alpha look: point-light indices, 8 bits each in x..z; w count
};
static_assert(sizeof(PushConstants) <= 128, "push constants beyond Vulkan's guaranteed 128 bytes");

} // namespace rendergpu
