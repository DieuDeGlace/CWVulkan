#pragma once
// CubeVulkan's optional surface decoration, inspired by water-rendering.md A.8.
// Reads finished render geometry; never consumes the generator's random stream.
#include "Terrain.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace waterLilies {
inline uint64_t mix(uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

template<class Eligible>
std::vector<Scenery> decorate(const Mesh &water, uint64_t seed, double patchDensity, int limit, Eligible eligible) {
    std::vector<Scenery> pads;
    std::set<std::pair<int, int>> visited;
    for (size_t first = 0; first + 3 < water.vertices.size() && (int)pads.size() < limit; first += 4) {
        const auto &a = water.vertices[first], &b = water.vertices[first + 1], &c = water.vertices[first + 2];
        glm::vec3 edge1(b.x - a.x, b.y - a.y, b.z - a.z), edge2(c.x - a.x, c.y - a.y, c.z - a.z);
        if (glm::cross(edge1, edge2).z <= 0 || edge1.z != 0 || edge2.z != 0)
            continue;
        glm::vec3 centre(0);
        for (size_t corner = 0; corner < 4; corner++) {
            const auto &v = water.vertices[first + corner];
            centre += glm::vec3(v.x, v.y, v.z) * .25f;
        }
        int x = (int)std::floor(centre.x), y = (int)std::floor(centre.y);
        if (!visited.insert({x, y}).second)
            continue;
        // Independent eight-block patches and a one-in-ten choice within them.
        int patchX = (int)std::floor(centre.x / 8), patchY = (int)std::floor(centre.y / 8);
        uint64_t patch = mix(seed ^ uint32_t(patchX) ^ (uint64_t(uint32_t(patchY)) << 32));
        if (double(patch & 65535) / 65536 >= std::clamp(patchDensity, 0.0, 1.0))
            continue;
        uint64_t choice = mix(seed ^ uint32_t(x) ^ (uint64_t(uint32_t(y)) << 32) ^ 0x4c494c59ULL);
        if (choice % 10 != 0 || !eligible(centre))
            continue;
        Scenery pad;
        pad.model = (choice & 256) ? "water-lily01" : "water-lily02";
        pad.x = centre.x;
        pad.y = centre.y;
        pad.z = centre.z + .1f;
        pad.scale = .09f;
        pad.yaw = float((choice >> 16) & 65535) * (6.283185307f / 65536);
        pads.push_back(pad);
    }
    return pads;
}
} // namespace waterLilies
