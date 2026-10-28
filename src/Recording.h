#pragma once
// Frames recorded by tools/ZoneExtractor from the game's Creature::animation state:
// the sprite and world matrix of every body part, once per tick.
//   CWA1: player recordings without a position.
//   CWA2: adds the creature's position (dots, 65536 per block).
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace recording {
constexpr int SPRITES = 14, MATRICES = 33;

// Sprite slots (in animation state order) and the matrices the game draws them
// with. Unused or absent parts hold garbage matrices, which isPart() rejects.
// face, hair, chest, unk, hands, wings, tail, feet, weapon R, weapon L, shoulder, neck.
// Native draws: wings use 13/14 (0x2aa74/0x2ab83), tail uses 17
// (0x27a40), shoulder uses 11 (0x278cd), neck uses 12 (0x27760).
inline const int SLOT_MATRICES[SPRITES][2] = {
    {3, -1}, {4, -1}, {2, -1}, {-1, -1}, {9, 10}, {13, 14}, {17, -1},
    {15, 16}, {7, -1}, {8, -1}, {11, -1}, {12, -1}, {-1, -1}, {-1, -1},
};
constexpr int HAIR_SLOT = 1, FEET_SLOT = 7;

struct Frame {
    uint64_t ms = 0;
    int64_t position[3] = {0, 0, 0}; // dots; zero in CWA1
    int32_t state = 0, timer = 0, flags = 0, ability = 0;
    float sinceAbility = 0, yaw = 0, velocity[3] = {0, 0, 0};
    int32_t slots[SPRITES]{};
    float matrices[MATRICES][16]{};
};

// The game's matrices are row-vector (D3D): in memory order they are glm's
// column-major columns. X/Y translations are relative to the creature, Z is
// absolute game height; matrix 0 supplies the creature's origin.
inline glm::mat4 matrix(const Frame &f, int i) { return glm::make_mat4(f.matrices[i]); }

// A real part transform: finite, affine, voxel-sized axes.
inline bool isPart(const glm::mat4 &m) {
    for (int c = 0; c < 4; c++)
        for (int r = 0; r < 4; r++)
            if (!std::isfinite(m[c][r]))
                return false;
    if (std::abs(m[0][3]) > 1e-3f || std::abs(m[1][3]) > 1e-3f || std::abs(m[2][3]) > 1e-3f ||
        std::abs(m[3][3] - 1) > 1e-3f)
        return false;
    for (int c = 0; c < 3; c++) {
        float a = glm::length(glm::vec3(m[c]));
        if (!(a > .005f && a < 1.f))
            return false;
    }
    return std::abs(m[3].x) < 1e5f && std::abs(m[3].y) < 1e5f && std::abs(m[3].z) < 1e5f;
}

inline bool read(const std::filesystem::path &path, std::vector<Frame> &out, bool *hasPosition = nullptr) {
    out.clear();
    if (hasPosition)
        *hasPosition = false;
    std::ifstream in(path, std::ios::binary);
    char magic[4];
    if (!in.read(magic, 4))
        return false;
    bool positioned = std::memcmp(magic, "CWA2", 4) == 0;
    if (!positioned && std::memcmp(magic, "CWA1", 4) != 0)
        return false;
    if (hasPosition)
        *hasPosition = positioned;
    const size_t size = 8 + (positioned ? 24 : 0) + 16 + 20 + SPRITES * 4 + MATRICES * 64;
    std::vector<char> b(size);
    while (in.read(b.data(), size)) {
        Frame f;
        const char *p = b.data();
        auto take = [&](void *dst, size_t n) {
            std::memcpy(dst, p, n);
            p += n;
        };
        take(&f.ms, 8);
        if (positioned)
            take(f.position, 24);
        take(&f.state, 16);
        take(&f.sinceAbility, 20);
        take(f.slots, SPRITES * 4);
        take(f.matrices, MATRICES * 64);
        if (!out.empty() && f.ms < out.back().ms) {
            out.clear();
            return false; // Playback requires ordered timestamps.
        }
        out.push_back(f);
    }
    return !out.empty();
}
} // namespace recording
