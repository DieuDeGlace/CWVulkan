// Optional owner-data oracle: compare fresh auxiliary light snapshots with the
// engine reader and the same-neighborhood mesh-time light solve. No assets bundled.
#include "Extracted.h"
#include "AlphaLook.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
template <class T> T field(const uint8_t *data, size_t offset) {
    T value;
    std::memcpy(&value, data + offset, sizeof value);
    return value;
}
}
int steamLightOracleTests(const std::filesystem::path &directory, const std::filesystem::path &fileList) {
    ExtractedWorld world;
    std::string error;
    if (!world.open(directory, error)) {
        std::cerr << error << '\n';
        return 1;
    }
    std::ifstream names(fileList);
    if (!names) {
        std::cerr << "Fresh snapshot file list is missing\n";
        return 1;
    }
    const auto oldStyle = renderStyle();
    setRenderStyle(RenderStyle::Steam);
    size_t zones = 0, models = 0, objects = 0, mismatches = 0, incomplete = 0;
    std::string name;
    while (std::getline(names, name)) {
        if (!name.empty() && name.back() == '\r') name.pop_back();
        const auto relative = std::filesystem::path(name);
        if (relative.filename() != relative || relative.extension() != ".rec") {
            std::cerr << "Expected one zone record filename per line\n";
            ++mismatches;
            continue;
        }
        std::ifstream in(directory / relative, std::ios::binary | std::ios::ate);
        const auto length = in ? in.tellg() : std::streampos(-1);
        if (length < 4 || length > 128 * 1024 * 1024) { ++mismatches; continue; }
        std::vector<uint8_t> bytes((size_t)length);
        in.seekg(0);
        if (!in.read((char *)bytes.data(), bytes.size()) || std::memcmp(bytes.data(), "CWR1", 4)) {
            ++mismatches; continue;
        }
        std::vector<std::array<uint8_t, 64>> modelRecords;
        std::vector<std::array<uint8_t, 224>> objectRecords;
        bool valid = true;
        for (size_t at = 4; at < bytes.size();) {
            if (bytes.size() - at < 12) { valid = false; break; }
            const auto kind = field<uint32_t>(bytes.data(), at);
            const auto stride = field<uint32_t>(bytes.data(), at + 4);
            const auto count = field<uint32_t>(bytes.data(), at + 8);
            at += 12;
            if (!stride || count > (bytes.size() - at) / stride) { valid = false; break; }
            if ((kind == 0x90 && stride != 64) || (kind == 0xc0 && stride != 224)) {
                valid = false; break;
            }
            if (kind == 0x90)
                for (uint32_t i = 0; i < count; ++i) {
                    modelRecords.emplace_back();
                    std::memcpy(modelRecords.back().data(), bytes.data() + at + i * stride, stride);
                }
            if (kind == 0xc0)
                for (uint32_t i = 0; i < count; ++i) {
                    objectRecords.emplace_back();
                    std::memcpy(objectRecords.back().data(), bytes.data() + at + i * stride, stride);
                }
            at += (size_t)stride * count;
        }
        if (!valid) { ++mismatches; continue; }
        if (modelRecords.empty() && objectRecords.empty()) continue;
        auto zoneFile = relative;
        zoneFile.replace_extension(".cwz");
        int zx = 0, zy = 0;
        if (!ExtractedWorld::zoneCoordinates(zoneFile, zx, zy)) { ++mismatches; continue; }
        const int localX = zx - world.originZoneX(), localY = zy - world.originZoneY();
        Mesh opaque, water;
        worldgen::MeshLightVolume volume;
        if (!world.mesh(localX, localY, opaque, water, nullptr, &volume) || volume.columns.empty()) {
            ++incomplete;
            continue;
        }
        const auto actual = world.lightSources(localX, localY, volume);
        if (!actual || actual->models.size() != modelRecords.size() || actual->objects.size() != objectRecords.size()) {
            ++mismatches;
            continue;
        }
        ++zones;
        for (size_t i = 0; i < modelRecords.size(); ++i) {
            const auto *raw = modelRecords[i].data();
            const auto &a = actual->models[i];
            const auto &r = a.record;
            const bool match = r.type == field<int32_t>(raw, 0) && r.x == field<int64_t>(raw, 8) &&
                r.y == field<int64_t>(raw, 16) && r.z == field<int64_t>(raw, 24) &&
                a.bakedLight == field<float>(raw, 40) && r.r == field<float>(raw, 44) &&
                r.g == field<float>(raw, 48) && r.b == field<float>(raw, 52) && r.flags == field<int32_t>(raw, 56);
            if (!match) {
                std::cerr << name << " model " << i << " sample engine=" << a.bakedLight
                          << " native=" << field<float>(raw, 40) << '\n';
                ++mismatches;
            }
            ++models;
        }
        for (size_t i = 0; i < objectRecords.size(); ++i) {
            const auto *raw = objectRecords[i].data();
            const auto &a = actual->objects[i];
            if (a.type != field<int32_t>(raw, 0) || a.x != field<int64_t>(raw, 8) || a.y != field<int64_t>(raw, 16) ||
                a.z != field<int64_t>(raw, 24) || a.scale != field<float>(raw, 36)) ++mismatches;
            ++objects;
        }
    }
    setRenderStyle(oldStyle);
    std::cout << "Steam light oracle: " << zones << " nonempty zones; " << models << " model lights, " << objects
              << " static lights; " << mismatches << " mismatches; " << incomplete << " incomplete neighborhoods\n";
    return mismatches || incomplete || !zones || (!models && !objects) ? 1 : 0;
}
