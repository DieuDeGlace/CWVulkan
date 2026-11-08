#include "Extracted.h"
#include "SteamPointLights.h"
#include <cstring>
#include <fstream>

void ExtractedWorld::readModelLists(const std::filesystem::path &zoneFile, int ax, int ay, Zone &zone) {
    auto models = std::make_shared<std::vector<ModelRecord>>();
    zone.models = models;
    auto path = zoneFile;
    path.replace_extension(".rec");
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input || input.tellg() < 4 || input.tellg() > 128 * 1024 * 1024)
        return;
    std::vector<uint8_t> bytes(size_t(input.tellg()));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bytes.data()), bytes.size()) || std::memcmp(bytes.data(), "CWR1", 4))
        return;
    auto sources = std::make_shared<steampoints::ZoneSources>();
    sources->key = {ax, ay};
    bool sawModels = false, sawObjects = false, valid = true;
    for (size_t at = 4; at < bytes.size();) {
        if (bytes.size() - at < 12)
            return;
        uint32_t header[3];
        std::memcpy(header, bytes.data() + at, sizeof header);
        at += sizeof header;
        const size_t stride = header[1], count = header[2];
        if (!stride || count > (bytes.size() - at) / stride)
            return;
        if (header[0] == 0x18 && stride == sizeof(ModelRecord) && count) {
            const size_t first = models->size();
            models->resize(first + count);
            std::memcpy(models->data() + first, bytes.data() + at, count * stride);
        } else if (header[0] == 0x90) {
            if (stride != 0x40 || sawModels)
                valid = false;
            else
                for (size_t i = 0; i < count; ++i) {
                    const auto *record = bytes.data() + at + i * stride;
                    steampoints::ModelSource source;
                    auto &m = source.record;
                    std::memcpy(&m.type, record, 4);
                    std::memcpy(&m.x, record + 8, 8);
                    std::memcpy(&m.y, record + 16, 8);
                    std::memcpy(&m.z, record + 24, 8);
                    std::memcpy(&m.size, record + 32, 4);
                    std::memcpy(&m.yaw, record + 36, 4);
                    std::memcpy(&source.bakedLight, record + 40, 4);
                    std::memcpy(&m.r, record + 44, 4);
                    std::memcpy(&m.g, record + 48, 4);
                    std::memcpy(&m.b, record + 52, 4);
                    std::memcpy(&m.flags, record + 56, 4);
                    sources->models.push_back(source);
                }
            sawModels = true;
        } else if (header[0] == 0xc0) {
            if (stride != 0xe0 || sawObjects)
                valid = false;
            else
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
            sawObjects = true;
        }
        at += stride * count;
    }
    if (valid)
        zone.lightSources = std::move(sources);
}

std::shared_ptr<const steampoints::ZoneSources> ExtractedWorld::lightSources(int zx, int zy) {
    const int64_t ax = int64_t(zx) + originX, ay = int64_t(zy) + originY;
    if (ax < INT32_MIN || ax > INT32_MAX || ay < INT32_MIN || ay > INT32_MAX)
        return {};
    auto zone = editedZone(int(ax), int(ay));
    if (!zone)
        zone = cached(int(ax), int(ay));
    return zone ? zone->lightSources : nullptr;
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
