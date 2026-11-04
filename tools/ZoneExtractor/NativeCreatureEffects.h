#pragma once
// Original layout/lifecycle: steam-creature-core-and-effects.md.
#include "CaptureMemory.h"
#include <array>
#include <cstring>
#include <utility>
#include <vector>

namespace captureabi {
struct CreatureEffect {
    std::array<uint8_t, 24> bytes;
    template <class T> T value(size_t offset) const {
        T result;
        std::memcpy(&result, bytes.data() + offset, sizeof result);
        return result;
    }
    uint8_t kind() const { return bytes[0]; }
    float magnitude() const { return value<float>(4); }
    float time() const { return value<float>(8); }
    int64_t source() const { return value<int64_t>(16); }
};
constexpr size_t MAX_CAPTURE_EFFECTS = 16384; // Tool budget, not native capacity.

// Caller retains Creature lifetime and the observed applicable guard through
// copying. Rechecks do not make replacement-in-place payload writes atomic.
template <class Read> bool readCreatureEffects(uintptr_t creature, std::vector<CreatureEffect> &output, Read read) {
    output.clear();
    std::array<uintptr_t, 2> header, ends;
    if (!readField(creature, 0x980, header.data(), 16, read) || !header[0] || header[1] > MAX_CAPTURE_EFFECTS ||
        !readField(header[0], 0, ends.data(), 16, read))
        return false;
    const auto sentinel = header[0], count = header[1];
    std::vector<CreatureEffect> effects;
    effects.reserve(count);
    auto previous = sentinel, node = ends[0];
    size_t visited = 0;
    while (node != sentinel) {
        if (!node || visited >= count)
            return false;
        std::array<uint8_t, 40> stored;
        if (!readField(node, 0, stored.data(), stored.size(), read))
            return false;
        std::array<uintptr_t, 2> links;
        std::memcpy(links.data(), stored.data(), 16);
        if (links[1] != previous)
            return false;
        CreatureEffect effect;
        std::memcpy(effect.bytes.data(), stored.data() + 16, effect.bytes.size());
        effects.push_back(effect);
        previous = node;
        node = links[0];
        ++visited;
    }
    if (visited != count || previous != ends[1])
        return false;
    std::array<uintptr_t, 2> currentHeader, currentEnds;
    if (!readField(creature, 0x980, currentHeader.data(), 16, read) || currentHeader != header ||
        !readField(sentinel, 0, currentEnds.data(), 16, read) || currentEnds != ends)
        return false;
    output = std::move(effects);
    return true;
}
} // namespace captureabi
