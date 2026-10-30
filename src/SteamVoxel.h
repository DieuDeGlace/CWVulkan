#pragma once
// Native voxel RGB and light-byte production, independent of geometric opacity.
// Facts: docs/specs/steam-voxel-vertex-inputs.md. Alpha uses its own encoder.
#include <glm/glm.hpp>
#include <glm/gtc/type_precision.hpp>
#include <functional>
#include <cstdint>
#include <array>

namespace steamvoxel {
using Point = glm::i64vec3;
inline constexpr int FACE_ORDER[6]{1, 0, 3, 2, 5, 4};
inline constexpr int CORNERS[6][4][3]{
    {{1,0,0}, {1,1,0}, {1,1,1}, {1,0,1}},
    {{0,0,0}, {0,0,1}, {0,1,1}, {0,1,0}},
    {{0,1,0}, {0,1,1}, {1,1,1}, {1,1,0}},
    {{0,0,0}, {1,0,0}, {1,0,1}, {0,0,1}},
    {{0,0,1}, {1,0,1}, {1,1,1}, {0,1,1}},
    {{0,0,0}, {0,1,0}, {1,1,0}, {1,0,0}}
};
inline constexpr std::array<uint32_t, 6> CUB_TRIANGLES{0, 1, 2, 0, 2, 3};
std::array<uint32_t, 6> terrainTriangles(const std::array<glm::u8vec3, 4> &rgb);
struct Sample {
    bool open = false; // raw-black CUB voxel, or effective terrain type0/2
    uint8_t light = 0; // completed terrain light-volume byte
};
using Probe = std::function<Sample(Point)>;
enum class CubEncoding { Stock, Unhalved };

uint8_t byte(float value); // native multiply255/truncate/low-byte conversion
// The returned fourth byte is light V, not an AO or opacity channel.
glm::u8vec4 cubCorner(glm::u8vec3 voxelRGB, Point corner, int face, const Probe &,
                      glm::u8vec3 additive = {}, CubEncoding = CubEncoding::Stock, int radius = 3);
glm::u8vec4 terrainCorner(glm::vec4 base, Point corner, int face, const Probe &, bool bypass = false);
} // namespace steamvoxel
