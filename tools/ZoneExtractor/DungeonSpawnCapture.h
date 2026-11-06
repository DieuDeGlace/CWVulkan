#pragma once
#include "NativeInventory.h"
#include "NativeSpawnCellTrees.h"
#include <map>
#include <sstream>
#include <string>
#include <tuple>

namespace dungeoncapture {
inline constexpr size_t MaxSpawnCaptureBytes = 256 * 1024;
inline constexpr char SpawnCaptureHeader[] = "CW-DUNGEON-SPAWN-CELLS 1";
inline constexpr char SpawnCaptureWithKeysHeader[] = "CW-DUNGEON-SPAWN-CELLS 2";
inline constexpr uint8_t DungeonKeyCategory = 0x16;
inline constexpr size_t MaxKeysPerSpawn = 32;

struct SpawnKey {
    int32_t quantity = 0, id = 0;
    captureabi::SpawnCell region{};
    uint32_t modifier = 0, realm = 0;
    unsigned rarity = 0, material = 0;

    bool operator==(const SpawnKey &other) const {
        return std::tie(quantity, id, region, modifier, realm, rarity, material) ==
               std::tie(other.quantity, other.id, other.region, other.modifier, other.realm, other.rarity, other.material);
    }
};

struct SpawnCapture {
    captureabi::SpawnCell zone{};
    uint32_t seed = 0;
    int32_t day = 0;
    std::vector<captureabi::SpawnCellRecord> records;
    bool keysRecorded = false;
    std::map<int64_t, std::vector<SpawnKey>> keys;
};

template <class Read> bool readSpawnKeys(uintptr_t spawn, std::vector<SpawnKey> &output, Read read) {
    // The generated spawn owns an inventory at +0x7f0 (worldgen-creatures.md §0.3, §4.5).
    captureabi::Inventory inventory;
    if (!captureabi::readInventoryAt(spawn, 0x7f0, inventory, read, 16, 256))
        return false;
    std::vector<SpawnKey> keys;
    for (const auto &tab : inventory) {
        for (const auto &stack : tab) {
            const auto item = stack.item();
            if (item.category() != DungeonKeyCategory)
                continue;
            if (keys.size() == MaxKeysPerSpawn)
                return false;
            keys.push_back({stack.quantity(), item.subtype(), {item.regionX(), item.regionY()}, item.modifier(),
                            uint32_t(item.representedType()), item.rarity(), item.material()});
        }
    }
    output = std::move(keys);
    return true;
}

inline std::string encodeSpawnCells(const SpawnCapture &capture) {
    std::ostringstream output;
    output << (capture.keysRecorded ? SpawnCaptureWithKeysHeader : SpawnCaptureHeader)
           << '\n' << capture.zone[0] << ' ' << capture.zone[1] << ' '
           << capture.seed << ' ' << capture.day << ' ' << capture.records.size() << '\n';
    for (const auto &record : capture.records) {
        output << record.id;
        for (const auto *cells : {&record.cells.guards, &record.cells.activation}) {
            output << ' ' << cells->size();
            for (const auto &[x, y] : *cells)
                output << ' ' << x << ' ' << y;
        }
        if (capture.keysRecorded) {
            const auto keys = capture.keys.find(record.id);
            output << ' ' << (keys == capture.keys.end() ? 0 : keys->second.size());
            if (keys != capture.keys.end())
                for (const auto &key : keys->second)
                    output << ' ' << key.quantity << ' ' << key.id << ' ' << key.region[0] << ' ' << key.region[1]
                           << ' ' << key.modifier << ' ' << key.realm << ' ' << key.rarity << ' ' << key.material;
        }
        output << '\n';
    }
    return output.str();
}

inline bool decodeSpawnCells(const std::string &text, SpawnCapture &output) {
    if (text.size() > MaxSpawnCaptureBytes)
        return false;
    std::istringstream input(text);
    std::string header;
    SpawnCapture capture;
    size_t count = 0;
    if (!std::getline(input, header) || (header != SpawnCaptureHeader && header != SpawnCaptureWithKeysHeader) ||
        !(input >> capture.zone[0] >> capture.zone[1] >> capture.seed >> capture.day >> count) ||
        count > captureabi::MaxZoneSpawnRecords)
        return false;
    capture.keysRecorded = header == SpawnCaptureWithKeysHeader;
    size_t cellCount = 0;
    for (size_t i = 0; i < count; ++i) {
        captureabi::SpawnCellRecord record;
        if (!(input >> record.id) || (!capture.records.empty() && record.id <= capture.records.back().id))
            return false;
        for (auto *cells : {&record.cells.guards, &record.cells.activation}) {
            size_t size = 0;
            if (!(input >> size) || size > captureabi::MaxSpawnCells || size > captureabi::MaxZoneSpawnCells - cellCount)
                return false;
            cellCount += size;
            for (size_t j = 0; j < size; ++j) {
                captureabi::SpawnCell cell{};
                if (!(input >> cell[0] >> cell[1]) || (!cells->empty() && !(cells->back() < cell)))
                    return false;
                cells->push_back(cell);
            }
        }
        if (capture.keysRecorded) {
            size_t size = 0;
            if (!(input >> size) || size > MaxKeysPerSpawn)
                return false;
            if (size) {
                auto &keys = capture.keys[record.id];
                for (size_t j = 0; j < size; ++j) {
                    SpawnKey key;
                    if (!(input >> key.quantity >> key.id >> key.region[0] >> key.region[1] >> key.modifier >>
                          key.realm >> key.rarity >> key.material) || key.rarity > 255 || key.material > 255)
                        return false;
                    keys.push_back(key);
                }
            }
        }
        capture.records.push_back(std::move(record));
    }
    input >> std::ws;
    if (!input.eof())
        return false;
    output = std::move(capture);
    return true;
}
} // namespace dungeoncapture
