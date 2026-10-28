#pragma once
// Spawn points: what a zone remembers about the creatures that live in it
// (docs/specs/worldgen-creatures.md §0.3, §1). The world turns a spawn point into a creature when
// a player comes near (Residents.h). Generation makes them with a race, looks, gear and a place.
#include "worldgen/Regions.h"
#include "worldgen/ZoneRecords.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <vector>

namespace worldgen {

// A source of rand() values: the zone's stream, or a test's script. A light reference to any
// callable returning int; it must outlive the call it is passed to.
class Draw {
public:
    template <class F>
    Draw(F &f) : context_(&f), call_([](void *c) { return (*static_cast<F *>(c))(); }) {}
    int operator()() const { return call_(context_); }

private:
    void *context_;
    int (*call_)(void *);
};

// A creature's appearance (0xb0 bytes; spawn point +0x50, creature +0x78), as 0x53e80 makes it:
// colours, the model ids of its parts, sizes and offsets, and the appearance flags.
struct Appearance {
    static constexpr size_t SIZE = 0xb0;
    // Offsets inside it.
    static constexpr size_t COLOUR = 0x2; // three bytes
    static constexpr size_t FLAGS = 0x8;
    static constexpr size_t SCALE = 0xc; // x, y, z sizes (+0xc, +0x10, +0x14)
    static constexpr size_t HEIGHT = 0x14;
    static constexpr size_t HEAD = 0x18, HAIR = 0x1a, HANDS = 0x1c, FEET = 0x1e, CHEST = 0x20, TAIL = 0x22, SHOULDERS = 0x24,
                            WINGS = 0x26;

    // The part scales (+0x28 ..) and the placing offsets of chest, head, hands, feet and tail
    // (three floats each, +0x68 ..), as creature-pose.md §2 names them.
    static constexpr size_t HEAD_SCALE = 0x28, CHEST_SCALE = 0x2c, HAND_SCALE = 0x30, FOOT_SCALE = 0x34,
                            ARM_SCALE = 0x38, WEAPON_SCALE = 0x3c, TAIL_SCALE = 0x40, SHOULDER_SCALE = 0x44,
                            WING_SCALE = 0x48;
    static constexpr size_t CHEST_PLACE = 0x68, HEAD_PLACE = 0x74, HAND_PLACE = 0x80, FOOT_PLACE = 0x8c,
                            TAIL_PLACE = 0x98;

    std::array<uint8_t, SIZE> bytes{};

    // The appearance as its constructor (0x4d1f0) leaves it (worldgen-creatures.md §7.3): white
    // hair, unit size, no models, the default part scales and places, everything else zero.
    // (The constructor leaves the padding +5..+7 alone; here it is zero.)
    static Appearance constructed() {
        Appearance a;
        a.bytes[COLOUR] = a.bytes[COLOUR + 1] = a.bytes[COLOUR + 2] = 0xff;
        for (size_t i = 0; i < 3; i++)
            a.set(SCALE + 4 * i, 1.0f);
        for (size_t at = HEAD; at <= WINGS; at += 2)
            a.set(at, (int16_t)-1);
        a.set(HEAD_SCALE, 1.01f);
        a.set(CHEST_SCALE, 1.0f);
        a.set(HAND_SCALE, 1.0f);
        a.set(FOOT_SCALE, 0.98f);
        a.set(ARM_SCALE, 1.0f);
        a.set(WEAPON_SCALE, 0.95f);
        a.set(TAIL_SCALE, 0.8f);
        a.set(SHOULDER_SCALE, 1.0f);
        a.set(WING_SCALE, 1.0f);
        a.setPlace(CHEST_PLACE, 0.0f, 0.0f, -5.0f);
        a.setPlace(HEAD_PLACE, 0.0f, 0.5f, 5.0f);
        a.setPlace(HAND_PLACE, 6.0f, 0.0f, 0.0f);
        a.setPlace(FOOT_PLACE, 3.0f, 1.0f, -10.5f);
        a.setPlace(TAIL_PLACE, 0.0f, -8.0f, 2.0f);
        return a;
    }

    uint32_t flags() const { return get<uint32_t>(FLAGS); }
    void setFlags(uint32_t f) { set(FLAGS, f); }
    void addFlags(uint32_t f) { setFlags(flags() | f); }
    float height() const { return get<float>(HEIGHT); }

    template <class T> T get(size_t at) const {
        T v;
        std::memcpy(&v, &bytes[at], sizeof v);
        return v;
    }
    template <class T> void set(size_t at, T v) { std::memcpy(&bytes[at], &v, sizeof v); }
    void setPlace(size_t at, float x, float y, float z) { set(at, x), set(at + 4, y), set(at + 8, z); }
};

// Appearance flags generation sets or reads.
namespace looksflag {
constexpr uint32_t TAKE_DOWN_WEAPONS = 0x8;     // gear: both hands hold weapon 0x16
constexpr uint32_t ONE_WEAPON = 0x10;           // gear: weapon 0x17 in the right hand
constexpr uint32_t PLAIN_CHEST = 0x20;          // gear: chest piece id 1, no armour
constexpr uint32_t STRONG = 0x200;              // grows 1.5 times; leaders and bosses
constexpr uint32_t RESIDENT = 0x2000;           // belongs to its place (a mission's people)
constexpr uint32_t QUEST_GIVER = 0x1000;
constexpr uint32_t WILD = 0x8000;               // added to some races in wild groups
constexpr uint32_t BIG = 0x20000;               // grows 1.5 times
constexpr uint32_t GIANT = 0x40000;             // grows 7 times
constexpr uint32_t SPECIAL_TOWN = 0x400000;     // townsfolk of the region's special town
constexpr uint32_t FOREIGN_STYLE = 0x100000;
constexpr uint32_t DEMON_MARKED = 0x4000000;    // a demon portal's region
} // namespace looksflag

// The equipment slots, in the game's order (cube::Equipment).
enum EquipSlot { SLOT_NONE, NECK, CHEST, FEET, HANDS, SHOULDER, LEFT_HAND, RIGHT_HAND, LEFT_RING, RIGHT_RING, PET, SLOTS };

// One inventory stack (0xa4 bytes): a quantity of an item.
struct Stack {
    int32_t quantity = 0;
    Item item;
};

// An inventory (spawn point +0x7f0, creature +0x9f0; worldgen-creatures.md §4.5): tabs of stacks,
// and two counters for items that are only counted, never stacked.
struct Inventory {
    std::vector<std::vector<Stack>> tabs; // +0x00
    int32_t countedMaterial = 0;          // +0xbc: category 12 with material 11
    int32_t countedCategory14 = 0;        // +0xc0: category 14

    // Adds `quantity` of an item (0x4e810), onto `tab`, or with -1 onto its category's tab.
    void add(const Item &item, int32_t quantity, int tab = -1);
    // The tab an item of a category goes to (0x5f100).
    static int tabOf(uint8_t category);
    // Items of these categories pile up in one stack (0x5f0e0).
    static bool stackable(uint8_t category);
    bool empty() const { return tabs.empty() && countedMaterial == 0 && countedCategory14 == 0; }
};

// What gear 0x511e0 gives a creature (worldgen-creatures.md §4.3).
struct Gear {
    uint8_t cls = 0;  // in: the class chosen by the caller (0 picks one); out: the class
    uint8_t spec = 0; // specialisation
    std::array<Item, SLOTS> equipment{};
    std::vector<int32_t> abilities;  // +0x8c0
    int32_t abilityPick = 0;          // +0x8d8
    std::vector<int32_t> abilities2; // +0x8e0
    Inventory inventory;             // +0x7f0
};

// A spawn point (cube::SpawnPoint, 0x950 bytes; §0.3), with its constructor's defaults (0x118a60).
struct SpawnPoint {
    int64_t x = 0, y = 0, z = 0; // +0x00: 1/65536 blocks, z at the feet
    float yaw = 0;               // +0x18, degrees
    int32_t hostility = 1;       // +0x1c: 1 hostile, 3 friendly person, 4 neutral, 6, 7 animal, 8 pet-like
    int32_t race = 4;            // +0x20
    int32_t level = 0;           // +0x24
    int32_t people = 0;          // +0x28: the nation it belongs to (a realm kind 12-22, or a group code)
    int32_t tag = -1;            // +0x2c: copied to the creature's +0x974; generation never sets it
    uint32_t style = 0;          // +0x30: the realm of its place
    int64_t id = 0;              // +0x38
    int64_t owner = 0;           // +0x40: the point it belongs to (a rider, a boss, a master)
    uint8_t stage = 0;           // +0x4c: copied to the creature's stage byte
    Appearance appearance = Appearance::constructed(); // +0x50
    char name[16] = {};          // +0x100
    Gear gear;                   // +0x34, +0x35 (class), +0x110 (equipment), +0x7f0, +0x8c0..+0x8e0
    uint32_t roam = 0;           // +0x8b8: how it wanders
    uint8_t mode = 0;            // +0x8bc: 0x8a for a spider that drops from above
    Int2 home{0x7fffffff, 0x7fffffff}; // +0x8f8: the place cell of an encounter's people
    std::vector<Int2> area;      // +0x900: 16-block cells it keeps to
    std::set<int64_t> group;     // +0x910: ids it keeps with
    int32_t requiredDefeated = 0; // +0x920
    std::vector<Int2> clearedCells; // +0x928: never filled by generation
    std::set<int64_t> prerequisites; // +0x938: ids that must all be dead
    int32_t requiredStage = 0;   // +0x948

    uint32_t flags() const { return appearance.flags(); }
    void addFlags(uint32_t f) { appearance.addFlags(f); }
};

// Hostility kinds generation uses.
namespace hostility {
constexpr int32_t HOSTILE = 1;
constexpr int32_t FRIENDLY = 3;
constexpr int32_t NEUTRAL = 4;
constexpr int32_t TAMED = 6;
constexpr int32_t ANIMAL = 7;
constexpr int32_t PET = 8;
} // namespace hostility

// The creature lists of a zone (zone +0x460 .. +0x500).
struct ZoneSpawns {
    std::map<int64_t, SpawnPoint> points; // +0x460: by id, which is the order they were added in
    std::vector<int64_t> tracked;         // +0x480: deaths that count toward requiredDefeated
    int32_t deadTracked = 0;              // +0x490
    std::vector<int64_t> mission;         // +0x498: all must die for the place's mission
    std::vector<int64_t> refreshed;       // +0x4e8: the daily refresh's creatures
    int32_t day = -1;                     // +0x4f8: the day of the last refresh
    struct ShopSpot {
        int64_t x = 0, y = 0, z = 0;
    };
    std::vector<ShopSpot> shopSpots; // +0x500

    // The zone takes a point (0x119530): one without an id gets the zone's first free id.
    int64_t add(SpawnPoint p, int32_t zx, int32_t zy);
    SpawnPoint *find(int64_t id);
};

// The first id of a zone's spawn points (§1.1).
int64_t firstSpawnId(int32_t zx, int32_t zy);

} // namespace worldgen
