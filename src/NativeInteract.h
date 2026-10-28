#pragma once
// What a creature does with the interactions it was handed (0x2c3710, after its ability
// step in the world's update; only on the server). Each one has a kind (+0xc4); every
// record is looked at once, front to back, and then the whole list is emptied, whether a
// record did anything or not (docs/specs/zone-object-interaction.md §1).
//
//   * 1  an item used from the inventory: only a bomb does anything here (a bomb creature).
//   * 2  the gate opener (item 0x18:5): wakes the gate keepers nearby and opens gates.
//   * 3  activating a creature: a lever or lift, the arena master's next stage, or the
//        thanks of a resident whose task is done (a reward, then what it carries).
//   * 5  using a zone object: sitting or lying down, opening and shutting doors, handing
//        an object the item it takes (a locked door, a chest, a brazier), and sleeping at
//        the innkeeper's at night.
//   * 7  picking up a ground item, 8 dropping one, 10 calling the pet.
//   * 11 the rangers' shadow clone, 12 the mages' water bubble, 13 the warriors' heroic
//        shout (docs/specs/skills-and-interactions.md).
#include "NativeAI.h"
#include "NativeBuffs.h"

#include <array>
#include <cstdint>
#include <functional>
#include <set>
#include <utility>
#include <vector>

namespace nativeinteract {

// The interaction record (nativeai::Interaction, 0xc8 bytes) by its offsets. A creature's
// list node holds it 0x10 bytes in, after the list's two pointers.
constexpr size_t ITEM = 0x00;       // an item (0xa0 bytes: type +0, subtype i32 +4, ...)
constexpr size_t ZONE_X = 0xa0;     // i32 (or -1)
constexpr size_t ZONE_Y = 0xa4;     // i32 (or -1)
constexpr size_t OBJECT = 0xa8;     // i32: an object's (or ground item's) index in that zone (or -1)
constexpr size_t TARGET = 0xb0;     // i64: a creature's id (0 if none)
constexpr size_t KIND = 0xc4;       // byte
constexpr size_t ALWAYS_ONE = 0xc6; // i16, 1 in every new record

enum Kind : uint8_t {
    USE_ITEM = 1,
    GATE_OPENER = 2,
    ACTIVATE = 3,
    USE_OBJECT = 5,
    PICK_UP = 7,
    DROP = 8,
    CALL_PET = 10,
    SHADOW_CLONE = 11,
    WATER_BUBBLE = 12,
    HEROIC_SHOUT = 13,
};

// 0x9660: a new record of `kind` with no item, no zone or object (-1) and no target.
nativeai::Interaction fresh(uint8_t kind);

// A zone object (zone +0x60, 0xe0 bytes; nativeai::Object) by its offsets.
constexpr size_t OBJECT_TYPE = 0x00;     // i32
constexpr size_t OBJECT_POSITION = 0x08; // i64 x3
constexpr size_t OBJECT_STATE = 0x28;    // byte: a door shut, or an object still waiting for its item
constexpr size_t OBJECT_SWING = 0x2c;    // float: ms into the open/shut swing (1000 = done)
constexpr size_t OBJECT_USER = 0x30;     // i64: who sits or lies on it (0 = free)
constexpr size_t OBJECT_ITEM = 0x38;     // the item it takes (0xa0 bytes; type 0 = none)
constexpr float FULL_SWING = 1000.0f;    // ms

// A sound request (world +0xa0, 0x30 bytes): position +0x00, id +0x18, pitch +0x1c,
// volume +0x20, owner +0x28.
using Sound = std::array<uint8_t, 0x30>;

// 0x2d3a10: an object goes to `state` (0 = open, else shut). A change restarts its swing
// from where the last one stood and asks for the sound of its type at the object; asking
// for the state it is already in does nothing. Returns whether the state changed.
bool openOrShut(nativeai::Object &object, uint8_t state, std::vector<Sound> &sounds);

// 0x4df60: whether two items (0xa0 bytes) are the same item, by the fields the game compares.
bool sameItem(const uint8_t *a, const uint8_t *b);

// The sounds these kinds play (their index in the game's sound list).
constexpr int32_t BUBBLE_SOUND = 0x30;        // pickup.wav (also a pick-up's)
constexpr int32_t HEROIC_SHOUT_SOUND = 0x6b;  // heroic-shout.wav
constexpr int32_t SLEEP_SOUND = 0x1f;
constexpr int32_t DROP_SOUND = 0x3c;
constexpr int32_t GATE_OPENER_SOUND = 0x70;

constexpr int MAX_OWNED = 3;                  // a shadow clone needs its caster to own fewer creatures
constexpr float HEROIC_SHOUT_RANGE2 = 2500.0f; // squared blocks: within 50 blocks of the shouter

// A ground item (zone +0x30, 0xe0 bytes): the item +0x00, position +0xa0, flags +0xc4,
// count +0xd4.
using GroundItem = std::array<uint8_t, 0xe0>;
// The slot a picked-up item leaves (§6 step 6): an empty ground item, its item type 0.
void emptyGroundItem(GroundItem &g);
// A pick-up notice (world +0x110, 0xd0 bytes): who +0x00, the item +0x08, count +0xa8,
// where it lay +0xb0, its flags +0xc8.
using PickUpNotice = std::array<uint8_t, 0xd0>;

using ZoneKey = std::pair<int, int>;

struct World {
    bool client = false;   // world+0x358: a client of a remote server (nothing is handled here)
    int32_t day = 0;       // world+0x08: the day counter
    float timeOfDay = 0;   // world+0x0c: ms since midnight
    std::function<nativeai::Creature *(int64_t id)> byId;                    // world+0x450
    std::vector<nativeai::Creature *> creatures;                             // world+0x40, in the world's order
    // Kinds 1, 11 and 12 make creatures. `construct` (0x4ce80) returns a new creature with
    // the constructor's defaults and the id given, kept by the world but not in it yet;
    // null if the world can't make one (the interaction then does nothing). Once it is set
    // up, the handler appends it to `creatures` and calls `added` (0x29d0d0), for the
    // world's id map and its other lists, so it takes its own turns later in this tick.
    std::function<nativeai::Creature *(int64_t id)> construct;
    std::function<void(nativeai::Creature *made)> added;
    std::function<std::vector<nativeai::Object> *(int zx, int zy)> objects;  // a loaded zone's objects (zone+0x60)
    std::function<std::vector<GroundItem> *(int zx, int zy)> groundItems;    // a loaded zone's ground items (zone+0x30)
    // What a chest holds (zone+0x450): the item kept for object `index`, or null if none.
    std::function<const uint8_t *(int zx, int zy, int index)> contents;
    std::vector<Sound> sounds;                                               // world+0xa0
    std::vector<nativebuffs::Notice> buffNotes;                              // world+0x150
    std::function<int()> rand;                                               // the C runtime's rand(), 0..32767
    std::set<int64_t> *rewarded = nullptr;                                   // world+0x30
    // 0x2a7ca0: a reward of `tier` at `pos`. `from` is the record of the creature it thanks
    // (for the caller's bookkeeping), or null for an object's reward.
    std::function<void(const int64_t pos[3], int tier, const uint8_t *from)> reward;
    // 0x2a5b70: an item (0xa0 bytes) laid at `pos` with its angle, size 1 and count. `by`
    // is the creature the game passes along (kind 8's dropper), or null; `from` is whose
    // item it was, for the caller's bookkeeping, or null.
    std::function<void(const uint8_t *item, const int64_t pos[3], float angle, int count, nativeai::Creature *by,
                       const uint8_t *from)>
        lay;
    std::vector<PickUpNotice> pickUps;    // world+0x110
    std::vector<int32_t> sleepNotices;    // world+0x160
    std::set<ZoneKey> changedGroundZones; // world+0x190: zones whose ground items changed
    std::set<ZoneKey> changedZones;       // world+0x1a0: zones whose objects changed
};

// 0x2c3710 for `c`: each of its pending interactions, front to back, then the list is
// emptied. On a client (`world.client`) nothing is handled and the list is kept.
void step(World &world, nativeai::Creature *c);

} // namespace nativeinteract
