#include "../tools/ZoneExtractor/NativeLoadedZones.h"
#include <cstring>
#include <iostream>
#include <map>

int nativeLoadedZoneTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL native loaded Zones: " << label << '\n';
            ++failures;
        }
    };
    constexpr std::uintptr_t world = 0x100000, sentinel = 0x200000, a = 0x700000, b = 0x400000;
    constexpr std::uintptr_t za = 0x900000, zb = 0x600000;
    std::map<std::uintptr_t, std::vector<std::uint8_t>> memory;
    std::vector<std::uintptr_t> reads;
    const auto store = [&](std::uintptr_t address, auto value) {
        auto &bytes = memory[address];
        bytes.resize(sizeof value);
        std::memcpy(bytes.data(), &value, sizeof value);
    };
    const auto read = [&](std::uintptr_t address, void *destination, std::size_t count) {
        reads.push_back(address);
        const auto found = memory.find(address);
        if (found == memory.end() || found->second.size() != count)
            return false;
        std::memcpy(destination, found->second.data(), count);
        return true;
    };
    const auto fixture = [&] {
        memory.clear();
        reads.clear();
        store(world + 0x468, std::array<std::uintptr_t, 2>{sentinel, 2});
        store(sentinel, std::array<std::uintptr_t, 2>{a, b});
        store(a, std::array<std::uintptr_t, 2>{b, sentinel});
        store(b, std::array<std::uintptr_t, 2>{sentinel, a});
        store(a + 0x18, za);
        store(b + 0x18, zb);
    };
    std::vector<std::uintptr_t> output;
    const auto capture = [&] { return captureabi::readLoadedZones(world, output, read); };
    fixture();
    check(capture() && output == std::vector<std::uintptr_t>{za, zb},
          "noncontiguous nodes preserve native forward order without reading keys or buckets");
    store(a + 0x18, std::uintptr_t(0));
    check(capture() && output == std::vector<std::uintptr_t>{zb},
          "entry count includes null Zone values while consumers skip their payloads");
    fixture();
    store(world + 0x468, std::array<std::uintptr_t, 2>{sentinel, 0});
    store(sentinel, std::array<std::uintptr_t, 2>{sentinel, sentinel});
    check(capture() && output.empty(), "the empty sentinel is not interpreted as a Zone");
    fixture();
    store(b, std::array<std::uintptr_t, 2>{a, a});
    check(!capture() && output.empty(), "cycles cannot become an unbounded capture loop");
    fixture();
    store(b, std::array<std::uintptr_t, 2>{sentinel, sentinel});
    check(!capture(), "a broken backward link rejects the sequence");
    fixture();
    store(sentinel, std::array<std::uintptr_t, 2>{a, a});
    check(!capture(), "the stored tail must match the traversed tail");
    for (const auto count : {std::uintptr_t(1), std::uintptr_t(3), std::uintptr_t(captureabi::MAX_LOADED_ZONES + 1)}) {
        fixture();
        store(world + 0x468, std::array<std::uintptr_t, 2>{sentinel, count});
        check(!capture() && output.empty(), "inconsistent or excessive entry counts publish no partial list");
    }
    fixture();
    memory.erase(b + 0x18);
    output.push_back(za);
    check(!capture() && output.empty(), "an inaccessible later payload discards stale and partial output");
    fixture();
    unsigned headerReads = 0;
    const auto replacingRead = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == world + 0x468 && ++headerReads == 2)
            store(world + 0x468, std::array<std::uintptr_t, 2>{sentinel + 0x1000, 2});
        return read(address, destination, count);
    };
    check(!captureabi::readLoadedZones(world, output, replacingRead) && output.empty(),
          "container replacement is rejected before publication");
    fixture();
    const auto maximum = std::numeric_limits<std::uintptr_t>::max();
    reads.clear();
    check(!captureabi::readLoadedZones(maximum, output, read) && reads.empty(), "World field spans cannot wrap");
    store(world + 0x468, std::array<std::uintptr_t, 2>{maximum - 7, 0});
    check(!capture(), "the full sentinel link span must be readable");
    std::cout << "Native loaded Zones: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
