#include "AlphaLight.h"
#include <algorithm>
#include <cmath>

namespace alphalight {

LightVolume::LightVolume(glm::ivec3 origin, glm::ivec3 size)
    : first(origin), extent(glm::max(size, glm::ivec3(0))), kinds((size_t)extent.x * extent.y * extent.z, BlockKind::Solid),
      lights((size_t)extent.x * extent.y * extent.z, 0), roofed((size_t)extent.x * extent.y, 0) {
}

bool LightVolume::inside(glm::ivec3 p) const {
    glm::ivec3 l = p - first;
    return l.x >= 0 && l.y >= 0 && l.z >= 0 && l.x < extent.x && l.y < extent.y && l.z < extent.z;
}

void LightVolume::flood() {
    const int sx = extent.x, sy = extent.y, sz = extent.z;
    if (!sx || !sy || !sz)
        return;
    const size_t layer = (size_t)sx * sy;
    // Step 1: sky light down each column, until the first block that is not open.
    std::fill(lights.begin(), lights.end(), uint8_t(0));
    std::vector<size_t> unlit; // open blocks the sky does not reach: the ones the sweeps fill
    for (int y = 0; y < sy; y++)
        for (int x = 0; x < sx; x++) {
            bool sky = !roofed[(size_t)y * sx + x];
            for (int z = sz - 1; z >= 0; z--) {
                size_t i = (size_t)z * layer + (size_t)y * sx + x;
                if (kinds[i] != BlockKind::Open)
                    sky = false;
                else if (sky)
                    lights[i] = 255;
                else
                    unlit.push_back(i);
            }
        }
    // Step 2: up to sixteen synchronous sweeps. Once every value is unchanged,
    // the state is a fixed point and all remaining sweeps would repeat it.
    std::vector<uint8_t> next(unlit.size());
    for (int sweep = 0; sweep < 16; sweep++) {
        bool changed = false;
        for (size_t k = 0; k < unlit.size(); k++) {
            size_t i = unlit[k];
            int x = (int)(i % sx), y = (int)(i / sx % sy), z = (int)(i / layer);
            int brightest = 0;
            auto neighbour = [&](int nx, int ny, int nz) {
                if (nx < 0 || ny < 0 || nz < 0 || nx >= sx || ny >= sy || nz >= sz)
                    return; // outside the box: nothing comes in
                size_t j = (size_t)nz * layer + (size_t)ny * sx + nx;
                int value = kinds[j] == BlockKind::Glow ? 255 : kinds[j] == BlockKind::Open ? std::max<int>(lights[j], 5) : 0;
                brightest = std::max(brightest, value);
            };
            neighbour(x + 1, y, z), neighbour(x - 1, y, z);
            neighbour(x, y + 1, z), neighbour(x, y - 1, z);
            neighbour(x, y, z + 1), neighbour(x, y, z - 1);
            next[k] = (uint8_t)((unsigned)brightest * 85u / 100u);
            changed |= next[k] != lights[i];
        }
        if (!changed)
            break;
        for (size_t k = 0; k < unlit.size(); k++)
            lights[unlit[k]] = next[k];
    }
}

CornerLight cornerLight(const LightVolume &volume, glm::ivec3 corner, int face) {
    const int *n = FACE_NORMALS[face];
    int axis = n[0] ? 0 : n[1] ? 1 : 2, ua = (axis + 1) % 3, va = (axis + 2) % 3;
    float lightSum = 0, openSum = 0, weightSum = 0;
    for (int u = -3; u <= 2; u++)
        for (int v = -3; v <= 2; v++) {
            // Blocks nearer the corner count more: 5/6, 1/2 or 1/6 by the farther axis.
            float reach = std::max({std::abs(u + .5f), std::abs(v + .5f), .5f});
            float weight = (3 - reach) / 3;
            glm::ivec3 p = corner;
            p[axis] += n[axis] > 0 ? 0 : -1; // the layer just outside the face
            p[ua] += u;
            p[va] += v;
            BlockKind kind = volume.kind(p);
            float value = kind == BlockKind::Glow ? 255.f : kind == BlockKind::Open ? (float)std::max<int>(volume.light(p), 5) : 5.f;
            lightSum += value * weight;
            openSum += kind == BlockKind::Solid ? 0 : weight;
            weightSum += weight;
        }
    return {lightSum / (weightSum * 255), openSum / weightSum};
}

glm::ivec3 environmentBlock(const std::array<int64_t, 3> &position) {
    return {int32_t(position[0] / 65536), int32_t(position[1] / 65536), int32_t(position[2] / 65536)};
}
float environmentLight(const LightVolume &volume, glm::ivec3 at) {
    const auto kind = volume.kind(at);
    if (kind == BlockKind::Glow)
        return 1;
    if (kind != BlockKind::Open)
        return 0;
    return float(std::max<int>(volume.light(at), 5)) / 255.f;
}

CornerCache::CornerCache(const LightVolume &volume, glm::ivec2 firstCorner, glm::ivec2 cornerCount)
    : volume(volume), first(firstCorner), extent(glm::max(cornerCount, glm::ivec2(0))) {
    for (auto &slice : slices)
        slice.values.resize(size_t(extent.x) * extent.y * 6);
}
CornerLight CornerCache::sample(glm::ivec3 corner, int face) {
    const int64_t x = int64_t(corner.x) - first.x, y = int64_t(corner.y) - first.y;
    if (x < 0 || y < 0 || x >= extent.x || y >= extent.y)
        return cornerLight(volume, corner, face);
    auto &slice = slices[uint32_t(corner.z) & 1u];
    if (!slice.ready || slice.z != corner.z) {
        std::fill(slice.values.begin(), slice.values.end(), std::nullopt);
        slice.z = corner.z;
        slice.ready = true;
    }
    auto &value = slice.values[(size_t(y) * extent.x + size_t(x)) * 6 + face];
    if (!value)
        value = cornerLight(volume, corner, face);
    return *value;
}

glm::vec3 occludedColour(glm::vec3 colour, float open) {
    glm::vec3 c = glm::clamp(colour, 0.f, 1.f);
    // The deeper shade: each channel minus 0.9 of the colour's luminance.
    float luminance = (.59f * c.g * .1f + .3f * c.r * .1f + .11f * c.b * .1f) * -9;
    glm::vec3 shade = glm::clamp(c * .1f * 10.f + luminance, 0.f, 1.f);
    return colour * open + (1 - open) * shade;
}

glm::vec3 waterColour(float light) {
    return (1 - light) * glm::vec3(.1f, .2f, 1) + light * glm::vec3(.3f, .4f, 1);
}

CornerShade shadeCorner(const LightVolume &volume, glm::ivec3 corner, int face, VoxelColor colour) {
    return shadeCorner(volume, corner, face, glm::vec3(colour.r, colour.g, colour.b) / 255.f);
}
CornerShade shadeCorner(const LightVolume &volume, glm::ivec3 corner, int face, glm::vec3 colour) {
    return shadeCorner(cornerLight(volume, corner, face), colour);
}
CornerShade shadeCorner(CornerLight light, glm::vec3 colour) {
    glm::vec3 rgb = occludedColour(colour, light.open);
    return {toByte(rgb.r), toByte(rgb.g), toByte(rgb.b), toByte(light.light)};
}

} // namespace alphalight
