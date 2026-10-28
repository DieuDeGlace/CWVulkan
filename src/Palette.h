#pragma once
// Per-region colours, measured from vanilla reference shots (reference/).
//
// The shots show colours after vanilla's final pixel stage (re-expressed in
// SteamShadows' InlineTerrain.h): a saturation boost, a 35% grey mix into
// green and blue, then a smoothstep curve. CubeVulkan's shaders apply the same
// stage (vanillaTone in common.glsl), so a colour measured on a sunlit top
// face is turned back into the block colour that reproduces it.
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace palette {
// Inverse of 3x^2 - 2x^3 on [0, 1].
inline float inverseSmooth(float y) {
    y = std::clamp(y, 0.f, 1.f);
    return .5f - std::sin(std::asin(1 - 2 * y) / 3);
}

// Block colour (0..255) that vanilla's tone stage turns into `observed` when
// fully lit (top faces at noon receive light 1 after clamping).
// `surface` = false for sky and fog colours, which skip the saturation boost.
inline glm::vec3 albedo(glm::vec3 observed, bool surface = true) {
    glm::vec3 o = observed / 255.f, x;
    for (int i = 0; i < 3; i++)
        x[i] = inverseSmooth(o[i]);
    // Undo g,b = 0.65 c + 0.35 * luminance (r unchanged).
    const glm::vec3 lum(.299f, .587f, .114f);
    const float k = .35f;
    glm::mat3 m(1.f);
    for (int col = 0; col < 3; col++)
        for (int row = 1; row < 3; row++)
            m[col][row] = (row == col ? .65f : 0.f) + k * lum[col];
    x = glm::inverse(m) * x;
    if (!surface)
        return glm::clamp(x, 0.f, 1.f) * 255.f;
    // Undo the saturation boost (luminance is preserved by it).
    float l = glm::dot(lum, glm::clamp(x, 0.f, 1.f));
    float boost = 1 + .25f * (1 - l);
    x = (x - l * (1 - boost)) / boost;
    return glm::clamp(x, 0.f, 1.f) * 255.f;
}
} // namespace palette
