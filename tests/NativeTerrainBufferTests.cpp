#include "../tools/ZoneExtractor/NativeTerrainBuffers.h"
#include <cstring>
#include <iostream>
#include <map>

int nativeTerrainBufferTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL native terrain buffers: " << label << '\n';
            ++failures;
        }
    };
    constexpr std::uintptr_t module = 0x10000000, zone = 0x200000, sentinel = 0x300000;
    constexpr std::uintptr_t a = 0x700000, b = 0x400000, ga = 0x900000, gb = 0x600000;
    constexpr std::uintptr_t wrapper = 0xa00000, context = 0xb00000, buffer = 0xc00000;
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
        store(zone + 0x1c8, std::array<std::uintptr_t, 2>{sentinel, 2});
        store(sentinel, std::array<std::uintptr_t, 2>{a, b});
        store(a, std::array<std::uintptr_t, 3>{b, sentinel, ga});
        store(b, std::array<std::uintptr_t, 3>{sentinel, a, gb});
        store(ga + 8, wrapper);
        store(gb + 8, std::uintptr_t(0));
        store(wrapper, module + captureabi::VERTEX_BUFFER_TABLE_RVA);
        store(wrapper + 8, std::array<std::uintptr_t, 2>{context, buffer});
    };
    std::vector<captureabi::VertexBufferView> output;
    const auto capture = [&] { return captureabi::readTerrainVertexBuffers(module, zone, output, read); };
    fixture();
    check(capture() && output.size() == 2 && output[0].wrapper == wrapper && output[0].context == context &&
              output[0].buffer == buffer && !output[1].wrapper,
          "noncontiguous links preserve group order and expose the actual shared vertex buffer");
    // No pending-list, sentinel payload, sprite or CPU mesh fields are present.
    fixture();
    store(zone + 0x1c8, std::array<std::uintptr_t, 2>{sentinel, 0});
    store(sentinel, std::array<std::uintptr_t, 2>{sentinel, sentinel});
    check(capture() && output.empty(), "a self-linked empty active list is valid without pending data");
    fixture();
    store(b, std::array<std::uintptr_t, 3>{a, a, gb});
    check(!capture() && output.empty(), "a cycle away from the sentinel is bounded and rejected");
    fixture();
    store(b, std::array<std::uintptr_t, 3>{sentinel, sentinel, gb});
    check(!capture(), "a broken backward link is rejected");
    fixture();
    store(sentinel, std::array<std::uintptr_t, 2>{a, a});
    check(!capture(), "the sentinel tail must match the traversed last node");
    fixture();
    store(zone + 0x1c8, std::array<std::uintptr_t, 2>{sentinel, 1});
    check(!capture(), "a short count cannot publish a partial linked sequence");
    fixture();
    store(zone + 0x1c8, std::array<std::uintptr_t, 2>{sentinel, 3});
    check(!capture(), "early sentinel termination must agree with the native count");
    fixture();
    store(zone + 0x1c8, std::array<std::uintptr_t, 2>{sentinel, captureabi::MAX_TERRAIN_GROUPS + 1});
    check(!capture() && reads.size() == 1, "oversized counts are rejected before allocation or traversal");
    fixture();
    memory.erase(gb + 8);
    output.push_back({wrapper, context, buffer});
    check(!capture() && output.empty(), "an unreadable later group discards stale and partial output");
    fixture();
    store(wrapper, std::uintptr_t(0));
    check(!capture(), "unproved buffer implementations are rejected");
    fixture();
    unsigned headerReads = 0;
    const auto changingRead = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == zone + 0x1c8 && ++headerReads == 2)
            store(zone + 0x1c8, std::array<std::uintptr_t, 2>{sentinel, 0});
        return read(address, destination, count);
    };
    check(!captureabi::readTerrainVertexBuffers(module, zone, output, changingRead) && output.empty(),
          "changed active publication is rejected before returning identities");
    fixture();
    unsigned groupReads = 0;
    const auto remeshingRead = [&](std::uintptr_t address, void *destination, std::size_t count) {
        if (address == ga + 8 && ++groupReads == 2)
            store(ga + 8, std::uintptr_t(0));
        return read(address, destination, count);
    };
    check(!captureabi::readTerrainVertexBuffers(module, zone, output, remeshingRead) && output.empty(),
          "a changed group resource cannot be paired with its predecessor");
    fixture();
    const auto maximum = std::numeric_limits<std::uintptr_t>::max();
    check(!captureabi::readTerrainVertexBuffers(module, maximum, output, read), "Zone field arithmetic cannot wrap");
    fixture();
    store(zone + 0x1c8, std::array<std::uintptr_t, 2>{maximum - 7, 0});
    check(!capture(), "the complete sentinel header must fit the address space");
    std::cout << "Native terrain buffers: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
