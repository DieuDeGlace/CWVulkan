#include "CharacterLoad.h"
#include "CharacterState.h"
#include "NativeProgress.h"
#include <algorithm>
#include <cstring>

namespace characterload {
namespace {
template <class T> void put(worldgen::CreatureRecord &record, size_t at, const T &value) {
    std::memcpy(record.data() + at, &value, sizeof value);
}
void resetForLoad(worldgen::CreatureRecord &record, nativeai::Creature &side) {
    characterstate::reset(record, side);
    // The decoder's default-core assignment is distinct from reset-alone.
    static const auto defaults = worldgen::newCreature(0);
    characterstate::copyCore(record.data(), defaults.data());
    record[0x60] = 0;
}
}
void apply(worldgen::CreatureRecord &record, nativeai::Creature &side, const nativesaves::Character &saved) {
    resetForLoad(record, side);
    std::memcpy(record.data() + 0x10, saved.position.data(), 24);
    // The live decoder consumes the stored orientation and then zeroes it.
    std::memset(record.data() + 0x28, 0, 12);
    put(record, 0x180, saved.health);
    put(record, 0x144, saved.level);
    put(record, 0x148, saved.experience);
    record[0x14c] = saved.classType; record[0x14d] = saved.specialization;
    put(record, 0x9a4, saved.field9a4); put(record, 0x9a8, saved.field9a8);
    std::memcpy(record.data() + 0x278, saved.equipment.data(), 11 * nativesaves::ITEM_SIZE);
    // Bounded name storage is the engine's safe policy for overlong records.
    std::memcpy(record.data() + 0x958, saved.name.data(), std::min(saved.name.size(), size_t(15)));
    if (const int race = nativesaves::raceOf(saved.looks.people(), saved.looks.sex()); race >= 0)
        put(record, worldgen::creature::RACE, race);
    std::array<uint8_t, 0xb0> appearance{};
    if (nativeprogress::appearanceOf(saved.looks, appearance))
        characterstate::copyAppearance(record.data() + 0x78, appearance.data());
    std::memcpy(record.data() + 0x1d0, saved.homeCell.data(), 8);

    put(record, 0xa08, int32_t(0));
    const nativesaves::Item empty{};
    characterstate::copyItem(record.data() + 0xa0c, empty.data());
    put(record, 0xaac, saved.gold); put(record, 0xab0, saved.fieldAb0);
    // Local gameplay keeps its live canonical source. Standalone/NPC adapters
    // retain their byte-record inventory without creating a second player copy.
    if (!side.inventorySource) {
        side.inventory.resize(saved.inventory.size());
        for (size_t tab = 0; tab < saved.inventory.size(); ++tab) {
            auto &destination = side.inventory[tab];
            destination.resize(saved.inventory[tab].size());
            for (size_t slot = 0; slot < destination.size(); ++slot) {
                const auto &stack = saved.inventory[tab][slot];
                std::memcpy(destination[slot].data(), &stack.count, 4);
                std::memcpy(destination[slot].data() + 4, stack.item.data(), stack.item.size());
            }
        }
    }
    side.buffs.reserve(saved.buffs.size());
    for (const auto &buff : saved.buffs) side.buffs.push_back(buff.bytes);
}
} // namespace characterload
