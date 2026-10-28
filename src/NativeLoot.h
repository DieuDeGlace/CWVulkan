#pragma once
// Independently expressed selection facts from the supported Steam build's
// drop routine (RVA 2a6860). This covers worn/pet/generated-gear decisions only;
// generated item contents, resources and inventory drops remain unfinished.
#include "Creatures.h"
#include <array>
#include <cstddef>

namespace nativeLoot {
struct Selection {
    size_t wornSlot = SIZE_MAX;
    bool pet = false, generated = false;
    bool legacyFlags = false;
};

// next() supplies an integer from 0..32767, the native rand() domain. Keeping
// modulo here retains the native 1/20 and 1/50 remainder bias. The engine's
// independent random sequence is not claimed to match the native sequence.
template <class Next> Selection select(const CreatureRecordings::Creature &creature, Next &&next) {
    Selection out;
    out.legacyFlags = !creature.flags2.has_value();
    uint32_t flags = creature.flags2.value_or(0); // explicitly reported legacy approximation
    if (creature.hostility == 0 || creature.hostility == 3 || (flags & 0x800)) return out;
    // Native insertion order: left weapon, right weapon, chest, shoulder,
    // hands, feet, neck. Preserve it for a given selection remainder.
    std::array<size_t, 7> slots{};
    size_t count = 0;
    for (size_t slot : {size_t(6), size_t(7), size_t(2), size_t(5), size_t(4), size_t(3), size_t(1)}) {
        if (slot >= creature.worn.size()) continue;
        const auto &item = creature.worn[slot];
        if (item.category && !(slot == 2 && item.id == 1) &&
            !(slot >= 6 && (item.id == 22 || item.id == 23))) slots[count++] = slot;
    }
    if (count && ((next() % 16 == 0) || (flags & 0x200))) out.wornSlot = slots[next() % count];
    if (creature.worn.size() > 10 && creature.worn[10].category && next() % 50 == 0) out.pet = true;
    if (creature.hostility == 1) out.generated = (next() % 20 == 0) || (flags & 0x40200);
    else if (creature.hostility == 7) out.generated = next() % 20 == 0;
    return out;
}
} // namespace nativeLoot
