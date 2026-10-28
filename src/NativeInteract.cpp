#include "NativeInteract.h"

#include "worldgen/Looks.h"
#include "NativeRules.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

// 0x2c3710, the interaction handler, and 0x2d3a10, a zone object opening or shutting.
// Kinds 1, 11, 12 and 13 follow docs/specs/skills-and-interactions.md; the others, and how
// the list is emptied, docs/specs/zone-object-interaction.md (§ numbers below are its).
namespace nativeinteract {
namespace {

template <class T> T get(const uint8_t *p, size_t o) {
    T v;
    std::memcpy(&v, p + o, sizeof v);
    return v;
}
template <class T> void set(uint8_t *p, size_t o, T v) { std::memcpy(p + o, &v, sizeof v); }

// --- Creatures the interactions make (kinds 1, 11 and 12) ---------------------------------

// Ids of creatures made in play count down from INT64_MIN + 1000: each new one is one below
// the smallest id in the world.
constexpr int64_t FIRST_MADE_ID = std::numeric_limits<int64_t>::min() + 1000;

constexpr int32_t BOMB_RACE = 0x90, BUBBLE_RACE = 0x11d;
constexpr uint8_t OBJECT_KIND = 7;   // hostility (+0x60) of bombs and bubbles
constexpr uint32_t SUMMONED = 0x800; // +0x80: a clone or a bubble (a clone can't cast another clone)
constexpr uint32_t BOMB_FLAGS = 0x14000;
constexpr int SKILL_LEVEL = 1000;    // 0x65690: every skill's level, in this build
constexpr uint8_t CLONE_AURA = 0x0f; // the clone's buff
constexpr uint8_t RALLIED = 0x11, PROVOKED = 0x12; // the heroic shout's buffs: allies, enemies
constexpr float RAND_MAX_F = 32767.0f;

int64_t idOf(const nativeai::Creature *c) { return get<int64_t>(c->record, 0x8); }
int64_t ownerOf(const nativeai::Creature *c) { return get<int64_t>(c->record, 0x1a0); }

int64_t newId(const World &w) {
    int64_t lowest = FIRST_MADE_ID;
    for (const nativeai::Creature *c : w.creatures)
        lowest = std::min(lowest, idOf(c));
    return lowest - 1;
}

// 0x29d0d0: the new creature joins the end of the world's list.
void addToWorld(World &w, nativeai::Creature *made) {
    w.creatures.push_back(made);
    if (w.added)
        w.added(made);
}

void addFlags(uint8_t *r, uint32_t flags) { set<uint32_t>(r, 0x80, get<uint32_t>(r, 0x80) | flags); }

// 0x53e80 with the race and no looks given: the race's appearance over the block at +0x78,
// keeping the flags already set.
// spec: open question: the spec draws no rand() for these creatures' looks, so the looks'
// draws are not the world's; all of them are 0, which gives the race's fixed looks.
void raceDefaults(uint8_t *r, int32_t race) {
    auto zero = [] { return 0; };
    worldgen::Appearance looks; // (keeping the flags it has)
    std::memcpy(looks.bytes.data(), r + 0x78, worldgen::Appearance::SIZE);
    worldgen::makeLooks(worldgen::sharedCreatureTables(), race, false, worldgen::Draw(zero), looks);
    std::memcpy(r + 0x78, looks.bytes.data(), worldgen::Appearance::SIZE);
}

// A sound request (world+0xa0) where a creature stands: which sound, its pitch, its volume.
void playSound(World &w, const uint8_t *creature, int32_t id, float pitch, float volume) {
    std::array<uint8_t, 0x30> s{};
    std::memcpy(s.data(), creature + 0x10, 24);
    set<int32_t>(s.data(), 0x18, id);
    set<float>(s.data(), 0x1c, pitch);
    set<float>(s.data(), 0x20, volume);
    w.sounds.push_back(s);
}

// Kind 1: an item used. Only a bomb (type 1, subtype 9) does anything here; the other items
// that queue this kind are dropped with the list.
void useItem(World &w, const nativeai::Creature *user, const nativeai::Interaction &in) {
    if (in[ITEM] != 1 || get<int32_t>(in.data(), ITEM + 4) != 9 || !w.construct)
        return;
    nativeai::Creature *bomb = w.construct(newId(w));
    if (!bomb)
        return;
    uint8_t *r = bomb->record;
    std::memcpy(r + 0x10, user->record + 0x10, 24); // where the user stands
    set<int32_t>(r, 0x64, BOMB_RACE);
    set<float>(r, 0x180, nativerules::maxHealth(r)); // full health, as the race has it
    r[0x60] = OBJECT_KIND;
    addFlags(r, BOMB_FLAGS);
    raceDefaults(r, BOMB_RACE);
    addToWorld(w, bomb);
}

// How far a skill of `level` has come: 0 at level 1, nearing 1 as it grows (0x61150).
float skillProgress(int level) { return 1.0f - 1.0f / ((float)(level - 1) / 50.0f + 1.0f); }

// Kind 11: the shadow clone, a copy of the caster that follows it.
void shadowClone(World &w, const nativeai::Creature *caster) {
    const int64_t casterId = idOf(caster);
    int owned = 0; // every creature it owns counts: clones, bubbles, pets, alive or dead
    for (const nativeai::Creature *c : w.creatures)
        owned += ownerOf(c) == casterId;
    if (owned >= MAX_OWNED || !w.construct)
        return;
    nativeai::Creature *clone = w.construct(newId(w));
    if (!clone)
        return;
    uint8_t *r = clone->record;
    const uint8_t *from = caster->record;
    // Everything from the position up to the buffs (0x3b7a0): place, motion, looks, class,
    // level, health, gear, name and so on. The id, buffs and lists stay the clone's own.
    std::memcpy(r + 0x10, from + 0x10, 0x980 - 0x10);
    r[0x60] = from[0x60] == 0 ? 4 : 1; // a player's clone sides with the players; others are monsters
    set<uint32_t>(r, 0xbcc, 0x480);     // its AI
    addFlags(r, SUMMONED);
    r[0x68] = 0; // no ability running (the copied ability time stays)
    set<int64_t>(r, 0x1a0, casterId);
    set<int32_t>(r, 0x144, get<int32_t>(from, 0x144));
    // Its aura (0x61150, 0x65460): from 0.5 to 1 strong and 15 to 30 s long by the skill's level.
    const float progress = skillProgress(SKILL_LEVEL);
    const float strength = 0.5f + 0.5f * progress;
    const float duration = (float)(int)(15000.0f + 15000.0f * progress);
    const nativebuffs::Buff aura = nativebuffs::make(CLONE_AURA, strength, duration, 0);
    nativebuffs::give(*clone, aura, false);
    w.buffNotes.push_back(nativebuffs::notice(idOf(clone), idOf(clone), aura, 1));
    addToWorld(w, clone);
}

// Kind 12: a water bubble where the caster stands. There is no limit on how many.
void waterBubble(World &w, const nativeai::Creature *caster) {
    if (!w.construct || !w.rand)
        return;
    nativeai::Creature *bubble = w.construct(newId(w));
    if (!bubble)
        return;
    uint8_t *r = bubble->record;
    const uint8_t *from = caster->record;
    set<int32_t>(r, 0x64, BUBBLE_RACE);
    r[0x60] = OBJECT_KIND;
    std::memcpy(r + 0x10, from + 0x10, 24);
    set<int64_t>(r, 0x1a0, idOf(caster));
    set<int32_t>(r, 0x144, get<int32_t>(from, 0x144));
    raceDefaults(r, BUBBLE_RACE);
    // Its power (+0x184): the caster's power value, by 75% to 125%.
    // spec: open question: that is 0x65c70 with selector 1, whose contributing entries the
    // spec leaves open; this uses selector 0's, the caster's spell power.
    const float powerScale = 0.75f + 0.5f * ((float)w.rand() / RAND_MAX_F);
    const std::vector<nativerules::Buff> stats = nativebuffs::forStats(*caster);
    set<float>(r, 0x184, nativerules::spellPower(from, stats.data(), stats.size()) * powerScale);
    // Its size: 75% to 150% of the race's.
    const float sizeScale = 0.75f + 0.75f * ((float)w.rand() / RAND_MAX_F);
    for (size_t axis = 0; axis < 3; axis++)
        set<float>(r, 0x84 + 4 * axis, get<float>(r, 0x84 + 4 * axis) * sizeScale);
    addFlags(r, SUMMONED);
    set<float>(r, 0x180, 1.0f);     // health
    set<float>(r, 0x188, 1.0f);     // mana
    set<uint32_t>(r, 0xbcc, 0x200); // its AI
    playSound(w, r, BUBBLE_SOUND, 0.8f + 0.1f * ((float)w.rand() / RAND_MAX_F), 1.0f);
    addToWorld(w, bubble);
}

// --- Kind 13: the heroic shout ----------------------------------------------------------

// The squared distance between two creatures, in blocks.
float distance2(const uint8_t *a, const uint8_t *b) {
    constexpr float BLOCKS_PER_DOT = 1.0f / 65536.0f;
    float sum = 0.0f;
    for (size_t axis = 0; axis < 3; axis++) {
        const int64_t dots = get<int64_t>(a, 0x10 + 8 * axis) - get<int64_t>(b, 0x10 + 8 * axis);
        const float d = (float)dots * BLOCKS_PER_DOT;
        sum += d * d;
    }
    return sum;
}
bool inShoutRange(const uint8_t *shouter, const uint8_t *other) {
    return distance2(shouter, other) < HEROIC_SHOUT_RANGE2;
}

// Whether `enemy` is angry at someone on the shouter's side within the shout's range.
bool angryAtNearAlly(const World &w, const uint8_t *shouter, const nativeai::Creature &enemy) {
    for (const auto &entry : enemy.aggro) {
        // spec: open question: the game does not check that this creature exists; one that
        // doesn't is skipped here.
        const nativeai::Creature *ally = w.byId ? w.byId(entry.first) : nullptr;
        if (ally && !nativerules::hostile(shouter, ally->record) && inShoutRange(shouter, ally->record))
            return true;
    }
    return false;
}

void heroicShout(World &w, const nativeai::Creature *shouter) {
    const uint8_t *s = shouter->record;
    const int64_t shouterId = idOf(shouter);
    playSound(w, s, HEROIC_SHOUT_SOUND, 1.0f, 1.0f);
    auto buff = [&](nativeai::Creature *c, uint8_t type, float strength, float duration) {
        const nativebuffs::Buff b = nativebuffs::make(type, strength, duration, shouterId);
        nativebuffs::give(*c, b, true);
        w.buffNotes.push_back(nativebuffs::notice(shouterId, idOf(c), b, 0));
    };
    // spec: open question: the game keeps the provoked enemies in a set ordered by memory
    // address; here they keep the world's order, which gives the same buffs (only the
    // notices' order can differ).
    std::vector<nativeai::Creature *> provoked;
    for (nativeai::Creature *c : w.creatures) {
        const uint8_t kind = c->record[0x60];
        if (kind == 7 || kind == 8)
            continue;
        if (!nativerules::hostile(s, c->record)) { // the shouter's side, the shouter included
            if (inShoutRange(s, c->record))
                buff(c, RALLIED, 0.05f, 10000.0f);
        } else if (angryAtNearAlly(w, s, *c) && std::find(provoked.begin(), provoked.end(), c) == provoked.end())
            provoked.push_back(c);
    }
    for (nativeai::Creature *c : provoked)
        buff(c, PROVOKED, 0.0f, 5000.0f);
}


// --- Shared pieces of the other kinds (zone-object-interaction.md §0) ---------------------

// A creature's record, by the fields these kinds read and write.
constexpr size_t C_ID = 0x08, C_POSITION = 0x10, C_INTENT = 0x40, C_RACE = 0x64, C_ABILITY = 0x68,
                 C_ABILITY_TIME = 0x6c, C_FLAGS = 0x80, C_TOGGLES = 0x128, C_LEVEL = 0x144, C_CLASS = 0x14c,
                 C_STEALTH = 0x18c, C_STAGE = 0x19c, C_PET = 0x9d8, C_PET_ORDER = 0x9e0;
// An item record (0xa0 bytes), by the fields the game compares (§0.7).
constexpr size_t I_TYPE = 0x00, I_SUBTYPE = 0x04, I_MODIFIER = 0x08, I_REGION_X = 0x0c, I_REGION_Y = 0x10,
                 I_RARITY = 0x14, I_FORMULA = 0x18, I_MATERIAL = 0x1c, I_ATTRIBUTES = 0x1d,
                 I_ATTRIBUTE_COUNT = 0x9d;
constexpr size_t ATTRIBUTE_SIZE = 4, MAX_ATTRIBUTES = (I_ATTRIBUTE_COUNT - I_ATTRIBUTES) / ATTRIBUTE_SIZE;
constexpr size_t ITEM_SIZE = 0xa0;
// A ground item (§0.3).
constexpr size_t G_POSITION = 0xa0, G_UNUSED_ONES = 0xb8, G_FLAGS = 0xc4, G_FLOAT = 0xc8, G_SLOT = 0xd0,
                 G_COUNT = 0xd4, G_OWNER = 0xd8;
constexpr uint8_t G_STAYS = 0x2; // flag: not picked up automatically; picking it up leaves it lying
// A pick-up notice (§6 step 5).
constexpr size_t N_WHO = 0x00, N_ITEM = 0x08, N_COUNT = 0xa8, N_POSITION = 0xb0, N_FLAGS = 0xc8;
// An inventory entry (creature +0x9f0, 0xa4 bytes): its count, then its item.
constexpr size_t ENTRY_COUNT = 0x00, ENTRY_ITEM = 0x04;

constexpr int64_t DOTS_PER_BLOCK = 65536;
constexpr int64_t ZONE_BLOCKS = 64;
constexpr float BLOCKS_PER_DOT = 1.0f / 65536.0f;

// Abilities, races and classes these kinds look for.
constexpr uint8_t SNEAKING = 0x50, LIFT_RISING = 0x99;
constexpr uint8_t RESTING_ABILITIES[] = {0x57, 0x58, 0x76}; // a creature in one of these may still sit down
constexpr int32_t SWITCH_RACE = 0xf5, GATE_KEEPER_RACE = 0xf6, LIFT_RACE = 0x12e;
constexpr uint32_t SWITCHED_BY_USE = 0x200000; // +0x80: a creature that activating sets going
constexpr uint8_t INNKEEPER = 0x84, ARENA_MASTER = 0xa1;
constexpr uint16_t TASK_DONE = 0x100; // +0x128
constexpr uint8_t STAGE_THANKED = 0xff;
constexpr int MAX_REWARD_TIER = 4;

// Zone object types (§10).
constexpr int32_t GATE = 0x10;
constexpr int32_t SEAT_TYPES[] = {0x1c, 0x1e, 0x1f, 0x20, 0x5f, 0x60, 0x73};
constexpr int32_t DOOR_TYPES[] = {3, 4, 5, 6, 0x14, 0x15, 0x54, 0x55, 0x56, 0x57, 0x69};
constexpr int32_t REWARD_TYPES[] = {0x15, 0x16};

// The gate opener (item type 0x18, subtype 5).
constexpr uint8_t SPECIAL_ITEM = 0x18;
constexpr int32_t GATE_OPENER_SUBTYPE = 5;
constexpr int64_t GATE_OPENER_REACH = 0x140000; // dots: the zones within 20 blocks
constexpr float GATE_KEEPER_RANGE2 = 400.0f;    // squared blocks
constexpr float GATE_RANGE2 = 64.0f;

constexpr float PICK_UP_RANGE2 = 16.0f;
constexpr float PICK_UP_PITCH_SPREAD = 0.1f;
constexpr uint8_t SHINY_TYPE = 0x0b;            // item 0x0b:0x25 sounds its own, an octave up
constexpr int32_t SHINY_SUBTYPE = 0x25;
constexpr int32_t SHINY_PICK_UP_SOUND = 0x2d;
constexpr float SHINY_PITCH = 2.0f;
constexpr int32_t PICK_UP_SOUND = 0x30;

constexpr float PET_CALL_RANGE2 = 40000.0f; // squared blocks: a pet further than 200 blocks comes to its owner

// The time of day, in ms since midnight.
constexpr float SIX_AM = 21600000.0f, SIX_PM = 64800000.0f, WAKE_UP = 25200000.0f;
constexpr float SLEEP_VOLUME = 0.75f;

// Door and gate sounds (§3 step 3).
constexpr int32_t NO_SOUND = -1;
constexpr int32_t DOOR_SOUND = 0x39, GATE_SOUND = 0x36, HATCH_SHUT_SOUND = 0x38, TRAPDOOR_SHUT_SOUND = 0x37;

template <class T, size_t N> bool oneOf(T v, const T (&list)[N]) { return std::find(list, list + N, v) != list + N; }

int draw(World &w) { return w.rand ? w.rand() : 0; }
nativeai::Creature *creatureById(const World &w, int64_t id) { return w.byId ? w.byId(id) : nullptr; }

using Position = std::array<int64_t, 3>;
Position positionAt(const uint8_t *record, size_t at) {
    return {get<int64_t>(record, at), get<int64_t>(record, at + 8), get<int64_t>(record, at + 16)};
}
void putPosition(uint8_t *record, size_t at, const Position &p) {
    for (size_t axis = 0; axis < 3; axis++)
        set<int64_t>(record, at + 8 * axis, p[axis]);
}
Position positionOf(const nativeai::Creature *c) { return positionAt(c->record, C_POSITION); }

// The squared distance between two positions, in blocks: (x² + y²) + z².
float squaredBlocks(const Position &a, const Position &b) {
    float sum = 0.0f;
    for (size_t axis = 0; axis < 3; axis++) {
        const float d = (float)(a[axis] - b[axis]) * BLOCKS_PER_DOT;
        sum += d * d;
    }
    return sum;
}

int64_t floorDiv(int64_t a, int64_t b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }
// 0x16e50: the zone a coordinate lies in.
int zoneOf(int64_t dots) { return (int)floorDiv(floorDiv(dots, DOTS_PER_BLOCK), ZONE_BLOCKS); }

// A sound request (§0.5) at `at`, with no owner.
void requestSound(std::vector<Sound> &sounds, const Position &at, int32_t id, float pitch, float volume) {
    Sound s{};
    putPosition(s.data(), 0x00, at);
    set<int32_t>(s.data(), 0x18, id);
    set<float>(s.data(), 0x1c, pitch);
    set<float>(s.data(), 0x20, volume);
    set<int64_t>(s.data(), 0x28, 0);
    sounds.push_back(s);
}

// §0.8: "a random spot around Q", up to a block either way. The first draw goes to y and
// the second to x; the height stays.
int64_t awayFrom(int r) { return (int64_t)((1.0f - ((float)r + (float)r) / RAND_MAX_F) * (float)DOTS_PER_BLOCK); }
Position randomSpotAround(World &w, const Position &q) {
    const int first = draw(w), second = draw(w);
    return {q[0] + awayFrom(second), q[1] + awayFrom(first), q[2] + (int64_t)0.0f};
}
// A drop's random angle, in degrees: the draw after the spot's.
float randomAngle(World &w) { return (float)draw(w) * 360.0f / RAND_MAX_F; }

void layItem(World &w, const uint8_t *item, const Position &at, float angle, int count, nativeai::Creature *by,
             const uint8_t *from) {
    if (w.lay)
        w.lay(item, at.data(), angle, count, by, from);
}

nativerules::HasBuff hasBuffOf(const nativeai::Creature &c) {
    return [&c](int type) {
        for (const auto &b : c.buffs)
            if (b[0] == (uint8_t)type)
                return true;
        return false;
    };
}

// --- Kind 5: using a zone object (§2) -----------------------------------------------------

bool isNight(float timeOfDay) { return timeOfDay >= SIX_PM || timeOfDay <= SIX_AM; } // (a NaN time is day)

// §2.2: a night's sleep at the innkeeper's. True when the record was for that.
bool sleepAtInn(World &w, nativeai::Creature *sleeper, const nativeai::Interaction &in) {
    const nativeai::Creature *host = creatureById(w, get<int64_t>(in.data(), TARGET));
    if (!host || host->record[C_CLASS] != INNKEEPER || !isNight(w.timeOfDay))
        return false;
    requestSound(w.sounds, positionOf(sleeper), SLEEP_SOUND, 1.0f, SLEEP_VOLUME);
    w.sleepNotices.push_back(0);
    w.day += 1;
    w.timeOfDay = WAKE_UP;
    return true;
}

// Whether a creature may sit down: doing nothing, already resting, or its ability is over.
bool isFree(const nativeai::Creature &c) {
    const uint8_t ability = c.record[C_ABILITY];
    if (ability == 0 || oneOf(ability, RESTING_ABILITIES))
        return true;
    const int duration = nativerules::abilityDuration(c.record, hasBuffOf(c));
    return get<float>(c.record, C_ABILITY_TIME) > (float)duration;
}

// §2.4: a free seat or bed is taken and its user stops where it is.
void takeSeat(World &w, nativeai::Creature *user, nativeai::Object &seat, const ZoneKey &zone) {
    const int64_t before = get<int64_t>(seat.data(), OBJECT_USER), id = idOf(user);
    set<int64_t>(seat.data(), OBJECT_USER, id);
    if (before != id)
        w.changedZones.insert(zone);
    user->record[C_ABILITY] = 0;
    for (size_t axis = 0; axis < 3; axis++)
        set<float>(user->record, C_INTENT + 4 * axis, 0.0f);
}

// §2.5: a door, gate or hatch with no item to take opens if shut, shuts if open.
void toggleDoor(World &w, nativeai::Object &door, const ZoneKey &zone) {
    if (!oneOf(get<int32_t>(door.data(), OBJECT_TYPE), DOOR_TYPES))
        return;
    const uint8_t before = door[OBJECT_STATE];
    openOrShut(door, before == 0 ? 1 : 0, w.sounds);
    if (door[OBJECT_STATE] != before)
        w.changedZones.insert(zone);
}

// §2.6: an object handed the item it waits for opens, gives what it holds and, for the
// reward-giving types, a reward as rich as the item handed in.
void giveItem(World &w, const nativeai::Interaction &in, nativeai::Object &o, const ZoneKey &zone, int index) {
    if (o[OBJECT_STATE] == 0 || !sameItem(in.data() + ITEM, o.data() + OBJECT_ITEM))
        return;
    openOrShut(o, 0, w.sounds);
    w.changedZones.insert(zone);
    const Position at = positionAt(o.data(), OBJECT_POSITION);
    const uint8_t *held = w.contents ? w.contents(zone.first, zone.second, index) : nullptr;
    if (held && held[I_TYPE] != 0) {
        const Position spot = randomSpotAround(w, at);
        const float angle = randomAngle(w);
        layItem(w, held, spot, angle, 1, nullptr, nullptr);
    }
    if (oneOf(get<int32_t>(o.data(), OBJECT_TYPE), REWARD_TYPES) && w.reward)
        w.reward(at.data(), o[OBJECT_ITEM + I_RARITY], nullptr);
}

void useObject(World &w, nativeai::Creature *user, const nativeai::Interaction &in) {
    uint8_t *r = user->record;
    if (r[C_ABILITY] == SNEAKING) { // §2.1: whatever follows, sneaking ends
        r[C_ABILITY] = 0;
        set<float>(r, C_STEALTH, 0.0f);
    }
    if (sleepAtInn(w, user, in))
        return;
    // §2.3: the object named, in a loaded zone.
    const ZoneKey zone{get<int32_t>(in.data(), ZONE_X), get<int32_t>(in.data(), ZONE_Y)};
    std::vector<nativeai::Object> *objects = w.objects ? w.objects(zone.first, zone.second) : nullptr;
    const int index = get<int32_t>(in.data(), OBJECT);
    if (!objects || index < 0 || index >= (int)objects->size())
        return;
    nativeai::Object &o = (*objects)[(size_t)index];
    const int32_t type = get<int32_t>(o.data(), OBJECT_TYPE);
    if (oneOf(type, SEAT_TYPES) && get<int64_t>(o.data(), OBJECT_USER) == 0 && isFree(*user))
        takeSeat(w, user, o, zone);
    else if (o[OBJECT_ITEM + I_TYPE] == 0)
        toggleDoor(w, o, zone);
    else
        giveItem(w, in, o, zone, index);
}

// --- Kind 2: the gate opener (§4) ---------------------------------------------------------

void gateOpener(World &w, const nativeai::Creature *user, const nativeai::Interaction &in) {
    if (in[ITEM + I_TYPE] != SPECIAL_ITEM || get<int32_t>(in.data(), ITEM + I_SUBTYPE) != GATE_OPENER_SUBTYPE)
        return;
    const Position here = positionOf(user);
    // The gate keepers nearby wake.
    for (nativeai::Creature *c : w.creatures) {
        uint8_t *r = c->record;
        if (squaredBlocks(here, positionOf(c)) <= GATE_KEEPER_RANGE2 && get<int32_t>(r, C_RACE) == GATE_KEEPER_RACE &&
            r[C_STAGE] == 0) {
            r[C_STAGE] = 1;
            requestSound(w.sounds, here, GATE_OPENER_SOUND, 1.0f, 1.0f);
        }
    }
    // The shut gates within 8 blocks open, in the zones within 20 blocks (x outer, y inner).
    if (!w.objects)
        return;
    for (int zx = zoneOf(here[0] - GATE_OPENER_REACH); zx <= zoneOf(here[0] + GATE_OPENER_REACH); zx++)
        for (int zy = zoneOf(here[1] - GATE_OPENER_REACH); zy <= zoneOf(here[1] + GATE_OPENER_REACH); zy++) {
            std::vector<nativeai::Object> *objects = w.objects(zx, zy);
            if (!objects)
                continue;
            for (nativeai::Object &o : *objects) {
                if (get<int32_t>(o.data(), OBJECT_TYPE) != GATE || o[OBJECT_STATE] == 0 ||
                    !(squaredBlocks(positionAt(o.data(), OBJECT_POSITION), here) < GATE_RANGE2))
                    continue;
                const uint8_t before = o[OBJECT_STATE];
                openOrShut(o, 0, w.sounds);
                if (o[OBJECT_STATE] != before)
                    w.changedZones.insert({zx, zy});
                requestSound(w.sounds, here, GATE_OPENER_SOUND, 1.0f, 1.0f);
            }
        }
}

// --- Kind 3: activating a creature (§5) ---------------------------------------------------

// §5 case 3: a resident whose task is done says thanks: once per creature, a reward near it
// and everything it carries (which it keeps).
void thank(World &w, nativeai::Creature *resident) {
    uint8_t *r = resident->record;
    r[C_STAGE] = STAGE_THANKED;
    set<uint16_t>(r, C_TOGGLES, (uint16_t)(get<uint16_t>(r, C_TOGGLES) & ~TASK_DONE));
    if (w.rewarded && !w.rewarded->insert(idOf(resident)).second)
        return; // (thanked before)
    const Position at = positionOf(resident);
    const Position spot = randomSpotAround(w, at);
    const int tier = std::min(get<int32_t>(r, C_LEVEL) + 1, MAX_REWARD_TIER);
    if (w.reward)
        w.reward(spot.data(), tier, r);
    for (const auto &tab : resident->inventory)
        for (const auto &entry : tab) {
            const Position place = randomSpotAround(w, at);
            const float angle = randomAngle(w);
            layItem(w, entry.data() + ENTRY_ITEM, place, angle, get<int32_t>(entry.data(), ENTRY_COUNT), nullptr, r);
        }
}

void activate(World &w, const nativeai::Interaction &in) {
    nativeai::Creature *target = creatureById(w, get<int64_t>(in.data(), TARGET));
    if (!target)
        return;
    uint8_t *r = target->record;
    const int32_t race = get<int32_t>(r, C_RACE);
    if (race == SWITCH_RACE || (get<uint32_t>(r, C_FLAGS) & SWITCHED_BY_USE)) { // a lever, a lift...
        r[C_STAGE] = 1;
        if (race == LIFT_RACE) { // a lift starts rising
            r[C_ABILITY] = LIFT_RISING;
            set<float>(r, C_ABILITY_TIME, 0.0f);
        }
    } else if (r[C_CLASS] == ARENA_MASTER && r[C_STAGE] != STAGE_THANKED) {
        // An even stage waits for the next round; an odd one is a round under way. (The game
        // also prints the new stage to its console.)
        if (r[C_STAGE] % 2 == 0)
            r[C_STAGE]++;
    } else if ((get<uint16_t>(r, C_TOGGLES) & TASK_DONE) && r[C_STAGE] == 0) {
        thank(w, target);
    }
}

// --- Kind 7: picking up a ground item (§6) ------------------------------------------------


void pickUp(World &w, const nativeai::Creature *picker, const nativeai::Interaction &in) {
    const ZoneKey zone{get<int32_t>(in.data(), ZONE_X), get<int32_t>(in.data(), ZONE_Y)};
    std::vector<GroundItem> *items = w.groundItems ? w.groundItems(zone.first, zone.second) : nullptr;
    const int index = get<int32_t>(in.data(), OBJECT);
    if (!items || index < 0 || index >= (int)items->size())
        return;
    GroundItem &g = (*items)[(size_t)index];
    const Position lying = positionAt(g.data(), G_POSITION), here = positionOf(picker);
    if (!(squaredBlocks(lying, here) <= PICK_UP_RANGE2) || !sameItem(g.data(), in.data() + ITEM))
        return; // out of reach (or not a number), or the item has changed
    float pitch = (float)draw(w) * PICK_UP_PITCH_SPREAD / RAND_MAX_F + 1.0f;
    int32_t sound = PICK_UP_SOUND;
    if (g[I_TYPE] == SHINY_TYPE && get<int32_t>(g.data(), I_SUBTYPE) == SHINY_SUBTYPE)
        sound = SHINY_PICK_UP_SOUND, pitch *= SHINY_PITCH;
    requestSound(w.sounds, here, sound, pitch, 1.0f);
    // The notice gives the item to the one who picked it up.
    PickUpNotice n{};
    set<int64_t>(n.data(), N_WHO, idOf(picker));
    std::memcpy(n.data() + N_ITEM, g.data(), ITEM_SIZE);
    set<int16_t>(n.data(), N_COUNT, get<int16_t>(g.data(), G_COUNT));
    putPosition(n.data(), N_POSITION, lying);
    set<int32_t>(n.data(), N_FLAGS, (int32_t)(int8_t)g[G_FLAGS]);
    w.pickUps.push_back(n);
    if (g[G_FLAGS] & G_STAYS)
        return;
    w.changedGroundZones.insert(zone);
    emptyGroundItem(g);
}

// --- Kind 8: dropping an item (§7) --------------------------------------------------------

// Up to a block either way from one rand(), the other way round from §0.8's spot.
int64_t dropOffset(int r) { return (int64_t)((((float)r + (float)r) / RAND_MAX_F - 1.0f) * (float)DOTS_PER_BLOCK); }

void dropItem(World &w, nativeai::Creature *dropper, const nativeai::Interaction &in) {
    const int first = draw(w), second = draw(w);
    const Position here = positionOf(dropper);
    const Position at{here[0] + dropOffset(second), here[1] + dropOffset(first), here[2] + (int64_t)0.0f};
    layItem(w, in.data() + ITEM, at, 0.0f, 1, dropper, dropper->record);
    requestSound(w.sounds, here, DROP_SOUND, 1.0f, 1.0f);
}

// --- Kind 10: calling the pet (§8) --------------------------------------------------------

void callPet(World &w, nativeai::Creature *owner) {
    if (nativeai::Creature *pet = creatureById(w, get<int64_t>(owner->record, C_PET))) {
        pet->aggro.clear(); // it forgets whom it was fighting
        if (squaredBlocks(positionOf(pet), positionOf(owner)) > PET_CALL_RANGE2)
            putPosition(pet->record, C_POSITION, positionOf(owner));
    }
    owner->damage.clear();
    set<int64_t>(owner->record, C_PET_ORDER, 0);
}

void handle(World &w, nativeai::Creature *c, const nativeai::Interaction &in) {
    switch (in[KIND]) {
    case USE_ITEM: useItem(w, c, in); break;
    case GATE_OPENER: gateOpener(w, c, in); break;
    case ACTIVATE: activate(w, in); break;
    case USE_OBJECT: useObject(w, c, in); break;
    case PICK_UP: pickUp(w, c, in); break;
    case DROP: dropItem(w, c, in); break;
    case CALL_PET: callPet(w, c); break;
    case SHADOW_CLONE: shadowClone(w, c); break;
    case WATER_BUBBLE: waterBubble(w, c); break;
    case HEROIC_SHOUT: heroicShout(w, c); break;
    default: break; // the game passes over any other kind
    }
}

// §3 step 3: the sound of an object of `type` going to `state`.
int32_t swingSound(int32_t type, uint8_t state) {
    if (type < 8)
        return DOOR_SOUND;
    if (type == 8 || type == 9 || type == GATE)
        return GATE_SOUND;
    if (type == 0x11)
        return state != 0 ? HATCH_SHUT_SOUND : NO_SOUND;
    if (type == 0x12)
        return state != 0 ? TRAPDOOR_SHUT_SOUND : NO_SOUND;
    return DOOR_SOUND;
}

} // namespace

void emptyGroundItem(GroundItem &g) {
    g.fill(0);
    std::memset(g.data() + G_UNUSED_ONES, 0xff, 8);
    set<int32_t>(g.data(), G_SLOT, -1);
    set<int16_t>(g.data(), G_COUNT, 1);
    set<int64_t>(g.data(), G_OWNER, 0);
}

bool openOrShut(nativeai::Object &object, uint8_t state, std::vector<Sound> &sounds) {
    uint8_t *o = object.data();
    if (o[OBJECT_STATE] == state)
        return false;
    // A swing turned back half-way starts from where it stood.
    const float swing = FULL_SWING - get<float>(o, OBJECT_SWING);
    o[OBJECT_STATE] = state;
    set<float>(o, OBJECT_SWING, 0.0f > swing ? 0.0f : swing);
    const int32_t sound = swingSound(get<int32_t>(o, OBJECT_TYPE), state);
    if (sound != NO_SOUND)
        requestSound(sounds, positionAt(o, OBJECT_POSITION), sound, 1.0f, 1.0f);
    return true;
}

bool sameItem(const uint8_t *a, const uint8_t *b) {
    auto same = [&](size_t at, size_t size) { return std::memcmp(a + at, b + at, size) == 0; };
    if (!same(I_TYPE, 1) || !same(I_SUBTYPE, 4) || !same(I_MODIFIER, 4) || !same(I_REGION_X, 4) ||
        !same(I_REGION_Y, 4) || !same(I_FORMULA, 4) || !same(I_MATERIAL, 1) || !same(I_ATTRIBUTE_COUNT, 1) ||
        !same(I_RARITY, 1))
        return false;
    const size_t count = std::min<size_t>(a[I_ATTRIBUTE_COUNT], MAX_ATTRIBUTES);
    return same(I_ATTRIBUTES, count * ATTRIBUTE_SIZE);
}

nativeai::Interaction fresh(uint8_t kind) {
    nativeai::Interaction in{};
    set<int64_t>(in.data(), ZONE_X, -1); // both zone coordinates -1, written as one 8-byte value
    set<int32_t>(in.data(), OBJECT, -1);
    set<int16_t>(in.data(), ALWAYS_ONE, 1);
    in[KIND] = kind;
    return in;
}

void step(World &w, nativeai::Creature *c) {
    if (w.client) // a client sends its player's interactions to the server instead
        return;
    for (const nativeai::Interaction &in : c->interactions)
        handle(w, c, in);
    c->interactions.clear(); // handled or not
}

} // namespace nativeinteract
