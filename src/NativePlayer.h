#pragma once
// The player as the game's rules see it: a game-layout record (worldgen::newCreature)
// with its race, class, specialisation, level, appearance flags, the region it stands
// in (+0x150) and what it wears (+0x278, 11 items of 0xa0 bytes), so its numbers come
// from the game's own functions (NativeRules): maximum health 0x5fac0, attack 0x4fa70,
// spell 0x65c70, armor 0x4f080, resistance 0x64d90, critical chance 0x50d30, haste
// 0x66460. Checked against the game's numbers for the live player (EngineTests
// playerstats, Extract/oracle/stats-*.bin).
//
// In the Steam game an item counts fully only in the region it comes from (or next to
// it, for some): elsewhere it counts as rarity -1. So the numbers change as the player
// moves between regions.
#include "worldgen/Residents.h"

#include <array>
#include <cstdint>
#include <filesystem>

namespace nativeplayer {

using Item = std::array<uint8_t, 0xa0>;

// A game item: category +0, id +4, modifier +8, region +0xc, rarity +0x14, formula +0x18,
// material +0x1c.
Item item(int category, int id, int material, int rarity, uint32_t modifier, int regionX, int regionY, int formula = 0);

struct Who {
    int race = 2, cls = 0, spec = 0, level = 1;
    uint32_t flags2 = 0;
    int32_t region[2] = {0, 0}; // where it stands (0x2ada20 of its block)
};
worldgen::CreatureRecord record(const Who &who, const std::array<Item, 11> &worn);

struct Stats {
    float maxHp = 0, attack = 0, spell = 0, armor = 0, resistance = 0, critical = 0, haste = 0;
};
// Without buffs.
Stats stats(const worldgen::CreatureRecord &r);

// The character's appearance (+0x78, 0xb0 bytes) from what ZoneExtractor read of the
// owner's character (Extract/characters/<name>/snapshot.txt): hair colour, flags (+0x80),
// scales, body-part models, their scales, turns and places. False without the file.
bool appearance(worldgen::CreatureRecord &r, const std::filesystem::path &snapshot);

// A generated item (category 30: treasure lying about or in chests) as a player of class
// `cls` finds it (the class-adapted item): by xorshift draws of its modifier, a weapon or one
// of four armour pieces of the class's kinds, a ring or an amulet; id 1 is a gold bag
// (category 31). Anything else is itself.
Item transformed(const Item &in, int cls);

} // namespace nativeplayer
