#pragma once
// Independent layouts: docs/specs/steam-world-creature-views.md sections2–6.
#include "CaptureMemory.h"
#include <array>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

namespace captureabi {
constexpr std::size_t WORLD_ACTOR_SECTION = 0x3c8;
constexpr std::size_t MAX_CAPTURE_CREATURES = 262144; // capture policy, not a native count limit

template <class Read> bool readClientWorld(std::uintptr_t game, std::uintptr_t &world, Read read) {
    world = 0;
    std::uintptr_t value = 0;
    if (!readField(game, 8, &value, sizeof value, read))
        return false;
    world = value;
    return true;
}
template <class Read> bool readLocalCreature(std::uintptr_t world, std::uintptr_t &creature, Read read) {
    creature = 0;
    std::uintptr_t value = 0;
    if (!readField(world, 0x448, &value, sizeof value, read))
        return false;
    creature = value;
    return true;
}

// Caller retains World/node/creature lifetimes and the applicable native guard
// through this traversal AND later use of borrowed addresses. Metadata rechecks
// do not pin allocations or make nested Creature fields an atomic snapshot.
template <class Read>
bool readRegisteredCreatures(std::uintptr_t world, std::vector<std::uintptr_t> &output, Read read) {
    output.clear();
    std::array<std::uintptr_t, 2> header{}, ends{};
    if (!readField(world, 0x40, header.data(), sizeof header, read) || !header[0] ||
        header[1] > MAX_CAPTURE_CREATURES || !readField(header[0], 0, ends.data(), sizeof ends, read))
        return false;
    std::vector<std::uintptr_t> result;
    result.reserve(header[1]);
    auto node = ends[0], previous = header[0];
    std::size_t visited = 0;
    while (node != header[0]) {
        std::array<std::uintptr_t, 3> entry{};
        if (!node || visited >= header[1] || !readField(node, 0, entry.data(), sizeof entry, read) ||
            entry[1] != previous)
            return false;
        result.push_back(entry[2]); // preserve order, duplicate values and null payloads; consumer policy is separate
        previous = node;
        node = entry[0];
        ++visited;
    }
    if (visited != header[1] || ends[1] != previous)
        return false;
    std::array<std::uintptr_t, 2> currentHeader{}, currentEnds{};
    if (!readField(world, 0x40, currentHeader.data(), sizeof currentHeader, read) || currentHeader != header ||
        !readField(header[0], 0, currentEnds.data(), sizeof currentEnds, read) || currentEnds != ends)
        return false;
    output = std::move(result);
    return true;
}

struct CreatureLookup {
    bool present = false;
    std::uintptr_t creature = 0; // native0xe720 returns0 for both absent and present-null
};

template <class Read> bool lookupCreature(std::uintptr_t world, std::int64_t id, CreatureLookup &output, Read read) {
    output = {};
    std::array<std::uintptr_t, 2> wrapper{};
    std::array<std::uint8_t, 0x1a> header{}; // never read the sentinel's uninitialized key/value
    if (!readField(world, 0x450, wrapper.data(), sizeof wrapper, read) || !wrapper[0] ||
        wrapper[1] > MAX_CAPTURE_CREATURES || !readField(wrapper[0], 0, header.data(), header.size(), read) ||
        header[0x18] != 1 || header[0x19] != 1)
        return false;
    const auto link = [](const auto &bytes, std::size_t offset) {
        std::uintptr_t value;
        std::memcpy(&value, bytes.data() + offset, sizeof value);
        return value;
    };
    const auto minimum = link(header, 0), root = link(header, 8), maximum = link(header, 16);
    if (!wrapper[1]) {
        if (minimum != wrapper[0] || root != wrapper[0] || maximum != wrapper[0])
            return false;
    } else if (!minimum || !root || !maximum || minimum == wrapper[0] || root == wrapper[0] || maximum == wrapper[0])
        return false;
    std::optional<std::int64_t> lower, upper;
    auto node = root, parent = wrapper[0];
    CreatureLookup result;
    std::size_t visited = 0;
    while (node != wrapper[0]) {
        std::array<std::uint8_t, 0x1a> entry{};
        std::int64_t key;
        std::uintptr_t value;
        // The additional256 path bound is capture policy; valid balanced trees
        // within the count budget need far fewer steps.
        if (!node || visited++ >= wrapper[1] || visited > 256 ||
            !readField(node, 0, entry.data(), entry.size(), read) || entry[0x19] || entry[0x18] > 1 ||
            link(entry, 8) != parent || !readField(node, 0x20, &key, sizeof key, read) ||
            !readField(node, 0x28, &value, sizeof value, read) || (lower && key <= *lower) || (upper && key >= *upper))
            return false;
        if (key == id) {
            result = {true, value};
            break;
        }
        parent = node;
        if (id < key) {
            upper = key;
            node = link(entry, 0);
        } else {
            lower = key;
            node = link(entry, 16);
        }
    }
    std::array<std::uintptr_t, 2> currentWrapper{};
    std::array<std::uint8_t, 0x1a> currentHeader{};
    if (!readField(world, 0x450, currentWrapper.data(), sizeof currentWrapper, read) || currentWrapper != wrapper ||
        !readField(wrapper[0], 0, currentHeader.data(), currentHeader.size(), read) || currentHeader != header)
        return false;
    output = result;
    return true;
}
} // namespace captureabi
