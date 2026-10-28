// Creature records, residents, arena stages and pets, written from
// docs/specs/worldgen-creatures.md §7.
#include "worldgen/Residents.h"

#include "worldgen/Gear.h"
#include "worldgen/Looks.h"
#include "worldgen/StructureTables.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>

namespace worldgen {
namespace {

constexpr int64_t REACH = 256 * DOTS; // a player's reach: 256 blocks

template <class T> void put(uint8_t *record, size_t at, T v) { std::memcpy(record + at, &v, sizeof v); }
template <class T> T get(const uint8_t *record, size_t at) {
    T v;
    std::memcpy(&v, record + at, sizeof v);
    return v;
}

constexpr int32_t NO_PLACE = 0x7fffffff;
constexpr uint8_t SPIDER_DROPS = 0x8a; // the mode of a spider that drops from above

// 0xf3e20: within 256 blocks of the player on each axis and in all.
bool withinReach(const SpawnPoint &p, const ResidentWorld::Player &q) {
    const int64_t dx = p.x - q.x, dy = p.y - q.y, dz = p.z - q.z;
    if (std::llabs(dx) > REACH || std::llabs(dy) > REACH)
        return false;
    const double fx = (double)dx * (1.0 / 65536.0), fy = (double)dy * (1.0 / 65536.0), fz = (double)dz * (1.0 / 65536.0);
    return fx * fx + fy * fy + fz * fz <= 256.0 * 256.0;
}

} // namespace

// --- §7.3 The constructor --------------------------------------------------------------------

CreatureRecord newCreature(int64_t id) {
    using namespace creature;
    CreatureRecord r(CREATURE_SIZE, 0);
    uint8_t *c = r.data();
    put(c, ID, id);
    c[HOSTILITY] = (uint8_t)hostility::FRIENDLY;
    std::memcpy(c + APPEARANCE, Appearance::constructed().bytes.data(), Appearance::SIZE);
    put(c, STUN, -3000.0f);
    put(c, LEVEL, (int32_t)1);
    put(c, HEALTH, 500.0f);
    put(c, BLOCK_POWER, 1.0f);
    put(c, GRIP, 1.0f);
    put(c, LIGHT_RADIUS, 10.0f);
    put(c, HOME_PLACE, NO_PLACE);
    put(c, HOME_PLACE + 4, NO_PLACE);
    put(c, HOME_CELL, (int32_t)-1);
    put(c, HOME_CELL + 4, (int32_t)-1);
    put(c, STAMINA, 1.0f);
    put(c, WETNESS, 1.0f);
    put(c, ABILITY_PICK, (int32_t)1);
    put(c, FIELD_1530, 200.0f);
    put(c, POSE_SIZE_FACTOR, 1.0f); // (not in §7.3's table; see Residents.h)
    return r;
}

// --- §7.1 Residents --------------------------------------------------------------------------

std::vector<Int2> residentZones(const std::vector<ResidentWorld::Player> &players) {
    std::vector<Int2> out;
    for (const auto &q : players) {
        const int32_t x0 = (int32_t)((q.x - REACH) >> 22), x1 = (int32_t)((q.x + REACH) >> 22); // 64 blocks of dots
        const int32_t y0 = (int32_t)((q.y - REACH) >> 22), y1 = (int32_t)((q.y + REACH) >> 22);
        for (int32_t x = x0; x <= x1; x++)
            for (int32_t y = y0; y <= y1; y++)
                if (std::find(out.begin(), out.end(), Int2{x, y}) == out.end())
                    out.push_back({x, y});
    }
    return out;
}

bool residentReady(const SpawnPoint &p, int32_t zx, int32_t zy, const ResidentWorld &world) {
    if ((world.alive && world.alive(p.id)) || (world.dead && world.dead(p.id)))
        return false;
    if (std::none_of(world.players.begin(), world.players.end(), [&](const auto &q) { return withinReach(p, q); }))
        return false;
    if (p.requiredDefeated != 0) {
        int64_t sum = 0;
        for (int32_t x = zx - 8; x <= zx + 8; x++)
            for (int32_t y = zy - 8; y <= zy + 8; y++)
                sum += world.deadTracked ? world.deadTracked(x, y) : 0;
        if (sum < p.requiredDefeated)
            return false;
    }
    if (p.requiredStage != 0 && !(world.arenaAtStage && world.arenaAtStage(p.requiredStage)))
        return false;
    for (int64_t id : p.prerequisites)
        if (!(world.dead && world.dead(id)))
            return false;
    return true;
}

void writeItem(uint8_t *at, const Item &item) {
    std::memset(at, 0, creature::ITEM_SIZE);
    at[0] = item.category;
    put(at, 0x4, item.id);
    put(at, 0x8, item.modifier);
    put(at, 0xc, item.region.x);
    put(at, 0x10, item.region.y);
    at[0x14] = item.rarity;
    put(at, 0x18, item.realm);
    at[0x1c] = item.material;
}

void wearGear(uint8_t *record, const Gear &gear) {
    using namespace creature;
    record[CLASS] = gear.cls;
    record[SPEC] = gear.spec;
    for (int s = 0; s < SLOTS; s++)
        writeItem(record + EQUIPMENT + (size_t)s * ITEM_SIZE, gear.equipment[(size_t)s]);
    put(record, ABILITY_PICK, gear.abilityPick);
}

MadeCreature makeResident(const SpawnPoint &p, Draw worldRand, bool spared, const std::function<float(const uint8_t *record)> &maxHealth) {
    using namespace creature;
    MadeCreature made{newCreature(p.id), {p.gear.inventory, p.gear.abilities, p.gear.abilities2, p.group}};
    uint8_t *record = made.record.data();
    // 3. The point's fields.
    for (size_t at : {HOME, POSITION}) {
        put(record, at, p.x);
        put(record, at + 8, p.y);
        put(record, at + 16, p.z);
    }
    put(record, HOME_CELL, floorDiv(blocksOf(p.x), 4)); // (16-block cells)
    put(record, HOME_CELL + 4, floorDiv(blocksOf(p.y), 4));
    put(record, YAW, p.yaw);
    record[HOSTILITY] = (uint8_t)p.hostility;
    put(record, RACE, p.race);
    record[PEOPLE] = (uint8_t)p.people;
    put(record, TAG, p.tag);
    put(record, STYLE, (uint64_t)p.style);
    put(record, LEVEL, p.level);
    wearGear(record, p.gear);
    record[STAGE] = p.stage;
    std::memcpy(record + APPEARANCE, p.appearance.bytes.data(), Appearance::SIZE);
    std::memcpy(record + NAME, p.name, sizeof p.name);
    // Fields generation never sets: the spawn point's zeros.
    put(record, UNSET_A08, (int32_t)0);
    std::memset(record + UNSET_ITEM, 0, ITEM_SIZE);
    put(record, UNSET_AAC, (int32_t)0);
    put(record, UNSET_AB0, (int32_t)0);
    put(record, ROAM, p.roam);
    put(record, OWNER, p.owner);
    put(record, HOME_PLACE, p.home.x);
    put(record, HOME_PLACE + 4, p.home.y);
    const Int2 region = regionAt(blocksOf(p.x), blocksOf(p.y));
    put(record, REGION, region.x);
    put(record, REGION + 4, region.y);
    if (maxHealth)
        put(record, HEALTH, maxHealth(record));
    record[ABILITY] = p.mode;
    // 4. A spider that drops from above starts higher.
    if (p.mode == SPIDER_DROPS) {
        int64_t z;
        std::memcpy(&z, record + POSITION + 16, 8);
        put(record, POSITION + 16, z + dotsOf(worldRand() % 50 + 80));
    }
    // 5. The demon portal's mark.
    uint32_t flags;
    std::memcpy(&flags, record + FLAGS, 4);
    if (flags & looksflag::DEMON_MARKED) {
        if (spared) {
            flags &= ~looksflag::DEMON_MARKED;
        } else {
            record[HOSTILITY] = (uint8_t)hostility::HOSTILE;
            for (int i = 0; i < 3; i++) {
                float v;
                std::memcpy(&v, record + SIZE + 4 * (size_t)i, 4);
                put(record, SIZE + 4 * (size_t)i, v * 1.1f);
            }
            flags &= ~looksflag::WILD;
        }
        put(record, FLAGS, flags);
    }
    return made;
}

// --- §7.4 Pets -------------------------------------------------------------------------------

CreatureExtras makePet(uint8_t *record, int64_t owner, const CreatureTables &tables,
                       const std::function<uint8_t *(int64_t id)> &ownerRecord) {
    using namespace creature;
    const int32_t race = get<int32_t>(record, RACE);
    // 1. Every pet of a race draws the same numbers.
    CrtRandom crt;
    crt.seed((uint32_t)race);
    auto next = [&crt] { return crt.next(); };
    const Draw rand(next);
    // 2. Its race's looks, over its appearance (keeping the flags it has).
    Appearance a;
    std::memcpy(a.bytes.data(), record + APPEARANCE, Appearance::SIZE);
    makeLooks(tables, race, false, rand, a);
    std::memcpy(record + APPEARANCE, a.bytes.data(), Appearance::SIZE);
    // 3. Its gear.
    Gear g;
    g.cls = record[CLASS];
    makeGear({record[HOSTILITY], race, get<int32_t>(record, LEVEL), get<uint32_t>(record, FLAGS), record[PEOPLE]}, rand, g);
    wearGear(record, g);
    // 4. Its owner.
    put(record, OWNER, owner);
    put(record, ROAM, PET_ROAM);
    // 5. The owner's pet.
    if (ownerRecord)
        if (uint8_t *o = ownerRecord(owner))
            put(o, PET_ID, get<int64_t>(record, ID));
    return {std::move(g.inventory), std::move(g.abilities), std::move(g.abilities2), {}};
}

MadeCreature makeInPlay(int64_t id, int32_t race, int32_t hostility, int32_t level, const int64_t position[3], Int2 region,
                        int64_t owner, const CreatureTables &tables, const std::function<float(const uint8_t *record)> &maxHealth) {
    using namespace creature;
    MadeCreature made{newCreature(id), {}};
    uint8_t *r = made.record.data();
    r[HOSTILITY] = (uint8_t)hostility;
    put(r, RACE, race);
    put(r, LEVEL, level);
    for (int k = 0; k < 3; k++)
        put(r, POSITION + 8 * (size_t)k, position[k]);
    put(r, REGION, region.x);
    put(r, REGION + 4, region.y);
    made.extras = makePet(r, owner, tables);
    if (maxHealth)
        put(r, HEALTH, maxHealth(r));
    return made;
}

std::vector<std::vector<std::array<uint8_t, creature::STACK_SIZE>>> inventoryRecords(const Inventory &inventory) {
    std::vector<std::vector<std::array<uint8_t, creature::STACK_SIZE>>> tabs(inventory.tabs.size());
    for (size_t t = 0; t < inventory.tabs.size(); t++)
        for (const Stack &s : inventory.tabs[t]) {
            std::array<uint8_t, creature::STACK_SIZE> stack{};
            std::memcpy(stack.data(), &s.quantity, 4);
            writeItem(stack.data() + 4, s.item);
            tabs[t].push_back(stack);
        }
    return tabs;
}

uint8_t arenaStage(uint8_t stage, const std::vector<const SpawnPoint *> &points, const std::function<bool(int64_t)> &dead) {
    std::map<int32_t, bool> done;
    int32_t last = 0;
    for (const SpawnPoint *p : points)
        if (p->requiredStage != 0 && dead(p->id))
            done[p->requiredStage] = true;
    for (const SpawnPoint *p : points)
        if (p->requiredStage != 0 && !dead(p->id))
            done[p->requiredStage] = false;
    for (const SpawnPoint *p : points)
        last = std::max(last, p->requiredStage);
    for (const auto &[s, d] : done)
        if (d)
            stage = (uint8_t)std::max<int32_t>(stage, s + 1);
    if (last != 0 && last < stage)
        stage = 0xff;
    return stage;
}

} // namespace worldgen
