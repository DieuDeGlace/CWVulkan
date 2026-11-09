#include "CharacterState.h"
#include <algorithm>
#include <cstring>

namespace characterstate {
namespace {
struct Range {
    size_t at, size;
};
template <class T> void put(worldgen::CreatureRecord &record, size_t at, const T &value) {
    std::memcpy(record.data() + at, &value, sizeof value);
}
} // namespace
void copyItem(uint8_t *to, const uint8_t *from) {
    for (const Range range : {Range{0, 1}, {4, 17}, {0x18, 4}, {0x1c, 130}})
        std::memcpy(to + range.at, from + range.at, range.size);
}
void copyAppearance(uint8_t *to, const uint8_t *from) {
    std::memcpy(to, from, 5);
    std::memcpy(to + 8, from + 8, 0xa8);
}
void copyCore(uint8_t *to, const uint8_t *from) {
    for (const Range range : {Range{0x10, 0x51},
                              {0x64, 5},
                              {0x6c, 0x11},
                              {0x80, 0xaa},
                              {0x12c, 0x22},
                              {0x150, 0x4e},
                              {0x1a0, 0x38},
                              {0x958, 0x19},
                              {0x974, 0xc}})
        std::memcpy(to + range.at, from + range.at, range.size);
    for (size_t at = 0x1d8; at < 0x958; at += nativesaves::ITEM_SIZE)
        copyItem(to + at, from + at);
}
void reset(worldgen::CreatureRecord &record, nativeai::Creature &side) {
    for (const Range range :
         {Range{0x34, 0x24}, {0x68, 1},     {0x6c, 4},      {0x128, 2},    {0x70, 8},     {0x158, 4},
          {0x19c, 2},        {0x134, 0x10}, {0x278, 0x6e0}, {0x184, 0x10}, {0x1a0, 0x10}, {0x15c, 0x18},
          {0x958, 0x18},     {0x970, 1},    {0x974, 4},     {0x978, 8},    {0x9e0, 8},    {0x9d8, 8},
          {0xad8, 8},        {0x1544, 4},   {0xbc0, 8},     {0xbc8, 4},    {0x994, 8},    {0xb58, 4},
          {0x99c, 4},        {0xbb0, 8},    {0x9e8, 4},     {0xba8, 4},    {0xbb8, 1},    {0xc18, 8}})
        std::memset(record.data() + range.at, 0, range.size);
    put(record, 0x130, -3000.0f);
    put(record, 0x194, 1.0f);
    put(record, 0x198, 10.0f);
    put(record, 0x1b0, int32_t(-1));
    put(record, 0x1b4, int32_t(-1));
    put(record, 0x1d0, int32_t(-1));
    put(record, 0x1d4, int32_t(-1));
    put(record, 0x144, int32_t(1));
    put(record, 0x148, int32_t(0));
    put(record, 0x14c, uint16_t(0));
    put(record, 0x9a0, 1.0f);
    static const auto defaults = worldgen::newCreature(0);
    copyAppearance(record.data() + 0x78, defaults.data() + 0x78);
    uint64_t begin;
    std::memcpy(&begin, record.data() + 0xc20, 8);
    put(record, 0xc28, begin);
    side.interactions.clear();
    side.aggro.clear();
    side.damage.clear();
    side.cooldowns.clear();
    side.inventory.clear();
    side.buffs.clear();
    // Identity, separate sets/ranges, active item, current position/HP and omitted bytes survive.
}
void capture(const uint8_t *record, const nativeai::Creature &side, nativesaves::Character &out) {
    std::memcpy(out.position.data(), record + 0x10, 24);
    std::memcpy(out.orientation.data(), record + 0x28, 12);
    std::memcpy(&out.health, record + 0x180, 4);
    std::memcpy(&out.level, record + 0x144, 4);
    std::memcpy(&out.experience, record + 0x148, 4);
    out.classType = record[0x14c];
    out.specialization = record[0x14d];
    std::memcpy(&out.field9a4, record + 0x9a4, 4);
    std::memcpy(&out.field9a8, record + 0x9a8, 4);
    std::memcpy(out.equipment.data(), record + 0x278, 11 * nativesaves::ITEM_SIZE);
    const uint8_t *name = record + 0x958;
    out.name.assign(name, std::find(name, name + 16, uint8_t(0)));
    std::memcpy(out.homeCell.data(), record + 0x1d0, 8);
    out.buffs.clear();
    out.buffs.reserve(side.buffs.size());
    for (const auto &buff : side.buffs)
        out.buffs.push_back({buff});
}
} // namespace characterstate
