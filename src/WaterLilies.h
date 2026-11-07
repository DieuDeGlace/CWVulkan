#pragma once
// Optional render decoration. Never consumes the generator's random stream.
#include "Terrain.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>
#include <utility>

namespace waterLilies {
inline uint64_t mix(uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}
namespace detail {
inline bool surface(const Mesh &water, size_t first, glm::vec3 offset, glm::vec3 &centre) {
    const auto position = [&](size_t i) {
        const auto &v = water.vertices[first + i];
        return offset + glm::vec3(v.x, v.y, v.z);
    };
    const glm::vec3 a = position(0), edge1 = position(1) - a, edge2 = position(2) - a;
    if (glm::cross(edge1, edge2).z <= 0 || edge1.z != 0 || edge2.z != 0)
        return false;
    centre = glm::vec3(0);
    for (size_t i = 0; i < 4; ++i)
        centre += position(i) * .25f;
    return true;
}
template<class Eligible> struct Builder {
    uint64_t seed;
    double density;
    int limit;
    Eligible eligible;
    std::vector<Scenery> pads;
    std::set<std::pair<int, int>> visited;
    bool full() const { return int(pads.size()) >= limit; }
    void accept(glm::vec3 centre) {
        const int x = int(std::floor(centre.x)), y = int(std::floor(centre.y));
        if (!visited.insert({x, y}).second)
            return;
        const int patchX = int(std::floor(centre.x / 8)), patchY = int(std::floor(centre.y / 8));
        const uint64_t patch = mix(seed ^ uint32_t(patchX) ^ (uint64_t(uint32_t(patchY)) << 32));
        if (double(patch & 65535) / 65536 >= std::clamp(density, 0.0, 1.0))
            return;
        const uint64_t choice = mix(seed ^ uint32_t(x) ^ (uint64_t(uint32_t(y)) << 32) ^ 0x4c494c59ULL);
        if (choice % 10 != 0 || !eligible(centre))
            return;
        Scenery pad;
        pad.model = (choice & 256) ? "water-lily01" : "water-lily02";
        pad.x = centre.x;
        pad.y = centre.y;
        pad.z = centre.z + .1f;
        pad.scale = .09f;
        pad.yaw = float((choice >> 16) & 65535) * (6.283185307f / 65536);
        pads.push_back(pad);
    }
};
} // namespace detail

template<class Eligible>
std::vector<Scenery> decorate(const Mesh &water, uint64_t seed, double density, int limit, Eligible eligible) {
    detail::Builder<Eligible> out{seed, density, limit, std::move(eligible)};
    for (size_t first = 0; first + 3 < water.vertices.size() && !out.full(); first += 4) {
        glm::vec3 centre;
        if (detail::surface(water, first, {}, centre))
            out.accept(centre);
    }
    return std::move(out.pads);
}

template<class Eligible>
std::vector<Scenery> decorate(const std::vector<TerrainMeshBand> &bands, glm::vec3 offset,
                              uint64_t seed, double density, int limit, Eligible eligible) {
    if (limit <= 0 || density <= 0)
        return {};
    struct Surface { glm::vec3 centre, order; bool whole; };
    std::vector<Surface> surfaces;
    for (const auto &band : bands) {
        const Mesh &mesh = band.water;
        for (size_t first = 0; first + 3 < mesh.vertices.size(); first += 4) {
            glm::vec3 centre, local;
            if (!detail::surface(mesh, first, offset + glm::vec3(0, 0, float(band.base)), centre) ||
                !detail::surface(mesh, first, {}, local))
                continue;
            const auto &a = mesh.vertices[first], &b = mesh.vertices[first + 1], &c = mesh.vertices[first + 2];
            const bool whole = std::abs(b.x - a.x) == 64 && std::abs(c.y - b.y) == 64;
            local.z += float(band.base);
            surfaces.push_back({centre, local, whole});
        }
    }
    // Recover the former aggregate mesh's sea-quad-first, Y/X/Z visitation so
    // duplicate cells, eligibility calls and the per-zone pad cap stay unchanged.
    std::stable_sort(surfaces.begin(), surfaces.end(), [](const Surface &a, const Surface &b) {
        if (a.whole != b.whole)
            return a.whole;
        return std::tie(a.order.y, a.order.x, a.order.z) < std::tie(b.order.y, b.order.x, b.order.z);
    });
    detail::Builder<Eligible> out{seed, density, limit, std::move(eligible)};
    for (const auto &surface : surfaces) {
        if (out.full())
            break;
        out.accept(surface.centre);
    }
    return std::move(out.pads);
}
} // namespace waterLilies
