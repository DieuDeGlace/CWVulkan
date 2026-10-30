#include "SteamSceneRange.h"
#include "GameMath.h"
#include "NativeKeyOrder.h"
#include "worldgen/Noise.h"
#include <algorithm>
#include <bit>
#include <cmath>

namespace scenerange {
namespace {
int64_t subtract(int64_t value, int64_t origin) {
    return std::bit_cast<int64_t>((uint64_t)value - (uint64_t)origin);
}
double horizontalSquared(Point position, Point camera) {
    const double x = (double)subtract(position[0], camera[0]) * (1.0 / 65536.0);
    const double y = (double)subtract(position[1], camera[1]) * (1.0 / 65536.0);
    return x*x + y*y;
}
float zoneWidth(int32_t radius) {
    return (float)std::bit_cast<int32_t>((uint32_t)radius << 6);
}
}
int32_t triangleCost(size_t opaqueIndices, size_t waterIndices, size_t lavaIndices) {
    const uint32_t cost = (uint32_t)(opaqueIndices / 3) + 2u * (uint32_t)(waterIndices / 3) + (uint32_t)(lavaIndices / 3);
    return std::bit_cast<int32_t>(cost);
}
glm::vec3 groundColour(Point physical) {
    const int32_t x = (int32_t)(physical[0] / 65536), y = (int32_t)(physical[1] / 65536);
    const double u = .01 * (double)x, v = .01 * (double)y;
    return {127.f + 100.f * worldgen::simplex(u + 9434, v + 123123),
            127.f + 100.f * worldgen::simplex(u + 7658, v + 3256),
            127.f + 100.f * worldgen::simplex(u + 6577, v + 74765)};
}
std::optional<float> candidate(const Frame &frame) {
    if (!frame.zone) return {};
    const int64_t x = frame.camera[0] >> 22, y = frame.camera[1] >> 22;
    const int64_t n = frame.zoneRadius;
    const float width = zoneWidth(frame.zoneRadius);
    float bound = width * width;
    for (int64_t zx = x - n; zx <= x + n; ++zx) for (int64_t zy = y - n; zy <= y + n; ++zy) {
        const auto zone = frame.zone(zx,zy);
        if (zone && zone->meshed) continue;
        const Point corner{std::bit_cast<int64_t>((uint64_t)zx << 22),
                           std::bit_cast<int64_t>((uint64_t)zy << 22), 0};
        const double distance = horizontalSquared(corner, frame.camera);
        if (distance < (double)bound) bound = (float)distance;
    }
    struct Entry { float distance; int32_t cost; };
    std::vector<Entry> accepted;
    for (int64_t zx = x - n; zx <= x + n; ++zx) for (int64_t zy = y - n; zy <= y + n; ++zy) {
        const auto zone = frame.zone(zx,zy);
        if (!zone || !zone->meshed || (frame.visible && !frame.visible(*zone,100000.f))) continue;
        const double distance = horizontalSquared(zone->center,frame.camera);
        if (distance > (double)bound) continue;
        if (!zone->triangleCost) return {}; // a legacy greedy mesh is not evidence of native cost
        accepted.push_back({(float)distance,*zone->triangleCost});
    }
    keyorder::sort(accepted, [](const Entry &entry) { return entry.distance; });
    const int64_t budget = n * n * 12288;
    int64_t total = 0;
    float previous = 0;
    for (const auto &entry : accepted) {
        total = std::bit_cast<int64_t>((uint64_t)total + (uint64_t)(int64_t)entry.cost);
        if (total >= budget) { bound = std::min(entry.distance,previous); break; }
        previous = entry.distance;
    }
    return std::max(std::sqrt(bound) - 44.8f, .001f);
}
bool State::update(const Frame &frame) {
    const auto distance = candidate(frame);
    if (!distance) return false;
    // The scene boundary narrows the capped wall interval to float, then the
    // native integration helper receives that rounded value as double.
    const double ms = (double)(float)std::min(frame.elapsedMs, 1000.0);
    const auto &player = frame.player;
    colour = (player.blockFlags & 0x22) == 0x22 ? groundColour(player.physical) : glm::vec3(0);
    const bool flagged = colour.r != 0 || colour.g != 0 || colour.b != 0;
    const float current = ranges.scene;
    float target = *distance, rate = .0005f;
    bool snap = false;
    if (player.ability == 0x9c && player.abilityMs > 1000) {
        target = std::max(target,64.f);
        rate = .005f;
    } else if ((!flagged && !player.effect13) || target <= 256) {
        snap = target <= current;
    } else target = 256;
    ranges.scene = snap ? target : current + (target-current) * gamemath::blendFactor(ms,rate);
    opacity = opacity + (1.f-opacity) * gamemath::blendFactor(ms,.05f);
    const float width = zoneWidth(frame.zoneRadius);
    ranges.models = std::min(std::min(width*.2f,ranges.scene),200.f);
    ranges.abilities = std::min(std::min(width*.5f,ranges.scene),256.f);
    colour /= 255.f;
    return true;
}
} // namespace scenerange
