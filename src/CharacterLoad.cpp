#include "CharacterLoad.h"
#include "NativeProgress.h"
#include <algorithm>
#include <cstring>

namespace characterload {
namespace {
template <class T> void put(worldgen::CreatureRecord &record, size_t at, const T &value) {
    std::memcpy(record.data() + at, &value, sizeof value);
}
struct Range { size_t at, size; };
void clearItem(uint8_t *item) {
    // Empty defined fields, retaining all eight padding bytes.
    for (const Range range : {Range{0,1}, {4,17}, {0x18,4}, {0x1c,130}})
        std::memset(item + range.at, 0, range.size);
}
void copyAppearance(worldgen::CreatureRecord &record, const uint8_t *appearance) {
    std::memcpy(record.data() + 0x78, appearance, 5);
    std::memcpy(record.data() + 0x80, appearance + 8, 0xa8);
}
void resetForLoad(worldgen::CreatureRecord &record, nativeai::Creature &side) {
    // This selected decoder's defaults are proved, including its temporary's
    // zero home-position slots. Preserve the omitted widths instead of assigning
    // a whole creature or reconstructing containers that survive this reset.
    static const auto defaults = worldgen::newCreature(0);
    for (const Range range : {Range{0x10,0x51}, {0x64,5}, {0x6c,0x11}, {0x80,0xaa},
                              {0x12c,0x22}, {0x150,0x4e}, {0x1a0,0x38},
                              {0x958,0x19}, {0x974,0xc}})
        std::memcpy(record.data() + range.at, defaults.data() + range.at, range.size);
    // Equipment padding was already cleared by the preceding native reset.
    std::memset(record.data() + 0x278, 0, 11 * nativesaves::ITEM_SIZE);
    clearItem(record.data() + 0x1d8);
    record[0x60] = 0;
    for (const Range range : {Range{0x9d8,8}, {0x9e0,8}, {0xad8,8}, {0x1544,4},
                              {0xbc0,8}, {0xbc8,4}, {0x994,8}, {0xb58,4}, {0x99c,4},
                              {0xbb0,8}, {0x9e8,4}, {0xba8,4}, {0xbb8,1}, {0xc18,8}})
        std::memset(record.data() + range.at, 0, range.size);
    put(record, 0x9a0, 1.0f);
    uint64_t begin;
    std::memcpy(&begin, record.data() + 0xc20, sizeof begin);
    put(record, 0xc28, begin); // logically empty range; retain allocation/capacity slots

    side.interactions.clear();
    side.aggro.clear(); side.damage.clear(); side.cooldowns.clear();
    side.inventory.clear(); side.buffs.clear();
    // counted/struck, skills/summon ranges and group belong to separate lifetimes.
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
        copyAppearance(record, appearance.data());
    std::memcpy(record.data() + 0x1d0, saved.homeCell.data(), 8);

    put(record, 0xa08, int32_t(0));
    clearItem(record.data() + 0xa0c);
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
