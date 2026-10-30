#include "SteamVoxel.h"
#include <algorithm>
#include <cmath>

namespace steamvoxel {
namespace {
constexpr int NORMALS[6][3]{{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
struct Layer {
    float total = 0, open = 0, light = 0;
};
Layer sample(Point corner, int face, const Probe &probe, int radius) {
    Layer result;
    if (face < 0 || face >= 6 || radius <= 0)
        return result;
    int begin[3], end[3];
    for (int axis = 0; axis < 3; ++axis) {
        const int normal = NORMALS[face][axis];
        begin[axis] = normal > 0 ? 0 : normal < 0 ? -1 : -radius;
        end[axis] = normal > 0 ? 1 : normal < 0 ? 0 : radius;
    }
    for (int x = begin[0]; x < end[0]; ++x)
        for (int y = begin[1]; y < end[1]; ++y)
            for (int z = begin[2]; z < end[2]; ++z) {
                const float furthest = std::max({std::abs(x + .5f), std::abs(y + .5f), std::abs(z + .5f)});
                const float weight = (radius - furthest) / radius;
                const auto voxel = probe(corner + Point{x, y, z});
                result.total += weight;
                if (voxel.open) {
                    result.open += weight;
                    result.light += voxel.light * weight;
                }
            }
    return result;
}

glm::vec3 colour(glm::vec3 base, float openness) {
    const float occlusion = std::pow(openness, 1.2000000476837158f);
    base *= occlusion;
    float luminance = .3f * base.r + .59f * base.g + .11f * base.b;
    if (luminance > 1) {
        base *= 1 / luminance;
        luminance = 1;
    }
    const float saturation = 3 - 2 * occlusion;
    return glm::clamp(glm::vec3((1 - saturation) * luminance) + saturation * base, glm::vec3(0), glm::vec3(1));
}

glm::u8vec4 pack(glm::vec4 value) { return {byte(value.r), byte(value.g), byte(value.b), byte(value.a)}; }
}

std::array<uint32_t, 6> terrainTriangles(const std::array<glm::u8vec3, 4> &rgb) {
    float luminance[4];
    for (int corner = 0; corner < 4; ++corner)
        luminance[corner] = .29899999499320984f * rgb[corner].r + .5870000123977661f * rgb[corner].g +
                            .11400000005960464f * rgb[corner].b;
    if (std::abs(luminance[1] - luminance[3]) <= std::abs(luminance[0] - luminance[2]))
        return {0, 1, 3, 2, 3, 1};
    return CUB_TRIANGLES;
}

uint8_t byte(float value) {
    const float scaled = value * 255;
    // The native invalid signed conversion has low byte0. Avoid undefined casts.
    if (!std::isfinite(scaled) || scaled >= 2147483648.0f || scaled < -2147483648.0f)
        return 0;
    return (uint8_t)(int32_t)scaled;
}

glm::u8vec4 cubCorner(glm::u8vec3 voxelRGB, Point corner, int face, const Probe &probe,
                      glm::u8vec3 additive, CubEncoding encoding, int radius) {
    const auto layer = sample(corner, face, probe, radius);
    const float openness = layer.total > 0 ? std::max(.5f, layer.open / layer.total) : 0;
    const auto rgb = colour(glm::vec3(voxelRGB) / 255.0f + glm::vec3(additive) / 255.0f, openness);
    glm::vec4 result(rgb, 2);
    if (encoding == CubEncoding::Stock)
        result *= .5f;
    return pack(result);
}

glm::u8vec4 terrainCorner(glm::vec4 base, Point corner, int face, const Probe &probe, bool bypass) {
    if (bypass)
        return pack(base);
    const auto layer = sample(corner, face, probe, 3);
    const float openness = layer.total > 0 ? layer.open / layer.total : 0;
    const float light = layer.open > 0 ? layer.light / layer.open / 255.0f : 0;
    return pack(glm::vec4(colour(glm::vec3(base), openness), light));
}
} // namespace steamvoxel
