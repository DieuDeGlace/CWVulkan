#pragma once
// Steam's per-frame light sources and zone tables, independently implemented
// from docs/specs/steam-scene-lighting.md §3. Shader draws use the first10 of16.
#include "worldgen/ZoneRecords.h"
#include <array>
#include <functional>
#include <map>
#include <utility>
#include <vector>
#include <glm/glm.hpp>

namespace steampoints {
using Point = std::array<int64_t, 3>;
using ZoneKey = std::pair<int32_t, int32_t>;
struct ModelSource {
    worldgen::Model record;
    float bakedLight = 0; // mesh-written +0x28 in this auxiliary copy
};
struct ZoneSources {
    ZoneKey key;
    std::vector<ModelSource> models; // auxiliary +0x90, including null markers
    std::vector<worldgen::StaticObject> objects; // auxiliary +0xc0
};
struct Light {
    Point position{};
    float radius = 0;
    glm::vec3 colour{0};
    float distanceKey = 0;
};
struct Frame {
    Point camera{};
    float range = 0, daylight = 1, clockMs = 0;
    std::function<bool(const Point &, float radius, float range)> visible;
    std::function<bool(int64_t cellX, int64_t cellY)> usedCell;
};
// Registration retains source order. Distribution sorts all source families
// together by native squared camera distance and retains16 per existing zone.
void collect(const ZoneSources &, const Frame &, std::vector<Light> &out);
float distanceKey(const Point &, const Point &camera);
// Full sprite center transformed by the final draw matrix; gameOrigin reverses
// the renderer's translation after the native float-to-fixed conversion.
ZoneKey bindingZone(const glm::mat4 &, glm::vec3 localCentre, const Point &gameOrigin);
using Tables = std::map<ZoneKey, std::vector<Light>>;
Tables distribute(std::vector<Light>, const Point &camera, const std::vector<ZoneKey> &loadedZones);
} // namespace steampoints
