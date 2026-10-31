#include "CubModel.h"
#include "AlphaLight.h"
#include "AlphaLook.h"
#include "SteamVoxel.h"
#include <cstring>
#include <fstream>

bool CubModel::loadMemory(const uint8_t *data, size_t size, std::string *error) {
    auto fail = [&](const char *why) {
        if (error)
            *error = why;
        return false;
    };
    if (size < 12)
        return fail("file shorter than its header");
    int32_t dims[3];
    std::memcpy(dims, data, 12);
    // The largest Steam sprite is a few hundred voxels per side.
    if (dims[0] <= 0 || dims[1] <= 0 || dims[2] <= 0 || dims[0] > 1024 || dims[1] > 1024 ||
        dims[2] > 1024)
        return fail("implausible dimensions");
    size_t count = (size_t)dims[0] * dims[1] * dims[2];
    if (size != 12 + count * 3)
        return fail("size does not match dimensions");
    sx = dims[0];
    sy = dims[1];
    sz = dims[2];
    rgb.assign(data + 12, data + size);
    return true;
}

bool CubModel::load(const std::filesystem::path &file, std::string *error) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        if (error)
            *error = "cannot open file";
        return false;
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    return loadMemory(bytes.data(), bytes.size(), error);
}

bool CubModel::solid(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 || x >= sx || y >= sy || z >= sz)
        return false;
    const uint8_t *p = &rgb[((size_t)z * sy * sx + (size_t)y * sx + x) * 3];
    return p[0] | p[1] | p[2];
}

VoxelColor CubModel::color(int x, int y, int z) const {
    const uint8_t *p = &rgb[((size_t)z * sy * sx + (size_t)y * sx + x) * 3];
    return {p[0], p[1], p[2], 0};
}

bool CubModel::exposed(int x, int y, int z) const {
    if (!solid(x, y, z))
        return false;
    for (const auto &n : FACE_NORMALS)
        if (!solid(x + n[0], y + n[1], z + n[2]))
            return true;
    return false;
}

Mesh CubModel::mesh(uint8_t flags, glm::u8vec3 shadeColour, steamvoxel::CubEncoding encoding) const {
    Mesh m;
    m.nativeCentre = glm::vec3(0, 0, (float)sz * .5f); // x/y are already centered in emitted vertices
    auto isSolid = [&](int x, int y, int z) { return solid(x, y, z); };
    const bool alphaUnhalved = renderStyle() == RenderStyle::Alpha && encoding == steamvoxel::CubEncoding::Unhalved;
    auto colour = [&](int x, int y, int z) {
        auto c = color(x, y, z);
        if (alphaUnhalved) {
            c.r = uint8_t(std::min(255, int(c.r) * 2));
            c.g = uint8_t(std::min(255, int(c.g) * 2));
            c.b = uint8_t(std::min(255, int(c.b) * 2));
        }
        c.flags = flags;
        return c;
    };
    const glm::vec3 offset(-sx * .5f, -sy * .5f, 0);
    if (renderStyle() == RenderStyle::Alpha) {
        // Alpha (docs/specs/alpha-rendering.md 11.1): each corner's colour fades towards the
        // model's shade colour by how closed in it is; models carry no baked light.
        const glm::vec3 closed = glm::vec3(shadeColour) / 255.f;
        auto shade = [&](int x, int y, int z, int face, int corner, VoxelColor c) {
            const int *k = FACE_CORNERS[face][corner];
            float open = alphalight::modelOpenness(isSolid, {x + k[0], y + k[1], z + k[2]}, face);
            auto channel = [&](uint8_t v, float shut) { return alphalight::toByte(v / 255.f * open + (1 - open) * shut); };
            return CornerShade{channel(c.r, closed.r), channel(c.g, closed.g), channel(c.b, closed.b), 255};
        };
        meshFaces({0, 0, 0}, {sx, sy, sz}, isSolid, colour, shade, m, 1.f, offset);
    } else {
        const steamvoxel::Probe probe = [&](steamvoxel::Point p) {
            return steamvoxel::Sample{!solid((int)p.x, (int)p.y, (int)p.z), 0};
        };
        auto nativeColour = [&](int x, int y, int z) {
            auto c = colour(x, y, z);
            c.flags |= STEAM_VERTEX;
            return c;
        };
        auto shade = [&](int x, int y, int z, int face, int k, VoxelColor c) {
            const int *corner = steamvoxel::CORNERS[face][k];
            const auto encoded =
                steamvoxel::cubCorner({c.r, c.g, c.b}, {x + corner[0], y + corner[1], z + corner[2]}, face, probe, {}, encoding);
            return CornerShade{encoded.r, encoded.g, encoded.b, encoded.a};
        };
        meshFaces({0, 0, 0}, {sx, sy, sz}, isSolid, nativeColour, shade, m, 1.f, offset, VoxelLayout::SteamCub);
    }
    return m;
}
