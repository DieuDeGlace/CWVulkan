#pragma once
#include "NativeLoadedZones.h"
#include "NativeZoneTerrain.h"
#include <map>

namespace captureabi {
struct StoredZone {
    uintptr_t address;
    std::array<int32_t, 2> coordinates;
};
// Caller retains World/Zone lifetime and the applicable zone/buffer leases.
template <class Read>
bool readStoredZones(uintptr_t module, uintptr_t world, std::vector<StoredZone> &output, Read read) {
    output.clear();
    std::vector<uintptr_t> addresses;
    if (!readLoadedZones(world, addresses, read))
        return false;
    std::vector<StoredZone> zones;
    zones.reserve(addresses.size());
    for (auto address : addresses) {
        std::array<int32_t, 2> coordinates;
        if (!readZoneIdentity(module, world, address, coordinates, read))
            return false;
        zones.push_back({address, coordinates});
    }
    output = std::move(zones);
    return true;
}

// Per-pass owned column cache for the existing diagnostic solid-block policy.
// This is not a native physics replacement or an atomic whole-World snapshot.
template <class Read> class StoredWorldDiagnostics {
    Read read;
    std::vector<StoredZone> storedZones;
    std::map<std::pair<uintptr_t, size_t>, StoredColumn> columns;
    size_t remainingBytes = MAX_ZONE_BLOCK_BYTES;
    bool valid;

  public:
    StoredWorldDiagnostics(uintptr_t module, uintptr_t world, Read reader) : read(reader) {
        valid = readStoredZones(module, world, storedZones, read);
    }
    bool ready() const { return valid; }
    bool solid(int64_t x, int64_t y, int64_t z, bool &output) {
        output = false;
        const int64_t zx = x / 64 - (x % 64 < 0), zy = y / 64 - (y % 64 < 0);
        if (!valid || zx < INT32_MIN || zx > INT32_MAX || zy < INT32_MIN || zy > INT32_MAX)
            return false;
        for (const auto &zone : storedZones) {
            if (zone.coordinates[0] != zx || zone.coordinates[1] != zy)
                continue;
            const size_t index = size_t(x - zx * 64) * 64 + size_t(y - zy * 64);
            const auto key = std::make_pair(zone.address, index);
            auto found = columns.find(key);
            if (found == columns.end()) {
                StoredColumn column;
                if (!readStoredColumn(zone.address, index, column, read, remainingBytes))
                    return false;
                remainingBytes -= column.blocks.size() * sizeof(StoredBlock);
                found = columns.emplace(key, std::move(column)).first;
            }
            const auto &column = found->second;
            if (z < column.baseZ) {
                output = true;
                return true;
            }
            // Avoid signed overflow at an extreme probe coordinate.
            const auto indexZ = uint64_t(z) - uint64_t(int64_t(column.baseZ));
            if (indexZ < column.blocks.size()) {
                const auto type = column.blocks[size_t(indexZ)][4];
                output = type != 0 && type != 2 && type != 3 && type != 17 && type != 19;
            }
            return true;
        }
        return false; // Unloaded/unavailable is distinct from known air.
    }
};
} // namespace captureabi
