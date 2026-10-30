#include "SteamPointLights.h"
#include "worldgen/Noise.h"
#include "SteamScenery.h"
#include "worldgen/StructureTables.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace steampoints {
namespace {
int64_t signedBits(uint64_t bits) {
    int64_t value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
int64_t add(int64_t value, int64_t delta) { return signedBits((uint64_t)value + (uint64_t)delta); }
Point raised(Point point, float height) {
    point[2] = add(point[2], worldgen::trunc64((double)(height * 65536.0f)));
    return point;
}
float rawDistance(const Point &point, const Point &camera) {
    float d[3];
    for (int axis = 0; axis < 3; ++axis)
        d[axis] = (float)signedBits((uint64_t)point[axis] - (uint64_t)camera[axis]) * (1.0f / 65536.0f);
    return d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
}
float flicker(const Point &point, float clockMs) {
    const double x = (double)signedBits((uint64_t)point[0] * 10) * (1.0 / 65536.0);
    const double y = (double)signedBits((uint64_t)point[1] * 10) * (1.0 / 65536.0);
    return std::cos(10 * worldgen::simplex(x, y) + .01f * clockMs);
}
// Personal cells narrow to signed32 blocks before the negative exactness
// correction and floor division (steam-scene-lighting.md section 3.5.1).
int32_t personalCell(int64_t fixed) {
    uint32_t bits = (uint32_t)(fixed / 65536);
    int32_t block;
    std::memcpy(&block, &bits, sizeof block);
    if (fixed < 0 && (int64_t)block * 65536 != fixed) {
        --bits;
        std::memcpy(&block, &bits, sizeof block);
    }
    return block >= 0 ? block / 16 : (int32_t)(((int64_t)block - 15) / 16);
}
bool visible(const Frame &frame, const Point &point, float radius) {
    return frame.visible && frame.visible(point, radius, frame.range);
}
}

float distanceKey(const Point &point, const Point &camera) {
    uint64_t sum = 0;
    for (int axis = 0; axis < 3; ++axis) {
        const uint64_t delta = (uint64_t)point[axis] - (uint64_t)camera[axis];
        sum += (uint64_t)(signedBits(delta * delta) / 65536);
    }
    return (float)signedBits(sum) * (1.0f / 65536.0f);
}

ZoneKey bindingZone(const glm::mat4 &matrix, glm::vec3 centre, const Point &origin) {
    glm::vec4 transformed;
    for (int row = 0; row < 4; ++row) {
        float value = centre.x * matrix[0][row] + centre.y * matrix[1][row];
        value += centre.z * matrix[2][row];
        transformed[row] = value + matrix[3][row];
    }
    const float reciprocal = 1 / transformed.w;
    int32_t zones[2];
    for (int axis = 0; axis < 2; ++axis) {
        const float scaled = (transformed[axis] * reciprocal) * 65536.0f;
        const int64_t fixed = add(worldgen::trunc64((double)scaled), origin[axis]);
        const int32_t block = (int32_t)(fixed >> 16);
        zones[axis] = block >> 6;
    }
    return {zones[0], zones[1]};
}

void collect(const ZoneSources &zone, const Frame &frame, std::vector<Light> &out) {
    for (const auto &source : zone.models) {
        const auto &model = source.record;
        const Point position{model.x, model.y, model.z};
        if (model.type == 0x4c) {
            const float limit = frame.range + 32;
            if (rawDistance(position, frame.camera) < limit * limit) {
                out.push_back({position, 30 + 2 * flicker(position, frame.clockMs), {model.r, model.g, model.b}});
                break; // only this zone's auxiliary model list, not its objects
            }
            continue;
        }
        if (!(model.flags & 1) || !visible(frame, position, 16))
            continue;
        float multiplier = 1;
        if (model.type == 0x17 || model.type == 0x18 || model.type == 0x19) {
            const float sunlight = (source.bakedLight * frame.daylight) / 255.0f;
            if (!(sunlight <= .4000000059604645f))
                continue;
            multiplier = 1 - sunlight;
        }
        out.push_back({position, 15 + .5f * flicker(position, frame.clockMs),
                       glm::vec3(model.r, model.g, model.b) * multiplier});
    }
    for (const auto &object : zone.objects) {
        const Point base{object.x, object.y, object.z};
        const float height = worldgen::objectSize(object.type).z * object.scale;
        Point test = base, position = raised(base, height);
        float testRadius = 32, radius = 20;
        glm::vec3 colour(0);
        bool dayGate = false, personal = false;
        switch (object.type) {
        case 0x3f:
        case 0x40:
            test = position;
            testRadius = 50;
            radius = 32;
            colour = {1.7f, 1.5f, .8f};
            dayGate = true;
            break;
        case 0x42:
            test = position;
            testRadius = 20;
            colour = {.3f, 2, .5f};
            dayGate = true;
            break;
        case 0x53:
            position = raised(base, height + 1);
            radius = 32;
            colour = {1.5f, 1, .4f};
            break;
        case 0:
        case 0x7e:
            position = raised(base, height + 1);
            radius = 32;
            colour = object.type == 0 ? glm::vec3(.25f, .3f, .5f) : glm::vec3(1.5f, 1, .4f);
            personal = true;
            break;
        case 0x41:
            colour = {1.2f, .8f, .4f};
            dayGate = true;
            break;
        case 0x70:
        case 0x71:
            colour = {.6f, .7f, 1.3f};
            break;
        case 0x72:
            colour = {1.2f, 1, .5f};
            dayGate = true;
            break;
        case 0x10:
            colour = {1.2f, 1, .5f};
            break;
        default:
            continue;
        }
        if (dayGate && !(frame.daylight < .4000000059604645f))
            continue;
        if (personal && (!frame.usedCell || !frame.usedCell(personalCell(base[0]), personalCell(base[1]))))
            continue;
        if (visible(frame, test, testRadius))
            out.push_back({position, radius, colour});
    }
}

float squaredDistance(const Point &position, const Point &camera) { return rawDistance(position, camera); }

void collectGround(const GroundSources &sources, float zoneDistanceSquared, const Frame &frame, std::vector<Light> &out) {
    const float limit = frame.terrainRange + 64;
    if (!(zoneDistanceSquared < limit * limit))
        return;
    for (const auto &item : sources.items) {
        if (item.timerMs != 0 || item.quantity == 0 ||
            (sources.player && item.owner != 0 && item.owner != *sources.player))
            continue;
        glm::vec3 colour(0);
        if (item.category == 0x0b && item.id == 0x13) colour = {.1f, .3f, .3f};
        else if (item.category == 0x0e) colour = {.1f, .3f, .5f};
        else if (item.category == 0x13 && item.id == 0) colour = {.6f, .6f, .2f};
        else if (item.category == 0x13 && item.id == 1) colour = {0, .6f, .2f};
        else continue;
        const auto position = raised(item.position, 3);
        if (frame.visible && frame.visible(position, 16, frame.terrainRange)) {
            const float fade = steamscenery::fade(rawDistance(position, frame.camera), frame.terrainRange);
            out.push_back({position, 16, colour * fade}); // zero fade still consumes a native light slot
        }
    }
}

float abilityRange(float sceneRange, int32_t zoneRadius) {
    const uint32_t shifted = (uint32_t)zoneRadius << 6;
    int32_t blocks;
    std::memcpy(&blocks, &shifted, sizeof blocks);
    return std::min(std::min((float)blocks * .5f, sceneRange), 256.f);
}

void collectEffects(const std::vector<EffectSource> &effects, const Frame &frame, std::vector<Light> &out) {
    for (const auto &effect : effects) {
        if ((effect.kind == 3 || effect.kind == 4) && frame.visible &&
            frame.visible(effect.position, 16, frame.terrainRange))
            out.push_back({effect.position, 16, glm::vec3(.5f, .5f, .2f) * effect.strength});
    }
}

void collectTransient(const std::vector<TransientSource> &sources, const Frame &frame, std::vector<Light> &out) {
    for (const auto &source : sources)
        if (frame.visible && frame.visible(source.position, source.radius, frame.terrainRange)) {
            const float fade = std::max(0.f, 1.f - (float)source.elapsedMs / (float)source.lifetimeMs);
            out.push_back({source.position, source.radius, source.colour * fade});
        }
}

Tables distribute(std::vector<Light> lights, const Point &camera, const std::vector<ZoneKey> &loadedZones) {
    Tables tables;
    for (auto key : loadedZones)
        tables.try_emplace(key);
    for (auto &light : lights)
        light.distanceKey = distanceKey(light.position, camera);
    std::sort(lights.begin(), lights.end(), [](const Light &a, const Light &b) { return a.distanceKey < b.distanceKey; });
    for (const auto &light : lights) {
        const int64_t extent = worldgen::trunc64((double)(light.radius * 65536.0f));
        const int64_t negativeExtent = signedBits(0 - (uint64_t)extent);
        const int64_t minX = add(light.position[0], negativeExtent) >> 22, maxX = add(light.position[0], extent) >> 22;
        const int64_t minY = add(light.position[1], negativeExtent) >> 22, maxY = add(light.position[1], extent) >> 22;
        // Iterating existing zones avoids enormous rectangles from malformed radii.
        // No sphere/zone intersection filter is part of the native membership rule.
        for (auto &[key, table] : tables)
            if (key.first >= minX && key.first <= maxX && key.second >= minY && key.second <= maxY && table.size() < 16)
                table.push_back(light);
    }
    return tables;
}
} // namespace steampoints
