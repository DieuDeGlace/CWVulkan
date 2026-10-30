#pragma once
// Steam's per-frame light sources and zone tables, independently implemented
// from docs/specs/steam-scene-lighting.md §3. Shader draws use the first10 of16.
#include "worldgen/ZoneRecords.h"
#include <array>
#include <functional>
#include <map>
#include <optional>
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
struct GroundSource {
    uint8_t category = 0;
    int32_t id = 0, timerMs = 0, quantity = 0;
    int64_t owner = 0;
    Point position{};
};
struct GroundSources {
    std::optional<int64_t> player;
    std::vector<GroundSource> items;
};
struct EffectSource {
    int32_t kind = 0;
    Point position{};
    float strength = 0; // native effect +0x54, not its damage or visual size
};
struct Light {
    Point position{};
    float radius = 0;
    glm::vec3 colour{0};
    float distanceKey = 0;
};
using UsedCellQuery = std::function<bool(int32_t cellX, int32_t cellY)>;
struct Frame {
    Point camera{};
    float range = 0, daylight = 1, clockMs = 0;
    float terrainRange = 0; // projectile/item range; auxiliary models use range
    std::function<bool(const Point &, float radius, float range)> visible;
    UsedCellQuery usedCell;
};
// Registration retains source order. Distribution sorts all source families
// together by native squared camera distance and retains16 per existing zone.
void collect(const ZoneSources &, const Frame &, std::vector<Light> &out);
// Ground sources precede auxiliary model/object lights within each accepted zone.
void collectGround(const GroundSources &, float zoneDistanceSquared, const Frame &, std::vector<Light> &out);
float squaredDistance(const Point &, const Point &camera);
void collectEffects(const std::vector<EffectSource> &, const Frame &, std::vector<Light> &out);
float distanceKey(const Point &, const Point &camera);
// Full sprite center transformed by the final draw matrix; gameOrigin reverses
// the renderer's translation after the native float-to-fixed conversion.
ZoneKey bindingZone(const glm::mat4 &, glm::vec3 localCentre, const Point &gameOrigin);
using Tables = std::map<ZoneKey, std::vector<Light>>;
Tables distribute(std::vector<Light>, const Point &camera, const std::vector<ZoneKey> &loadedZones);
} // namespace steampoints
