#pragma once
// Cube World Alpha's light baked into chunk and model meshes: the light flood through empty
// and water blocks, each vertex's light and openness, and the occlusion colour.
//
// Written from docs/specs/alpha-rendering.md, section 9 (blocks) and 11.1 (models).
#include "Mesher.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace alphalight {

// What a block is to the light.
enum class BlockKind : uint8_t {
    Solid, // stops light
    Open,  // empty or water: carries light
    Glow,  // a block that counts as full light (the spec's type 13)
};

// A box of blocks with a light value (0..255) for each, as the flood leaves it.
class LightVolume {
  public:
    // Blocks [origin, origin + size). Every block starts Solid.
    LightVolume(glm::ivec3 origin, glm::ivec3 size);
    glm::ivec3 origin() const { return first; }
    glm::ivec3 size() const { return extent; }
    bool inside(glm::ivec3 p) const;
    void setKind(glm::ivec3 p, BlockKind kind) { kinds[index(p)] = kind; }
    // Marks a column as covered above the box: no sky light comes down it.
    void setRoofed(int x, int y) { roofed[(size_t)(y - first.y) * extent.x + (x - first.x)] = 1; }
    // Outside the box: Solid, light 0.
    BlockKind kind(glm::ivec3 p) const { return inside(p) ? kinds[index(p)] : BlockKind::Solid; }
    uint8_t light(glm::ivec3 p) const { return inside(p) ? lights[index(p)] : 0; }

    // §9.2: open blocks with nothing but open blocks above them in their column are sky-lit
    // (255); the rest are filled by 16 sweeps, each taking 85 % of the brightest neighbour
    // (a glowing neighbour counts 255, an open one at least 5). The box's top layer is taken
    // to be under the open sky unless its column is marked roofed.
    void flood();

  private:
    size_t index(glm::ivec3 p) const {
        glm::ivec3 l = p - first;
        return ((size_t)l.z * extent.y + l.y) * extent.x + l.x;
    }
    glm::ivec3 first, extent;
    std::vector<BlockKind> kinds;
    std::vector<uint8_t> lights;
    std::vector<uint8_t> roofed; // per column (x + y * width)
};

// One mesh corner's light (0..1, applied by the shader) and openness (0..1, darkens the
// colour), from the 6 x 6 blocks in front of its face (§9.3).
struct CornerLight {
    float light = 1, open = 1;
};
// `corner` is the vertex's integer position, `face` the face index (0 +X .. 5 -Z).
CornerLight cornerLight(const LightVolume &volume, glm::ivec3 corner, int face);

// §9.4: a block colour (0..1) darkened towards a deeper, more saturated shade as the corner
// closes in.
glm::vec3 occludedColour(glm::vec3 colour, float open);

// §9.5: water's colour from the water block's own light (0..1).
glm::vec3 waterColour(float light);

// §9.6: a colour channel (0..1) as a vertex byte.
inline uint8_t toByte(float channel) { return (uint8_t)((int)(channel * 255) & 0xff); }

// A mesh corner shaded for the Alpha look: rgb from occludedColour, alpha the corner light.
CornerShade shadeCorner(const LightVolume &volume, glm::ivec3 corner, int face, VoxelColor colour);

// §11.1: a model corner's openness, the share of empty voxels among the 6 x 6 in front of
// its face. `solid(x, y, z)` tests one voxel.
template <class Solid> float modelOpenness(Solid &&solid, glm::ivec3 corner, int face) {
    const int *n = FACE_NORMALS[face];
    int axis = n[0] ? 0 : n[1] ? 1 : 2, ua = (axis + 1) % 3, va = (axis + 2) % 3;
    int empty = 0, sampled = 0;
    for (int u = -3; u <= 2; u++)
        for (int v = -3; v <= 2; v++) {
            glm::ivec3 p = corner;
            p[axis] += n[axis] > 0 ? 0 : -1;
            p[ua] += u;
            p[va] += v;
            empty += solid(p.x, p.y, p.z) ? 0 : 1;
            sampled++;
        }
    return (float)empty / (float)sampled;
}

} // namespace alphalight
