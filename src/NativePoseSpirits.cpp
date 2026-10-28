// Weapon spirits (docs/specs/creature-pose-items.md §9.1, the weapon effect 0xeec50): the
// coloured voxels set into a weapon. Each spirit is one cube.cub voxel drawn at the weapon's
// own matrix, moved to the spirit's voxel of the weapon model, so it reads as an extra voxel in
// the blade. They do not move, glow or leave trails of their own.
#include "NativePose.h"

#include <cstring>

namespace nativepose {
namespace {

// An item record's spirits: up to 32 entries of four bytes from +0x1d (x, y, z as signed
// bytes, then the material), their number at +0x9d.
constexpr size_t SPIRITS = 0x1d, SPIRIT_COUNT = 0x9d, SPIRIT_SIZE = 4;
constexpr int MAX_SPIRITS = 32;

struct MaterialColour {
    int material;
    float r, g, b;
};
// Materials whose colour is multiplied by the tint (every channel, alpha too).
constexpr MaterialColour TINTED[] = {
    {1, 0.7f, 0.7f, 0.7f},  {2, 0.4f, 0.3f, 0.2f},   {3, 0.5f, 0.5f, 0.5f}, {5, 0.1f, 0.1f, 0.1f},
    {7, 0.9f, 0.9f, 0.9f},  {0xb, 1.0f, 0.7f, 0.2f}, {0xc, 0.8f, 0.8f, 0.85f},
};
// The four spirit materials: their own colour, only the tint's alpha taken, brightened by the
// call's strength.
constexpr MaterialColour SPIRIT_MATERIALS[] = {
    {0x80, 1.0f, 0.5f, 0.1f}, // fire: orange
    {0x81, 0.3f, 1.0f, 0.5f}, // green
    {0x82, 0.3f, 0.5f, 1.0f}, // blue
    {0x83, 0.8f, 0.8f, 1.0f}, // pale violet
};
// Any other material.
constexpr float OTHER_GREY = 0.5f;

} // namespace

std::array<float, 4> materialColour(int material, const float tint[4], float strength) {
    for (const MaterialColour &m : SPIRIT_MATERIALS)
        if (m.material == material) {
            const float bright = 0.5f * strength + 1.0f;
            return {m.r * bright, m.g * bright, m.b * bright, (strength + tint[3]) * bright};
        }
    std::array<float, 4> base = {OTHER_GREY, OTHER_GREY, OTHER_GREY, 1.0f};
    for (const MaterialColour &m : TINTED)
        if (m.material == material)
            base = {m.r, m.g, m.b, 1.0f};
    return {base[0] * tint[0], base[1] * tint[1], base[2] * tint[2], base[3] * tint[3]};
}

void spiritDraws(const Draw &effect, std::vector<Draw> &out) {
    if (effect.kind != WEAPON_EFFECT_DRAW || !effect.item || !effect.model)
        return;
    const uint8_t *item = effect.item;
    const int count = item[SPIRIT_COUNT] < MAX_SPIRITS ? item[SPIRIT_COUNT] : MAX_SPIRITS;
    for (int i = 0; i < count; i++) {
        const uint8_t *spirit = item + SPIRITS + SPIRIT_SIZE * i;
        const float x = (float)(int8_t)spirit[0], y = (float)(int8_t)spirit[1], z = (float)(int8_t)spirit[2];
        Draw cube;
        cube.model = effect.model;
        // The weapon matrix with its translation moved by (x, y, z) in the weapon's own voxels:
        // the cube is one weapon voxel in size, its corner on the spirit's voxel.
        std::memcpy(cube.matrix, effect.matrix, sizeof cube.matrix);
        const float *M = effect.matrix;
        for (int r = 0; r < 4; r++)
            cube.matrix[12 + r] = M[12 + r] + ((M[r] * x + M[4 + r] * y) + M[8 + r] * z);
        // Its colour, from the effect call's first number (0: the pose passes none).
        const std::array<float, 4> colour = materialColour(spirit[3], effect.tint, 0.0f);
        std::memcpy(cube.tint, colour.data(), sizeof cube.tint);
        cube.a = effect.a, cube.b = effect.b, cube.c = effect.c, cube.d = effect.d, cube.e = effect.e;
        cube.kind = PLAIN_DRAW;
        out.push_back(cube);
    }
}

} // namespace nativepose
