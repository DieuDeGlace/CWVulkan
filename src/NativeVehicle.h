#pragma once
// Derived CharacterData+0x148 item: docs/specs/steam-selected-vehicle.md.
#include "NativeSaves.h"
#include <functional>
#include <optional>
#include <span>

namespace nativevehicle {
using Tabs = std::vector<std::vector<nativesaves::Stack>>;
using BlockPosition = std::array<int32_t, 3>;
struct Environment {
    uint8_t *player = nullptr;
    bool namedWorld = false;
    const Tabs *inventory = nullptr;
    std::function<float()> wholeDuration; // signed32 native result already converted to float32
    std::function<std::span<const nativesaves::Record>(nativesaves::Cell)> loadedObjects;
    // An unavailable provider defers new travel selection without erasing an
    // active provisional item. The World adapter includes its verified fallback.
    std::function<std::optional<uint8_t>(BlockPosition)> blockFlags;
};

bool ownsSpecialItem(const uint8_t *player, const Tabs *, int32_t subtype);
bool nearbyObject(const Environment &, int32_t kind, bool requireState);
// Runs at the admitted World slice before ordinary creature traversal. A skipped
// slice retains the cache; cancellation writes the ability byte only.
void rebuild(nativesaves::Item &selection, const Environment &);
} // namespace nativevehicle
