#include "../src/Extracted.h"
#include "../tools/ZoneExtractor/ZoneTerrainCapture.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>

int nativeZoneTerrainTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL native Zone terrain: " << label << '\n';
            ++failures;
        }
    };
    constexpr std::uintptr_t module = 0x10000000, world = 0x200000, zone = 0x300000, payload = 0x900000;
    std::map<std::uintptr_t, std::vector<std::uint8_t>> memory;
    std::vector<std::pair<std::uintptr_t, std::size_t>> reads;
    const auto store = [&](std::size_t offset, auto value) {
        std::memcpy(memory[zone].data() + offset, &value, sizeof value);
    };
    const auto span = [&](std::uintptr_t begin, std::uintptr_t end, std::uintptr_t capacity) {
        store(0x518 + 0x40, std::array<std::uintptr_t, 3>{begin, end, capacity});
    };
    const auto fixture = [&] {
        memory.clear();
        reads.clear();
        // Unlisted fields deliberately contain nonzero bytes. Only the native
        // stored base/vector fields are initialized; no climate defaults exist.
        memory[zone].assign(0x58518, 0xcd);
        store(0, module + 0x4738c0);
        store(8, world);
        store(0x10, std::array<std::int32_t, 2>{-1, 2});
        for (std::size_t column = 0; column < 4096; ++column) {
            store(0x518 + column * 0x58 + 0x3c, std::int32_t(0));
            store(0x518 + column * 0x58 + 0x40, std::array<std::uintptr_t, 3>{});
        }
        store(0x518 + 0x3c, std::int32_t(-3));
        store(0x518 + 4095 * 0x58 + 0x3c, std::int32_t(-42));
        const std::array<std::uint8_t, 18> blocks{{7, 8, 9, 254, 0, 2, 7, 8, 9, 254, 0, 2, 1, 2, 3, 4, 5, 64}};
        memory[payload] = {blocks.begin(), blocks.end()};
        span(payload, payload + blocks.size(), payload + 24); // capacity's fourth record is deliberately unreadable
    };
    const auto read = [&](std::uintptr_t address, void *destination, std::size_t count) {
        reads.emplace_back(address, count);
        auto found = memory.upper_bound(address);
        if (found == memory.begin())
            return false;
        --found;
        const auto offset = address - found->first;
        if (offset > found->second.size() || count > found->second.size() - offset)
            return false;
        std::memcpy(destination, found->second.data() + offset, count);
        return true;
    };
    captureabi::ZoneTerrain terrain;
    const auto capture = [&](std::size_t budget = captureabi::MAX_ZONE_BLOCK_BYTES) {
        return captureabi::readZoneTerrain(module, world, zone, terrain, read, budget);
    };
    fixture();
    check(capture() && terrain.coordinates == std::array<std::int32_t, 2>{-1, 2} && terrain.columns.size() == 4096 &&
              terrain.columns.front().baseZ == -3 && terrain.columns.back().baseZ == -42 &&
              terrain.columns[0].blocks.size() == 3 && terrain.columns[0].blocks[0][3] == 254 &&
              terrain.columns[0].blocks[0][4] == 0 && terrain.columns[0].blocks[2][5] == 64,
          "signed whole-Zone identity, X-major columns and every stored byte are preserved without effective "
          "substitutions");
    if (terrain.columns.size() != 4096)
        return failures + 1;
    bool onlySpecifiedFields = true;
    for (const auto &[address, count] : reads)
        if (address >= zone && address < zone + 0x58518) {
            const auto offset = address - zone;
            const auto local = offset >= 0x518 ? (offset - 0x518) % 0x58 : offset;
            onlySpecifiedFields &= (offset == 0 && count == 24) || (offset >= 0x518 && local == 0x3c && count == 4) ||
                                   (offset >= 0x518 && local == 0x40 && count == 24);
        }
    check(onlySpecifiedFields, "uninitialized climate caches and capacity bytes are not read");
    const auto encoded = zonecapture::encode(terrain, 7);
    check(encoded.size() == 20 + 4096 * 8 + 16 && encoded[24] == 2 && encoded[28] == 2 && encoded[29] == 0 &&
              encoded[30] == 7 && encoded[33] == 254 && encoded[34] == 0 && encoded[35] == 2,
          "CWZ1 retains the separate biome label and compresses exact six-byte records");
    if (encoded.size() < 44)
        return failures + 1;
    // Exercise the existing engine consumer, not a second producer-side decoder.
    namespace fs = std::filesystem;
    const auto directory =
        fs::temp_directory_path() /
        ("CubeVulkan-terrain-storage-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    const bool created = fs::create_directory(directory, error);
    check(created, "create a fresh, test-owned codec directory");
    if (created) {
        const auto file = directory / "z_-1_2.cwz";
        {
            std::ofstream stream(file, std::ios::binary);
            stream.write(reinterpret_cast<const char *>(encoded.data()), std::streamsize(encoded.size()));
            check(bool(stream), "write the test-owned CWZ1 fixture");
        }
        std::string reason;
        check(ExtractedWorld::validateZoneFile(file, reason),
              "the actual engine accepts the captured terrain encoding");
        const auto stored = ExtractedWorld::readStoredZoneColumns(file, reason);
        check(stored.size() == 4096 && stored[0].base == terrain.columns[0].baseZ && stored[0].blocks.size() == 3 &&
                  std::memcmp(stored[0].blocks.data(), terrain.columns[0].blocks.data(), 18) == 0,
              "dungeon comparison reads exact stored bytes and column spans");
        terrain.columns.back().baseZ = INT32_MIN;
        const auto emptyBase = zonecapture::encode(terrain, 7);
        {
            std::ofstream stream(file, std::ios::binary | std::ios::trunc);
            stream.write(reinterpret_cast<const char *>(emptyBase.data()), std::streamsize(emptyBase.size()));
        }
        const auto raw = ExtractedWorld::readStoredZoneColumns(file, reason);
        check(raw.size() == 4096 && raw.back().empty() && raw.back().base == INT32_MIN &&
                  !ExtractedWorld::validateZoneFile(file, reason),
              "stored-only reader retains raw empty base without changing playable validation");
        fs::remove(file, error);
        fs::remove(directory, error); // remove only the empty directory created above, never recursively
    }
    terrain.columns[0].blocks.assign(65536, captureabi::StoredBlock{1, 2, 3, 4, 5, 6});
    const auto longRun = zonecapture::encode(terrain, 7);
    check(longRun.size() == encoded.size() && longRun[24] == 2 && longRun[28] == 255 && longRun[29] == 255 &&
              longRun[36] == 1 && longRun[37] == 0,
          "identical runs split at 65535 instead of wrapping the uint16 length");
    fixture();
    store(0x518 + 0x58 + 0x40, std::array<std::uintptr_t, 3>{payload + 24, payload + 24, payload + 30});
    check(capture() && terrain.columns[1].blocks.empty(),
          "allocated-empty vectors do not turn capacity into live blocks");
    store(0x518 + 4095 * 0x58 + 0x3c, std::numeric_limits<std::int32_t>::min());
    check(capture() && terrain.columns.back().baseZ == std::numeric_limits<std::int32_t>::min(),
          "primitive capture does not manufacture a different base height for engine coordinate limits");
    fixture();
    check(!capture(17) && terrain.columns.empty(),
          "aggregate resource budget rejects before publishing a partial Zone");
    for (const auto bad :
         {std::array<std::uintptr_t, 3>{payload, payload - 6, payload},
          std::array<std::uintptr_t, 3>{payload, payload + 7, payload + 12},
          std::array<std::uintptr_t, 3>{payload, payload + 12, payload + 6}, std::array<std::uintptr_t, 3>{0, 0, 6},
          std::array<std::uintptr_t, 3>{payload, payload + 131073 * 6, payload + 131073 * 6}}) {
        fixture();
        span(bad[0], bad[1], bad[2]);
        check(!capture() && terrain.columns.empty(),
              "malformed/misaligned/excessive spans cannot publish stale terrain");
    }
    fixture();
    memory.erase(payload);
    check(!capture() && terrain.columns.empty(), "unreadable live block storage fails through the guarded reader");
    fixture();
    const auto mutating = [&](std::uintptr_t address, void *destination, std::size_t count) {
        const bool copied = read(address, destination, count);
        if (address == payload)
            store(0x518 + 0x3c, std::int32_t(9));
        return copied;
    };
    check(!captureabi::readZoneTerrain(module, world, zone, terrain, mutating) && terrain.columns.empty(),
          "observed base/vector replacement rejects the copied payload");
    fixture();
    unsigned identities = 0;
    const auto replacing = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == zone && ++identities == 2)
            store(0x10, std::array<std::int32_t, 2>{3, 4});
        return read(address, destination, count);
    };
    check(!captureabi::readZoneTerrain(module, world, zone, terrain, replacing) && terrain.columns.empty(),
          "changed Zone identity rejects the complete result");
    for (const auto offset : {0u, 8u}) {
        fixture();
        store(offset, std::uintptr_t(0));
        check(!capture(), "unknown concrete Zone type or owning World is not traversed");
    }
    fixture();
    captureabi::StoredColumn column;
    reads.clear();
    check(!captureabi::readStoredColumn(zone, 4096, column, read) && reads.empty(),
          "column indices cannot leave the in-place array");
    check(!captureabi::readZoneTerrain(module, world, std::numeric_limits<std::uintptr_t>::max(), terrain, read),
          "Zone header spans cannot wrap");
    check(zonecapture::encode({}, 7).empty(), "an incomplete terrain DTO has no serialized payload");
    std::cout << "Native Zone terrain: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
