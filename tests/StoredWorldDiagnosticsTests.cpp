#include "../tools/ZoneExtractor/StoredWorldDiagnostics.h"
#include <iostream>

int storedWorldDiagnosticsTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL stored World diagnostics: " << label << '\n';
            ++failures;
        }
    };
    constexpr uintptr_t module = 0x140000000, world = 0x10000, head = 0x20000, node = 0x30000, zone = 0x40000;
    constexpr uintptr_t blocks = 0x900000;
    constexpr size_t column = 63 * 64 + 1;
    std::map<uintptr_t, std::vector<uint8_t>> memory;
    const auto store = [&](uintptr_t at, const auto &value) {
        auto &bytes = memory[at];
        bytes.resize(sizeof value);
        std::memcpy(bytes.data(), &value, sizeof value);
    };
    store(world + 0x468, std::array<uintptr_t, 2>{head, 1});
    store(head, std::array<uintptr_t, 2>{node, node});
    store(node, std::array<uintptr_t, 4>{head, head, 0, zone});
    std::array<uint8_t, 24> identity{};
    const uintptr_t table = module + captureabi::ZONE_TABLE_RVA;
    const std::array<int32_t, 2> coordinates{-1, 2};
    std::memcpy(identity.data(), &table, 8);
    std::memcpy(identity.data() + 8, &world, 8);
    std::memcpy(identity.data() + 16, coordinates.data(), 8);
    store(zone, identity);
    const auto field = zone + captureabi::ZONE_COLUMNS + column * captureabi::ZONE_COLUMN_STRIDE;
    store(field + 0x3c, int32_t(5));
    store(field + 0x40, std::array<uintptr_t, 3>{blocks, blocks + 18, blocks + 18});
    memory[blocks].resize(18);
    memory[blocks][4] = 0;
    memory[blocks][10] = 4;
    memory[blocks][16] = 2;
    size_t payloadReads = 0;
    const auto read = [&](uintptr_t at, void *out, size_t count) {
        auto found = memory.upper_bound(at);
        if (found == memory.begin())
            return false;
        --found;
        const auto offset = at - found->first;
        if (offset > found->second.size() || count > found->second.size() - offset)
            return false;
        if (at == blocks)
            ++payloadReads;
        std::memcpy(out, found->second.data() + offset, count);
        return true;
    };
    captureabi::StoredWorldDiagnostics view(module, world, read);
    bool solid = false;
    check(view.ready() && view.solid(-1, 129, 4, solid) && solid && view.solid(-1, 129, 5, solid) && !solid &&
              view.solid(-1, 129, 6, solid) && solid && view.solid(-1, 129, 7, solid) && !solid &&
              view.solid(-1, 129, INT64_MAX, solid) && !solid && payloadReads == 1,
          "negative block coordinates map to local63/1; cached column retains below/base/solid/water/above policy");
    memory[blocks][4] = 4;
    check(view.solid(-1, 129, 5, solid) && !solid && payloadReads == 1,
          "one pass reuses owned first-read bytes instead of repeated native copies");
    captureabi::StoredWorldDiagnostics refreshed(module, world, read);
    check(refreshed.solid(-1, 129, 5, solid) && solid && payloadReads == 2, "new pass refreshes source column");
    check(!view.solid(64, 129, 6, solid) && !view.solid(INT64_MIN, 129, 6, solid),
          "unloaded and out-of-coordinate-domain probes are unavailable, not known air");
    captureabi::StoredWorldDiagnostics missing(module, 0, read);
    check(!missing.ready(), "missing World rejected before any borrowed traversal");
    memory[zone][8] ^= 1;
    std::vector<captureabi::StoredZone> zones{{123, {4, 5}}};
    check(!captureabi::readStoredZones(module, world, zones, read) && zones.empty(),
          "wrong Zone owner publishes no partial index");
    std::cout << "Stored World diagnostic views: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
