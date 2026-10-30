#pragma once
// Voxel face mesher with per-vertex ambient occlusion (or, for the Alpha look, baked light:
// see AlphaLight.h), shared by terrain and .cub models. Faces between two solid voxels are
// skipped.
#include <glm/glm.hpp>
#include "SteamVoxel.h"
#include <cstdint>
#include <vector>

// 20 bytes. Normal ids match the native game's face ids:
// 0 +X, 1 -X, 2 +Y, 3 -Y, 4 +Z, 5 -Z.
struct Vertex {
    float x, y, z;
    uint8_t r, g, b, ao; // Native: packed RGB/light V. Classic: RGB/corner AO.
    uint8_t normal, flags, pad0, pad1;
};
enum VertexFlags : uint8_t {
    SWAY = 1,  // Leaf blocks: the leaf-sway option moves them in the vertex shader (old generator: a swaying plant).
    GLOW = 2,  // Unlit (lamps, runes).
    WATER_SURFACE = 4, // Water surface.
    NATIVE_MENU = 8, // Captured menu clip transform, with native colours baked.
    PACKED_NORMAL = 16, // normal/pad0/pad1 hold an arbitrary unit normal as UNORM8.
    WORLD_MAP = 32, // (DrawItem only) the world map's own pass: DrawItem::map, shaders/map.*.
    MENU_BACKGROUND = 64, // Screen-space interface art, blended behind the menu's models.
    STEAM_VERTEX = 128, // Native packed RGB/V semantics; shader doubles only RGB.
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 min{1e30f}, max{-1e30f};
    bool empty() const { return indices.empty(); }
};

struct VoxelColor {
    uint8_t r, g, b, flags;
};

// Corner positions of each face in a unit cube, counter-clockwise seen from
// outside, and the face normals.
inline const int FACE_CORNERS[6][4][3] = {
    {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}, // +X
    {{0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1, 1}}, // -X
    {{1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1}}, // +Y
    {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, // -Y
    {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, // +Z
    {{1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}}, // -Z
};
inline const int FACE_NORMALS[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
                                       {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

// The colour and fourth byte one face corner is given (see meshFaces).
struct CornerShade {
    uint8_t r, g, b, a;
};

enum class VoxelLayout { Classic, SteamCub, SteamTerrain };
using MeshSelector = std::function<Mesh &(int x, int y, int z)>;
inline void appendFaceIndices(Mesh &mesh, uint32_t base, const CornerShade (&corners)[4], VoxelLayout layout) {
    auto indices = steamvoxel::CUB_TRIANGLES;
    if (layout == VoxelLayout::SteamTerrain)
        indices = steamvoxel::terrainTriangles({glm::u8vec3(corners[0].r, corners[0].g, corners[0].b),
            glm::u8vec3(corners[1].r, corners[1].g, corners[1].b), glm::u8vec3(corners[2].r, corners[2].g, corners[2].b),
            glm::u8vec3(corners[3].r, corners[3].g, corners[3].b)});
    else if (layout == VoxelLayout::Classic && corners[0].a + corners[2].a < corners[1].a + corners[3].a)
        indices = {1, 2, 3, 1, 3, 0};
    for (uint32_t index : indices)
        mesh.indices.push_back(base + index);
}

// Meshes every solid voxel in [lo, hi), emitting the faces that border a non-solid voxel.
// `solid(x,y,z)` may be queried outside the range. `color(x,y,z)` gives a solid voxel's
// colour. `shade(x, y, z, face, corner, colour)` gives each face corner's final colour and
// fourth byte (ambient occlusion or baked light; higher is brighter): `corner` indexes
// FACE_CORNERS[face]. Vertices are `offset + p * scale`.
template <class Solid, class Color, class Shade>
void meshFaces(glm::ivec3 lo, glm::ivec3 hi, Solid &&solid, Color &&color, Shade &&shade, Mesh &output,
               float scale = 1, glm::vec3 offset = glm::vec3(0), VoxelLayout layout = VoxelLayout::Classic,
               const MeshSelector &select = {}) {
    for (int z = lo.z; z < hi.z; z++)
        for (int y = lo.y; y < hi.y; y++)
            for (int x = lo.x; x < hi.x; x++) {
                if (!solid(x, y, z))
                    continue;
                Mesh &mesh = select ? select(x, y, z) : output;
                VoxelColor c{};
                bool haveColor = false;
                for (int face = 0; face < 6; face++) {
                    const int f = layout == VoxelLayout::Classic ? face : steamvoxel::FACE_ORDER[face];
                    const int *n = FACE_NORMALS[f];
                    if (solid(x + n[0], y + n[1], z + n[2]))
                        continue;
                    if (!haveColor) {
                        c = color(x, y, z);
                        haveColor = true;
                    }
                    CornerShade corners[4];
                    for (int k = 0; k < 4; k++)
                        corners[k] = shade(x, y, z, f, k, c);
                    uint32_t base = (uint32_t)mesh.vertices.size();
                    for (int k = 0; k < 4; k++) {
                        const int *corner = layout == VoxelLayout::Classic ? FACE_CORNERS[f][k] : steamvoxel::CORNERS[f][k];
                        glm::vec3 p = offset + glm::vec3(x + corner[0], y + corner[1],
                                                         z + corner[2]) * scale;
                        const CornerShade &s = corners[k];
                        mesh.vertices.push_back({p.x, p.y, p.z, s.r, s.g, s.b, s.a, (uint8_t)f, c.flags, 0, 0});
                        mesh.min = glm::min(mesh.min, p);
                        mesh.max = glm::max(mesh.max, p);
                    }
                    appendFaceIndices(mesh, base, corners, layout);
                }
            }
}

// meshFaces with the classic per-corner ambient occlusion: each corner looks at the two
// voxels beside it and the one diagonal to it, in the layer in front of the face.
template <class Solid, class Color>
void meshVoxels(glm::ivec3 lo, glm::ivec3 hi, Solid &&solid, Color &&color, Mesh &mesh,
                float scale = 1, glm::vec3 offset = glm::vec3(0)) {
    auto occlusion = [&](int x, int y, int z, int f, int k, VoxelColor c) {
        const int *n = FACE_NORMALS[f];
        int nx = x + n[0], ny = y + n[1], nz = z + n[2];
        int axis = n[0] ? 0 : n[1] ? 1 : 2;
        int ua = (axis + 1) % 3, va = (axis + 2) % 3;
        const int *corner = FACE_CORNERS[f][k];
        int su = corner[ua] ? 1 : -1, sv = corner[va] ? 1 : -1;
        int u[3] = {0, 0, 0}, v[3] = {0, 0, 0};
        u[ua] = su;
        v[va] = sv;
        bool s1 = solid(nx + u[0], ny + u[1], nz + u[2]);
        bool s2 = solid(nx + v[0], ny + v[1], nz + v[2]);
        bool sc = solid(nx + u[0] + v[0], ny + u[1] + v[1], nz + u[2] + v[2]);
        int level = s1 && s2 ? 0 : 3 - (s1 + s2 + sc);
        return CornerShade{c.r, c.g, c.b, (uint8_t)(level * 85)};
    };
    meshFaces(lo, hi, solid, color, occlusion, mesh, scale, offset);
}
