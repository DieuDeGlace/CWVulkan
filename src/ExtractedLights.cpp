#include "Extracted.h"
#include "SteamPointLights.h"
#include <cstring>
#include <fstream>

std::shared_ptr<const steampoints::ZoneSources> ExtractedWorld::lightSources(
    int zx, int zy, const worldgen::MeshLightVolume &solved) {
    if (solved.columns.empty())
        return {};
    const int64_t ax = (int64_t)zx + originX, ay = (int64_t)zy + originY;
    if (ax < INT32_MIN || ax > INT32_MAX || ay < INT32_MIN || ay > INT32_MAX)
        return {};
    auto zone = load((int)ax, (int)ay);
    if (!zone)
        return {};
    auto sources = std::make_shared<steampoints::ZoneSources>();
    sources->key = {(int32_t)ax, (int32_t)ay};
    if (zone->generated && zone->things) {
        for (const auto &model : zone->things->modelLights)
            sources->models.push_back({model});
        sources->objects = zone->things->lists.lights;
    } else {
        const auto path = folder / ("z_" + std::to_string(ax) + "_" + std::to_string(ay) + ".rec");
        std::ifstream in(path, std::ios::binary | std::ios::ate);
        if (!in || in.tellg() < 4 || in.tellg() > 128 * 1024 * 1024)
            return {};
        std::vector<uint8_t> bytes((size_t)in.tellg());
        in.seekg(0);
        if (!in.read((char *)bytes.data(), bytes.size()) || std::memcmp(bytes.data(), "CWR1", 4))
            return {};
        bool sawModels = false, sawObjects = false;
        for (size_t at = 4; at < bytes.size();) {
            if (bytes.size() - at < 12)
                return {};
            uint32_t header[3];
            std::memcpy(header, bytes.data() + at, sizeof header);
            at += sizeof header;
            const size_t stride = header[1], count = header[2];
            if (!stride || count > (bytes.size() - at) / stride)
                return {};
            if ((header[0] == 0x90 && (stride != 0x40 || sawModels)) ||
                (header[0] == 0xc0 && (stride != 0xe0 || sawObjects)))
                return {};
            if (header[0] == 0x90 && stride == 0x40) {
                sawModels = true;
                for (size_t i = 0; i < count; ++i) {
                    const auto *record = bytes.data() + at + i * stride;
                    steampoints::ModelSource source;
                    auto &m = source.record;
                    std::memcpy(&m.type, record, 4);
                    std::memcpy(&m.x, record + 8, 8);
                    std::memcpy(&m.y, record + 16, 8);
                    std::memcpy(&m.z, record + 24, 8);
                    std::memcpy(&m.r, record + 44, 4);
                    std::memcpy(&m.g, record + 48, 4);
                    std::memcpy(&m.b, record + 52, 4);
                    std::memcpy(&m.flags, record + 56, 4);
                    sources->models.push_back(source);
                }
            } else if (header[0] == 0xc0 && stride == 0xe0) {
                sawObjects = true;
                for (size_t i = 0; i < count; ++i) {
                    const auto *record = bytes.data() + at + i * stride;
                    worldgen::StaticObject object;
                    std::memcpy(&object.type, record, 4);
                    std::memcpy(&object.x, record + 8, 8);
                    std::memcpy(&object.y, record + 16, 8);
                    std::memcpy(&object.z, record + 24, 8);
                    std::memcpy(&object.scale, record + 36, 4);
                    sources->objects.push_back(object);
                }
            }
            at += stride * count;
        }
    }
    // Both native model lists receive independent mesh-time light writes.
    // Preserve auxiliary water flags separately from the ordinary model list.
    std::lock_guard lock(zone->modelLights->mutex);
    for (size_t index = 0; index < sources->models.size(); ++index) {
        auto &source = sources->models[index];
        auto &model = source.record;
        const steampoints::Point position{model.x, model.y, model.z};
        source.bakedLight = (float)solved.fixed(position);
        if (gameBlock(model.x >> 16, model.y >> 16, (int32_t)(model.z >> 16))[4] == 2)
            zone->modelLights->waterAuxiliary.insert(index);
        if ((model.flags & 8) || zone->modelLights->waterAuxiliary.count(index)) {
            model.flags |= 8;
            zone->modelLights->waterAuxiliary.insert(index);
        }
    }
    return sources;
}

std::vector<steampoints::ZoneKey> ExtractedWorld::loadedLightZones() const {
    std::vector<steampoints::ZoneKey> keys;
    std::lock_guard lock(cacheMutex);
    keys.reserve(cache.size());
    for (const auto &[key, zone] : cache)
        if (zone)
            keys.push_back(key);
    return keys;
}
