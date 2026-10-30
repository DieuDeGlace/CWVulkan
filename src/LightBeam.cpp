#include "LightBeam.h"
#include "SteamScenery.h"
#include <algorithm>
#include <cmath>

namespace lightbeam {
std::optional<Quad> sample(const Placement &p, glm::vec3 eye, float angleDegrees, float range, int daylight) {
    if (!(range > 0) || !std::isfinite(p.height) || !std::isfinite(p.width))
        return std::nullopt;
    const float distanceSquared = steamscenery::distanceSquared(p.position, eye);
    const float limitSquared = range * range;
    if (!(distanceSquared < limitSquared))
        return std::nullopt;
    const float fade = steamscenery::fade(distanceSquared, range);
    constexpr double PI = 3.14159265358979323846;
    const float radians = (float)((double)(angleDegrees / 180.0f) * PI);
    const float dx = std::cos(radians), dy = std::sin(radians);
    auto vertex = [&](int sign, bool bottom) {
        const float xOffset = bottom ? dx * p.width * 0.25f * p.height : dx * 0.5f * p.width;
        const float yOffset = bottom ? dy * p.width * 0.25f * p.height : dy * 0.5f * p.width;
        const int64_t x = p.position[0] + sign * (int64_t)(xOffset * 65536.0f);
        const int64_t y = p.position[1] + sign * (int64_t)(yOffset * 65536.0f);
        const int64_t z = p.position[2] - (bottom ? (int64_t)(p.height * 65536.0f) : 0);
        return glm::vec3((float)x, (float)y, (float)z) * (1.0f / 65536.0f);
    };
    Quad result;
    result.corners = {vertex(-1, false), vertex(1, false), vertex(-1, true), vertex(1, true)};
    result.alpha = (uint8_t)(int)(fade * (100 * daylight));
    result.distanceSquared = distanceSquared;
    return result;
}

Mesh mesh() {
    Mesh result;
    for (int i = 0; i < 4; ++i)
        result.vertices.push_back({(float)i, 0, 0, 255, 255, 255, (uint8_t)(i < 2 ? 255 : 0), 0, 0, 0, 0});
    result.indices = {0, 1, 2, 2, 1, 3};
    result.min = {0, 0, 0};
    result.max = {3, 0, 0};
    return result;
}
} // namespace lightbeam
