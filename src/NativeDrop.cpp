// Death drops, rewards and the item generators, written from docs/specs/drops-and-items.md.
// Every rand() draw happens in the order the spec lists, since the game shares one random
// sequence with everything else. Section numbers below refer to that spec.
#include "NativeDrop.h"

#include "worldgen/Places.h"
#include "worldgen/Residents.h"
#include "NativeRules.h"

#include <cstring>

namespace nativedrop {
namespace {

// --- Record layouts -------------------------------------------------------------------

// The item record (§0.1). The bytes in between (+0x01..+0x03, +0x15..+0x17, +0x9e, +0x9f)
// are padding that the game never copies or clears.
namespace item {
constexpr size_t TYPE = 0x00;      // u8
constexpr size_t SUBTYPE = 0x04;   // i32
constexpr size_t SEED = 0x08;      // u32
constexpr size_t REGION_X = 0x0c;  // i32
constexpr size_t REGION_Y = 0x10;  // i32
constexpr size_t RARITY = 0x14;    // u8
constexpr size_t STANDS_FOR = 0x18; // i32, for type 2
constexpr size_t MATERIAL = 0x1c;  // u8
constexpr size_t ATTRIBUTES = 0x1d; // 32 entries of 4 bytes
constexpr size_t ATTRIBUTE_COUNT = 0x9d; // u8
} // namespace item

// Item types met here.
enum ItemType : uint8_t {
    CONSUMABLE = 1,
    WEAPON = 3,
    CHEST = 4,
    HANDS = 5,
    FEET = 6,
    SHOULDER = 7,
    NECK = 8,
    RING = 9,
    RESOURCE = 11,
    COIN = 12,
    TYPE_15 = 15,
    PET_FOOD = 21,
};

// Creature fields (§0.2) that worldgen::creature does not name.
namespace creature {
constexpr size_t FLIGHT_ZONE = 0x1b0;   // two i32, copied into ground items
constexpr size_t NECK_SLOT = 0x318;
constexpr size_t CHEST_SLOT = 0x3b8;
constexpr size_t FEET_SLOT = 0x458;
constexpr size_t HANDS_SLOT = 0x4f8;
constexpr size_t SHOULDER_SLOT = 0x598;
constexpr size_t LEFT_WEAPON_SLOT = 0x638;
constexpr size_t RIGHT_WEAPON_SLOT = 0x6d8;
constexpr size_t PET_SLOT = 0x8b8;
constexpr size_t RESERVATION_ID = 0x968; // i64, stamped on items reserved for a player
} // namespace creature

// Hostility kinds (creature +0x60).
constexpr uint8_t PLAYER = 0, HOSTILE = 1, TOWNSPERSON = 3, OTHER_HOSTILE = 7;

// Appearance flags (creature +0x80).
constexpr uint32_t FLAG_TYPE_15_FOR_HOSTILE = 0x8;
constexpr uint32_t FLAG_TYPE_15 = 0x10;
constexpr uint32_t FLAG_ELITE = 0x200;     // forces gear, raises coins tenfold
constexpr uint32_t FLAG_NO_DROPS = 0x800;
constexpr uint32_t FLAG_BOSS = 0x40000;    // forces gear, raises coins twentyfold

// The ground item record (§0.4).
namespace ground {
constexpr size_t POSITION = 0xa0;  // i64 x, y
constexpr size_t HEIGHT = 0xb0;    // i64
constexpr size_t FLIGHT_ZONE = 0xb8; // two i32
constexpr size_t ANGLE = 0xc0;     // f32, degrees
constexpr size_t NO_PICKUP = 0xc4; // u8
constexpr size_t DELAY = 0xc8;     // f32
constexpr size_t UNUSED = 0xcc;    // i32, always 0
constexpr size_t DAY = 0xd0;       // i32
constexpr size_t COUNT = 0xd4;     // i32
constexpr size_t OWNER = 0xd8;     // i64
} // namespace ground

// Constants of §6.
constexpr float RAND_MAX_F = 32767.0f;
constexpr float BLOCK = 65536.0f;          // world units per block
constexpr float UNITS_TO_BLOCKS = 1.0f / 65536.0f;
constexpr float FULL_TURN = 360.0f;
constexpr float RESERVE_RADIUS2 = 65536.0f; // blocks², i.e. 256 blocks
constexpr float REWARD_RADIUS2 = 10000.0f;  // blocks², i.e. 100 blocks
constexpr float SCALE_GEAR = 1.0f, SCALE_SMALL = 0.75f, SCALE_RESOURCE = 0.8f, SCALE_LARGE = 3.0f;

// --- Small helpers ----------------------------------------------------------------------

template <class T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <class T> void put(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }

// Clearing an item zeroes every field and leaves the padding alone (§0.1).
void clearItem(uint8_t *it) {
    it[item::TYPE] = 0;
    put<int32_t>(it, item::SUBTYPE, 0);
    put<uint32_t>(it, item::SEED, 0);
    put<int32_t>(it, item::REGION_X, 0);
    put<int32_t>(it, item::REGION_Y, 0);
    it[item::RARITY] = 0;
    put<int32_t>(it, item::STANDS_FOR, 0);
    it[item::MATERIAL] = 0;
    std::memset(it + item::ATTRIBUTES, 0, item::ATTRIBUTE_COUNT - item::ATTRIBUTES);
    it[item::ATTRIBUTE_COUNT] = 0;
}

// Copying an item copies its fields, never its padding (§0.1).
void copyItem(uint8_t *to, const uint8_t *from) {
    to[item::TYPE] = from[item::TYPE];
    std::memcpy(to + item::SUBTYPE, from + item::SUBTYPE, item::RARITY + 1 - item::SUBTYPE);
    std::memcpy(to + item::STANDS_FOR, from + item::STANDS_FOR, item::ATTRIBUTE_COUNT + 1 - item::STANDS_FOR);
}

Item clearedItem() {
    Item it{};
    clearItem(it.data());
    return it;
}

const int64_t *positionOf(const nativeai::Creature *c) {
    return reinterpret_cast<const int64_t *>(c->record + worldgen::creature::POSITION);
}
uint8_t hostilityOf(const nativeai::Creature *c) { return c->record[worldgen::creature::HOSTILITY]; }
uint32_t flagsOf(const nativeai::Creature *c) { return get<uint32_t>(c->record, worldgen::creature::FLAGS); }
int classOf(const nativeai::Creature *c) { return c->record[worldgen::creature::CLASS]; }

// A position's block rounds down; its "truncated block" rounds toward zero (§0.3).
int blockOf(int64_t v) { return (int)(v >> 16); }
int truncatedBlockOf(int64_t v) { return (int)(v / 65536); }
int floorDiv(int v, int by) { return v >= 0 ? v / by : -((-v + by - 1) / by); }

// A block is open (air or water) when its type, byte 4, is 0 or 2.
bool isOpen(const nativeai::Block &b) { return (b[4] & ~2) == 0; }

// The squared distance in blocks between two positions, in single precision.
float distance2(const int64_t a[3], const int64_t b[3]) {
    float sum = 0;
    for (int i = 0; i < 3; i++) {
        const float d = (float)(a[i] - b[i]) * UNITS_TO_BLOCKS;
        sum += d * d;
    }
    return sum;
}

float randomAngle(World &w) { return (float)w.rand() * FULL_TURN / RAND_MAX_F; }

// --- Item rules ---------------------------------------------------------------------------

// The region set of §0.7: types whose region is stamped from the dropping creature.
bool takesRegion(uint8_t type) { return type < 31 && ((0x454083fcu >> type) & 1); }

// The reservation set of §0.7: items the drop layer hands to one player.
bool isReserved(const uint8_t *it) {
    const uint8_t type = it[item::TYPE];
    if (type == CONSUMABLE && get<int32_t>(it, item::SUBTYPE) == 13)
        return true;
    return type <= 26 && ((0x440d3fcu >> type) & 1);
}

// Types whose rarity a reservation may raise (§0.7): gear and type 26.
bool raisesRarity(uint8_t type) { return (type >= WEAPON && type <= RING) || type == 26; }

// Who may use an item (§0.5) is the class rule shared with the stat rules.
bool canUse(const nativeai::Creature *c, const uint8_t *it) { return nativerules::classMayUse(it, classOf(c)); }

// --- The scatter (§0.6) -----------------------------------------------------------------

struct Scatter {
    int64_t pos[3];
    float angle;
};

// One offset of up to a block either way, from one rand.
int64_t scatterOffset(int r) { return (int64_t)((1.0f - ((float)r + (float)r) / RAND_MAX_F) * BLOCK); }

// Three draws: the first moves y, the second x, the third picks the angle.
Scatter scatter(World &w, const int64_t centre[3]) {
    Scatter s;
    const int64_t dy = scatterOffset(w.rand());
    const int64_t dx = scatterOffset(w.rand());
    s.angle = randomAngle(w);
    s.pos[0] = centre[0] + dx;
    s.pos[1] = centre[1] + dy;
    s.pos[2] = centre[2];
    return s;
}

// Scatter around `centre`, then hand the item to the drop layer (count 1).
void scatterAndPlace(World &w, const uint8_t *it, const int64_t centre[3], float scale, nativeai::Creature *from,
                     int64_t owner = 0) {
    const Scatter s = scatter(w, centre);
    Place p;
    copyItem(p.item.data(), it);
    std::copy(s.pos, s.pos + 3, p.pos);
    p.angle = s.angle;
    p.scale = scale;
    p.count = 1;
    p.owner = owner;
    place(w, p, from);
}

// --- The drop layer, 0x2a5b70 (§1) --------------------------------------------------------

// Step 3: gear dropped by a non-player takes the region of the creature's home, or of the
// landmark it stands in when its block is marked (byte 5, bit 0x20).
void stampRegion(World &w, uint8_t *it, const nativeai::Creature *from) {
    const int64_t *pos = positionOf(from);
    const nativeai::Block here = w.block(blockOf(pos[0]), blockOf(pos[1]), blockOf(pos[2]));
    worldgen::Int2 point;
    if (here[5] & 0x20) {
        const int cx = floorDiv(truncatedBlockOf(pos[0]), 1024), cy = floorDiv(truncatedBlockOf(pos[1]), 1024);
        point = worldgen::cellPoint(cx, cy, worldgen::bigCell(cx, cy));
    } else {
        point.x = truncatedBlockOf(get<int64_t>(from->record, worldgen::creature::HOME));
        point.y = truncatedBlockOf(get<int64_t>(from->record, worldgen::creature::HOME + 8));
    }
    const worldgen::Int2 region = worldgen::regionAt(point.x, point.y);
    put<int32_t>(it, item::REGION_X, region.x);
    put<int32_t>(it, item::REGION_Y, region.y);
}

// Step 5.1-5.2: the players the item may go to.
std::vector<nativeai::Creature *> reservationCandidates(World &w, const uint8_t *it, const int64_t pos[3],
                                                        const nativeai::Creature *from) {
    std::vector<nativeai::Creature *> out;
    if (from && hostilityOf(from) == HOSTILE) {
        // Whoever the creature was angry at (ascending id), if a player who can use it.
        for (const auto &entry : from->aggro) {
            nativeai::Creature *c = w.byId ? w.byId(entry.first) : nullptr;
            if (c && hostilityOf(c) == PLAYER && canUse(c, it))
                out.push_back(c);
        }
    } else {
        for (nativeai::Creature *c : w.creatures)
            if (hostilityOf(c) == PLAYER && canUse(c, it) && distance2(positionOf(c), pos) < RESERVE_RADIUS2)
                out.push_back(c);
    }
    if (out.empty()) // any player at all
        for (nativeai::Creature *c : w.creatures)
            if (hostilityOf(c) == PLAYER)
                out.push_back(c);
    return out;
}

// Step 5.5: 1 in 20 per step, up to four steps, the rarity rises; then it is capped at 4.
void raiseRarity(World &w, uint8_t *it) {
    for (int step = 0; step < 4; step++) {
        if (w.rand() % 20 != 0)
            break;
        it[item::RARITY]++;
    }
    if (it[item::RARITY] > 3)
        it[item::RARITY] = 4;
}

// Step 5: returns false when there is no player to reserve for, and nothing is laid.
bool reserve(World &w, uint8_t *it, const int64_t pos[3], int64_t owner, const nativeai::Creature *from,
             int64_t &reservedFor) {
    const std::vector<nativeai::Creature *> candidates = reservationCandidates(w, it, pos, from);
    if (candidates.empty())
        return false;
    const nativeai::Creature *chosen = candidates[(size_t)w.rand() % candidates.size()];
    if (!canUse(chosen, it))
        return true;
    if (raisesRarity(it[item::TYPE]))
        raiseRarity(w, it);
    reservedFor = owner ? owner : get<int64_t>(chosen->record, creature::RESERVATION_ID);
    return true;
}

// Step 7.3: the height of the top of the first solid block at or below the position.
int64_t groundHeight(World &w, const int64_t pos[3]) {
    const int bx = blockOf(pos[0]), by = blockOf(pos[1]);
    int bz = blockOf(pos[2]);
    while (isOpen(w.block(bx, by, bz)))
        bz--;
    return (int64_t)(bz + 1) * 65536;
}

// --- Death drops, 0x2a6860 (§2) -----------------------------------------------------------

// §2.1 step 2: one worn item, 1 in 16 (always for elites).
void dropWornItem(World &w, nativeai::Creature *dead) {
    const uint8_t *r = dead->record;
    std::vector<const uint8_t *> worn;
    auto offer = [&](size_t slot, bool allowed) {
        if (r[slot + item::TYPE] != 0 && allowed)
            worn.push_back(r + slot);
    };
    auto notSubtypes = [&](size_t slot, int a, int b) {
        const int32_t s = get<int32_t>(r, slot + item::SUBTYPE);
        return s != a && s != b;
    };
    offer(creature::LEFT_WEAPON_SLOT, notSubtypes(creature::LEFT_WEAPON_SLOT, 22, 23));
    offer(creature::RIGHT_WEAPON_SLOT, notSubtypes(creature::RIGHT_WEAPON_SLOT, 22, 23));
    offer(creature::CHEST_SLOT, get<int32_t>(r, creature::CHEST_SLOT + item::SUBTYPE) != 1);
    offer(creature::SHOULDER_SLOT, true);
    offer(creature::HANDS_SLOT, true);
    offer(creature::FEET_SLOT, true);
    offer(creature::NECK_SLOT, true);
    if (worn.empty())
        return;
    const bool lucky = w.rand() % 16 == 0;
    if (!lucky && !(flagsOf(dead) & FLAG_ELITE))
        return;
    const uint8_t *chosen = worn[(size_t)w.rand() % worn.size()];
    scatterAndPlace(w, chosen, positionOf(dead), SCALE_GEAR, dead);
}

// §2.1 step 3: the pet, 1 in 50.
void dropPet(World &w, nativeai::Creature *dead) {
    const uint8_t *pet = dead->record + creature::PET_SLOT;
    if (pet[item::TYPE] == 0)
        return;
    if (w.rand() % 50 == 0)
        scatterAndPlace(w, pet, positionOf(dead), SCALE_GEAR, dead);
}

// §2.2 steps 1-2: a generated weapon or armour piece of the creature's level.
void dropGeneratedGear(World &w, nativeai::Creature *dead) {
    const uint32_t flags = flagsOf(dead);
    const bool forced = hostilityOf(dead) == HOSTILE && (flags & (FLAG_ELITE | FLAG_BOSS));
    if (w.rand() % 20 != 0 && !forced)
        return;
    Item it = clearedItem();
    const uint8_t rarity = (uint8_t)get<int32_t>(dead->record, worldgen::creature::LEVEL);
    if (w.rand() % 2 == 0)
        weapon(w, it.data(), rarity, -1);
    else
        armour(w, it.data(), rarity, -1, false);
    if (it[item::RARITY] < 4 && (flags & (FLAG_ELITE | FLAG_BOSS)))
        it[item::RARITY]++;
    scatterAndPlace(w, it.data(), positionOf(dead), SCALE_GEAR, dead);
}

// §2.2 step 3: 1 in 8, pet food or one of four set items.
void dropFixedItem(World &w, nativeai::Creature *dead) {
    if (w.rand() % 8 != 0)
        return;
    Item it = clearedItem();
    switch (w.rand() % 5) {
    case 0:
        petFood(w, it.data(), 0);
        break;
    case 1:
        it[item::TYPE] = RESOURCE, put<int32_t>(it.data(), item::SUBTYPE, 12), it[item::MATERIAL] = 25;
        break;
    case 2:
        it[item::TYPE] = CONSUMABLE, put<int32_t>(it.data(), item::SUBTYPE, 4);
        break;
    case 3:
        it[item::TYPE] = CONSUMABLE, put<int32_t>(it.data(), item::SUBTYPE, 7);
        break;
    default:
        it[item::TYPE] = CONSUMABLE, put<int32_t>(it.data(), item::SUBTYPE, 9);
        break;
    }
    scatterAndPlace(w, it.data(), positionOf(dead), SCALE_GEAR, dead);
}

// §2.2 step 4: a type-15 item, 1 in 20, for creatures flagged for it.
void dropType15Item(World &w, nativeai::Creature *dead) {
    const uint32_t flags = flagsOf(dead);
    const bool eligible =
        (hostilityOf(dead) == HOSTILE && (flags & FLAG_TYPE_15_FOR_HOSTILE)) || (flags & FLAG_TYPE_15);
    if (!eligible || w.rand() % 20 != 0)
        return;
    const int32_t level = get<int32_t>(dead->record, worldgen::creature::LEVEL);
    Item it = clearedItem();
    it[item::TYPE] = TYPE_15;
    it[item::RARITY] = level > 1 ? (uint8_t)level : 1;
    put<uint32_t>(it.data(), item::SEED, (uint32_t)w.rand());
    scatterAndPlace(w, it.data(), positionOf(dead), SCALE_SMALL, dead);
}

Item coin() {
    Item it = clearedItem();
    it[item::TYPE] = COIN;
    it[item::MATERIAL] = 11;
    return it;
}

// §2.2 step 5: (1 + 0.1 (level - 1)) x (1..2) coins, rounded up; x10 for elites, x20 for
// bosses.
void dropCoins(World &w, nativeai::Creature *dead) {
    const uint32_t flags = flagsOf(dead);
    const float base = ((float)get<int32_t>(dead->record, worldgen::creature::LEVEL) - 1.0f) * 0.1f + 1.0f;
    float amount = base * ((float)w.rand() / RAND_MAX_F + 1.0f);
    if (flags & FLAG_ELITE)
        amount *= 10.0f;
    if (flags & FLAG_BOSS)
        amount *= 20.0f;
    if (!(amount > 0))
        return;
    const Item c = coin();
    int laid = 0;
    do {
        scatterAndPlace(w, c.data(), positionOf(dead), SCALE_SMALL, dead);
        laid++;
    } while ((float)laid < amount);
}

// §2.3: one resource of a race, written into `it` (left empty for races without one);
// returns the scale it is dropped with.
float raceResource(World &w, int race, uint8_t *it) {
    auto resource = [&](int subtype, uint8_t material = 0, uint8_t rarity = 0) {
        it[item::TYPE] = RESOURCE;
        put<int32_t>(it, item::SUBTYPE, subtype);
        it[item::MATERIAL] = material;
        it[item::RARITY] = rarity;
    };
    switch (race) {
    case 0x3a: resource(2, 20); return SCALE_RESOURCE;
    case 0x46: resource(18); return SCALE_RESOURCE;
    case 0x47: resource(21); return SCALE_RESOURCE;
    case 0x78:
    case 0x79:
    case 0x7c:
    case 0x7e:
        if (w.rand() % 2 == 0)
            resource(5, 22);
        else
            resource(1, 2);
        return SCALE_RESOURCE;
    case 0x7a: resource(27); return SCALE_RESOURCE;
    case 0x7b: resource(11, 28); return SCALE_RESOURCE;
    case 0x7d: resource(6); return SCALE_RESOURCE;
    case 0x7f: resource(20); return SCALE_RESOURCE;
    case 0x80: resource(23); return SCALE_RESOURCE;
    case 0x83: resource(0, 11); return SCALE_LARGE;
    case 0x84: resource(0, 1); return SCALE_LARGE;
    case 0x85: resource(0, 12); return SCALE_LARGE;
    case 0x86: resource(0, 18); return SCALE_RESOURCE;
    case 0x87: resource(32, 0, 1); return SCALE_LARGE;
    case 0x88: resource(33, 0, 2); return SCALE_LARGE;
    case 0x89: resource(34, 0, 3); return SCALE_LARGE;
    case 0x8a: resource(35, 0, 4); return SCALE_LARGE;
    case 0x8b: resource(8, 23); return SCALE_RESOURCE;
    default: return SCALE_SMALL;
    }
}

// §2.3: 1..3 resources of the creature's race; 1 in 8 each becomes subtype 37 for the
// races 0x78..0x7f other than 0x7e.
void dropRaceResources(World &w, nativeai::Creature *dead) {
    const int race = get<int32_t>(dead->record, worldgen::creature::RACE);
    const bool mayBecome37 = race >= 0x78 && race <= 0x7f && race != 0x7e;
    const int count = w.rand() % 3 + 1;
    for (int i = 0; i < count; i++) {
        Item it = clearedItem();
        const float scale = raceResource(w, race, it.data());
        if (w.rand() % 8 == 0 && mayBecome37) {
            it[item::TYPE] = RESOURCE;
            put<int32_t>(it.data(), item::SUBTYPE, 37);
            it[item::MATERIAL] = 0;
            it[item::RARITY] = 0;
        } else if (it[item::TYPE] == 0) {
            break;
        }
        scatterAndPlace(w, it.data(), positionOf(dead), scale, dead);
    }
}

// §2.1 step 6: every stack it carries, one item at a time (except type 1 subtype 1).
void dropInventory(World &w, nativeai::Creature *dead) {
    for (const auto &tab : dead->inventory)
        for (const auto &slot : tab) {
            const int32_t count = get<int32_t>(slot.data(), 0);
            const uint8_t *it = slot.data() + 4;
            if (it[item::TYPE] == 0 || count <= 0)
                continue;
            if (it[item::TYPE] == CONSUMABLE && get<int32_t>(it, item::SUBTYPE) == 1)
                continue;
            for (int32_t i = 0; i < count; i++)
                scatterAndPlace(w, it, positionOf(dead), SCALE_SMALL, dead);
        }
}

// --- Generators (§3) ----------------------------------------------------------------------

// The generators build a list of candidates by changing the output and keeping a copy of
// it, then copy one of them, picked by a final draw, back over the output.
struct Candidates {
    std::vector<Item> list;
    void keep(const uint8_t *it) {
        Item copy{};
        copyItem(copy.data(), it);
        list.push_back(copy);
    }
    void pick(World &w, uint8_t *out) const { copyItem(out, list[(size_t)w.rand() % list.size()].data()); }
};

// A class outside 1..4 asks for every class's items.
bool anyClass(int cls) { return cls < 1 || cls > 4; }

void seedAndKeep(World &w, uint8_t *it, Candidates &c) {
    put<uint32_t>(it, item::SEED, (uint32_t)w.rand());
    c.keep(it);
}

} // namespace

// §1.
void place(World &w, const Place &p, nativeai::Creature *from) {
    if (w.placing)
        w.placing(p);
    // Steps 1-2: the working item and the ground record.
    Item it{};
    copyItem(it.data(), p.item.data());
    Drop d{};
    put<int64_t>(d.data(), ground::POSITION, p.pos[0]);
    put<int64_t>(d.data(), ground::POSITION + 8, p.pos[1]);
    put<int64_t>(d.data(), ground::HEIGHT, p.pos[2]);
    put<int32_t>(d.data(), ground::FLIGHT_ZONE, -1);
    put<int32_t>(d.data(), ground::FLIGHT_ZONE + 4, -1);
    put<float>(d.data(), ground::ANGLE, p.angle);
    put<int32_t>(d.data(), ground::UNUSED, 0);
    put<int64_t>(d.data(), ground::OWNER, 0);
    put<float>(d.data(), ground::DELAY, (float)(w.rand() % 300 + 500));
    put<int32_t>(d.data(), ground::DAY, w.day);
    if (from) {
        put<int32_t>(d.data(), ground::FLIGHT_ZONE, get<int32_t>(from->record, creature::FLIGHT_ZONE));
        put<int32_t>(d.data(), ground::FLIGHT_ZONE + 4, get<int32_t>(from->record, creature::FLIGHT_ZONE + 4));
    }
    // Step 3.
    if (from && hostilityOf(from) != PLAYER && takesRegion(it[item::TYPE]))
        stampRegion(w, it.data(), from);
    // Step 4.
    put<int32_t>(d.data(), ground::COUNT, p.count);
    d[ground::NO_PICKUP] = (it[item::TYPE] == 23 || it[item::TYPE] == 24) ? 2 : 0;
    // Step 5.
    const bool droppedByPlayer = from && hostilityOf(from) == PLAYER;
    if (!droppedByPlayer && isReserved(it.data())) {
        int64_t owner = 0;
        if (!reserve(w, it.data(), p.pos, p.owner, from, owner))
            return;
        put<int64_t>(d.data(), ground::OWNER, owner);
    }
    // Step 6: only a loaded zone takes the item.
    if (!w.zone || !w.zone(blockOf(p.pos[0]) >> 6, blockOf(p.pos[1]) >> 6))
        return;
    // Step 7.
    copyItem(d.data(), it.data());
    put<float>(d.data(), ground::ANGLE, randomAngle(w));
    put<int64_t>(d.data(), ground::HEIGHT, groundHeight(w, p.pos));
    w.drops.push_back(d);
}

// §2.1.
void drop(World &w, nativeai::Creature *dead) {
    const uint8_t kind = hostilityOf(dead);
    if (kind == PLAYER || kind == TOWNSPERSON || (flagsOf(dead) & FLAG_NO_DROPS))
        return;
    dropWornItem(w, dead);
    dropPet(w, dead);
    const bool hostile = kind == HOSTILE || kind == OTHER_HOSTILE;
    if (hostile) { // §2.2
        dropGeneratedGear(w, dead);
        dropFixedItem(w, dead);
        dropType15Item(w, dead);
        if (kind == HOSTILE)
            dropCoins(w, dead);
    }
    dropRaceResources(w, dead);
    if (hostile)
        dropInventory(w, dead);
}

// §4.
void reward(World &w, const int64_t pos[3], int tier) {
    const worldgen::Int2 region = worldgen::regionAt(truncatedBlockOf(pos[0]), truncatedBlockOf(pos[1]));
    // Gear for each player within 100 blocks, offered to that player.
    for (nativeai::Creature *player : w.creatures) {
        if (hostilityOf(player) != PLAYER || !(distance2(positionOf(player), pos) < REWARD_RADIUS2))
            continue;
        Item it = clearedItem();
        if (w.rand() % 2 == 0)
            armour(w, it.data(), (uint8_t)tier, classOf(player), false);
        else
            weapon(w, it.data(), (uint8_t)tier, classOf(player));
        put<int32_t>(it.data(), item::REGION_X, region.x);
        put<int32_t>(it.data(), item::REGION_Y, region.y);
        scatterAndPlace(w, it.data(), pos, SCALE_GEAR, nullptr, get<int64_t>(player->record, creature::RESERVATION_ID));
    }
    // Coins: (1 + 0.1 (tier - 1)) x 10 x (1..2), rounded down.
    const float perTier = (((float)tier - 1.0f) * 0.1f + 1.0f) * 10.0f;
    const int coins = (int)(((float)w.rand() / RAND_MAX_F + 1.0f) * perTier);
    const Item c = coin();
    for (int i = 0; i < coins; i++)
        scatterAndPlace(w, c.data(), pos, SCALE_SMALL, nullptr);
    // Tier resources: for each tier up to this one, 1 in 8.
    for (int i = 1; i <= tier; i++) {
        if (w.rand() % 8 != 0)
            continue;
        Item it = clearedItem();
        it[item::TYPE] = RESOURCE;
        put<int32_t>(it.data(), item::SUBTYPE, 31 + i);
        it[item::RARITY] = (uint8_t)i;
        scatterAndPlace(w, it.data(), pos, SCALE_SMALL, nullptr);
    }
}

// §3.1.
void weapon(World &w, uint8_t *out, uint8_t rarity, int cls) {
    Candidates c;
    clearItem(out);
    out[item::RARITY] = rarity;
    out[item::MATERIAL] = 1;
    out[item::TYPE] = WEAPON;
    auto subtype = [&](int s) { put<int32_t>(out, item::SUBTYPE, s); };
    if (cls != 2 && cls != 3 && cls != 4) { // warrior
        subtype(w.rand() % 3);
        seedAndKeep(w, out, c);
        out[item::MATERIAL] = 1;
        subtype(w.rand() % 3 + 15);
        seedAndKeep(w, out, c);
        subtype(13);
        seedAndKeep(w, out, c);
    }
    if (cls != 1 && cls != 2 && cls != 3) { // rogue
        subtype(3), seedAndKeep(w, out, c);
        subtype(5), seedAndKeep(w, out, c);
        subtype(4), seedAndKeep(w, out, c);
    }
    out[item::MATERIAL] = 2;
    if (cls == 2 || anyClass(cls)) { // ranger
        subtype(6), seedAndKeep(w, out, c);
        subtype(7), put<uint32_t>(out, item::SEED, (uint32_t)w.rand()); // drawn, never kept
        subtype(8), seedAndKeep(w, out, c);
    }
    if (cls == 3 || anyClass(cls)) { // mage
        subtype(10), seedAndKeep(w, out, c);
        subtype(11), seedAndKeep(w, out, c);
        subtype(12);
        out[item::MATERIAL] = w.rand() % 2 == 0 ? 12 : 11;
        seedAndKeep(w, out, c);
    }
    c.pick(w, out);
}

// §3.2.
void armour(World &w, uint8_t *out, uint8_t rarity, int cls, bool plain) {
    Candidates c;
    clearItem(out);
    out[item::RARITY] = rarity;
    auto keepType = [&](uint8_t type) {
        out[item::TYPE] = type;
        seedAndKeep(w, out, c);
    };
    if (cls != 2 && cls != 3 && cls != 4) { // warrior
        uint8_t material = 1;
        if (!plain) {
            const int r = w.rand() % 3;
            material = r == 1 ? 29 : r == 2 ? 4 : 1;
        }
        out[item::MATERIAL] = material;
        keepType(SHOULDER), keepType(CHEST), keepType(HANDS), keepType(FEET);
    }
    // The mage, ranger and rogue sections each draw a material and then use a fixed one.
    auto classSection = [&](uint8_t material) {
        if (!plain)
            w.rand(); // discarded
        out[item::MATERIAL] = material;
        keepType(SHOULDER), keepType(SHOULDER), keepType(CHEST), keepType(HANDS), keepType(FEET);
    };
    if (cls == 3 || anyClass(cls))
        classSection(26); // mage
    if (cls == 2 || anyClass(cls))
        classSection(27); // ranger
    if (cls != 1 && cls != 2 && cls != 3)
        classSection(28); // rogue
    // Accessories, for everyone.
    out[item::MATERIAL] = w.rand() % 2 == 0 ? 11 : 12;
    keepType(NECK);
    out[item::MATERIAL] = w.rand() % 2 == 0 ? 11 : 12;
    keepType(RING);
    c.pick(w, out);
}

// §3.3.
void petFood(World &w, uint8_t *out, int key) {
    const auto found = w.petListsByKey.find(key);
    const bool even = w.rand() % 2 == 0;
    const std::vector<int32_t> &list =
        even && found != w.petListsByKey.end() && !found->second.empty() ? found->second : w.petList;
    clearItem(out);
    out[item::TYPE] = PET_FOOD;
    const int r = w.rand();
    put<int32_t>(out, item::SUBTYPE, list.empty() ? 0 : list[(size_t)r % list.size()]);
}

bool inRegionSet(uint8_t type) { return takesRegion(type); }

} // namespace nativedrop
