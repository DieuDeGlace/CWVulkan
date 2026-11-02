#include "SceneVisibility.h"
#include "SteamScenery.h"
#include "GameMath.h"
#include <algorithm>
#include <cmath>

namespace scenevisibility {
Bounds build(int32_t zx, int32_t zy, const Columns &columns, const BlockType &type) {
    const int64_t x0 = (int64_t)zx * 64, y0 = (int64_t)zy * 64;
    Bounds result;
    result.upper = 0;
    bool exposed = false;
    for (int x = 0; x < 64; ++x)
        for (int y = 0; y < 64; ++y) {
            const int64_t gx = x0 + x, gy = y0 + y;
            const auto own = columns(gx, gy);
            if (!own)
                continue;
            int32_t base = own->base;
            const std::optional<Column> neighbors[] = {
                columns(gx - 1, gy), columns(gx + 1, gy), columns(gx, gy - 1), columns(gx, gy + 1)};
            for (const auto &neighbor : neighbors)
                if (neighbor)
                    base = std::min(base, neighbor->base);
            if (neighbors[3])
                result.upper = std::max(result.upper, neighbors[3]->end);
            for (int64_t z = (int64_t)base - 1; z < own->end; ++z) {
                const uint8_t current = type(gx, gy, z);
                if (!current)
                    continue;
                auto openFace = [&](int64_t nx, int64_t ny, int64_t nz) {
                    const uint8_t other = type(nx, ny, nz);
                    return other == 0 || (current != 2 && other == 2);
                };
                if (!(openFace(gx - 1, gy, z) || openFace(gx + 1, gy, z) ||
                      openFace(gx, gy - 1, z) || openFace(gx, gy + 1, z) ||
                      openFace(gx, gy, z - 1) || openFace(gx, gy, z + 1)))
                    continue;
                if (!exposed) {
                    result.lowest = result.upper = (int32_t)z;
                    exposed = true;
                } else {
                    result.lowest = std::min(result.lowest, (int32_t)z);
                    result.upper = std::max(result.upper, (int32_t)z);
                }
            }
        }
    result.upper = std::max(result.upper, 1);
    result.minimum = {x0 * 65536, y0 * 65536, (int64_t)result.lowest * 65536};
    result.maximum = {(x0 + 65) * 65536, (y0 + 65) * 65536, std::max(int64_t(0), (int64_t)result.upper + 1) * 65536};
    glm::vec3 half;
    for (int axis = 0; axis < 3; ++axis) {
        result.center[axis] = (result.minimum[axis] + result.maximum[axis]) / 2;
        half[axis] = (float)(result.center[axis] - result.maximum[axis]) * (1.0f / 65536.0f);
    }
    result.radius = std::sqrt(half.x * half.x + half.y * half.y + half.z * half.z) * 1.5f;
    result.plannedBands = (int32_t)(((int64_t)result.upper - result.lowest) / 250 + 1);
    return result;
}

Planes planes(const glm::mat4 &clip) {
    auto row = [&](int index) { return glm::vec4(clip[0][index], clip[1][index], clip[2][index], clip[3][index]); };
    return {row(3) + row(0), row(3) - row(0), row(3) + row(1), row(3) - row(1), row(3) + row(2), row(3) - row(2)};
}

bool View::visible(const Sphere &sphere, float range) const {
    if (!steamscenery::earlyWithinFixed(sphere.centre, eye, range))
        return false;
    glm::vec3 point;
    for (int axis = 0; axis < 3; ++axis)
        point[axis] = float(gamemath::addFixed(sphere.centre[axis], offset[axis])) * (1.f / 65536.f);
    for (const auto &plane : planes)
        if (((point.y * plane.y + point.x * plane.x) + point.z * plane.z) + plane.w + sphere.radius < 0)
            return false;
    return true;
}

bool View::box(const Bounds &bounds, float radius) const {
    const std::array<int64_t, 3> midpoint{gamemath::addFixed(bounds.minimum[0], bounds.maximum[0]) / 2,
                                          gamemath::addFixed(bounds.minimum[1], bounds.maximum[1]) / 2, 0};
    if (!steamscenery::earlyWithinFixed(midpoint, eye, radius, true))
        return false;
    for (int index : {0, 1, 2, 4, 5}) {
        const auto &plane = planes[index];
        glm::vec3 corner;
        for (int axis = 0; axis < 3; ++axis) {
            const int64_t fixed = plane[axis] > 0 ? bounds.maximum[axis] : bounds.minimum[axis];
            corner[axis] = float(gamemath::addFixed(fixed, offset[axis])) * (1.f / 65536.f);
        }
        if (((plane.y * corner.y + plane.x * corner.x) + plane.z * corner.z) + plane.w < 0)
            return false;
    }
    return true;
}

bool View::visible(const Bounds &bounds, float terrainRange, float modelRange) const {
    if (!bounds.meshed || !box(bounds, terrainRange + 64))
        return false;
    const float centerRange = modelRange + 64;
    return steamscenery::distanceSquaredFixed(bounds.center, eye) < centerRange * centerRange;
}
} // namespace scenevisibility
