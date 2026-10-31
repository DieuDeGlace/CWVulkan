// A creature's upkeep, written from docs/specs/creature-upkeep.md. The steps run in the
// spec's order (section 3); each says which step it is. All arithmetic is single precision,
// one rounded operation at a time, in the order the spec gives, unless a comment says
// "in double": the results must match the game's bit for bit.
#include "NativeUpkeep.h"

#include "GameMath.h"
#include "NativeBuffs.h"
#include "NativeCrt.h"
#include "NativeRules.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace nativeupkeep {
namespace {

// ---------------------------------------------------------------------------------------
// Where things are in a creature record (byte offsets).

constexpr size_t ID = 0x08;                  // i64
constexpr size_t POSITION = 0x10;            // i64 x, y, z (1/65536 block)
constexpr size_t ORIENTATION = 0x28;         // f32 x3; the third is a heading in degrees
constexpr size_t HEADING = 0x30;             // f32
constexpr size_t VELOCITY = 0x34;            // f32 x3
constexpr size_t VERTICAL_VELOCITY = 0x3c;   // f32
constexpr size_t INTENT = 0x40;              // f32 x3: where it wants to go
constexpr size_t MOVE_FLAGS = 0x5c;          // u32
constexpr size_t HOSTILITY = 0x60;           // u8
constexpr size_t RACE = 0x64;                // i32
constexpr size_t ABILITY = 0x68;             // u8, 0 for none
constexpr size_t ABILITY_TIME = 0x6c;        // f32 ms
constexpr size_t FLAGS = 0x80;               // u32
constexpr size_t SIZE_X = 0x84, SIZE_Y = 0x88, SIZE_Z = 0x8c; // f32 size factors
constexpr size_t STATE = 0x128;              // u16 state flags
constexpr size_t DODGE = 0x12c;              // f32 ms left of the dodge
constexpr size_t STUN = 0x130;               // f32 ms left of the stun
constexpr size_t STAMINA_LOCK = 0x134;       // f32 ms before stamina regenerates again
constexpr size_t TIMER_A = 0x138;            // f32 ms (meaning not established)
constexpr size_t TIMER_B = 0x13c;            // f32 ms (meaning not established)
constexpr size_t RECOVERING = 0x140;         // f32 <= 0, recovers toward 0
constexpr size_t CLASS = 0x14c;              // u8
constexpr size_t SPECIALISATION = 0x14d;     // u8
constexpr size_t REGION = 0x150;             // i32 x2
constexpr size_t CARRIED_OFFSET = 0x174;     // f32 x3: offset from the carrier (blocks)
constexpr size_t RISE = 0x17c;               // f32: race 0x12e's rise (shares the offset's z)
constexpr size_t HEALTH = 0x180;             // f32
constexpr size_t BLOCK_POWER = 0x184;        // f32 0..1
constexpr size_t MANA = 0x188;               // f32 0..1
constexpr size_t STEALTH = 0x18c;            // f32 0..1
constexpr size_t RISING = 0x19c;             // u8: race 0x12e is rising
constexpr size_t OWNER = 0x1a0;              // i64 id
constexpr size_t LINKED = 0x1a8;             // i64 id: mount, carrier or focus target
constexpr size_t RIGHT_HAND = 0x6d8;         // item (0xa0 bytes)
constexpr size_t SHARED_A = 0x990;           // f32: copied from mount, owner or focus target
constexpr size_t DECAYING = 0x994;           // f32: eases toward 0
constexpr size_t SHARED_B = 0x998;           // f32
constexpr size_t SHARED_C = 0x99c;           // f32
constexpr size_t STAMINA = 0x9a0;            // f32 0..1
constexpr size_t DODGE_BEFORE = 0xadc;       // f32: the dodge timer before its countdown
constexpr size_t HIT_POINT = 0xae0;          // i64 x3: where poison hits land
constexpr size_t SECOND_POSITION = 0xb10;    // i64 x3: kept in step with POSITION
constexpr size_t OWNER_COPY = 0xb34;         // f32 x3: copied from the owner (hostility 6)
constexpr size_t OWNER_COPY_RESET = 0xb58;   // 4 bytes zeroed (hostility 6)
constexpr size_t SWIMMING_SKILL = 0xb64;     // i32 skill points

// Movement flags (MOVE_FLAGS).
constexpr uint32_t ON_GROUND = 0x1, IN_WATER = 0x2, MOVE_BIT_2 = 0x4, MOVE_BIT_4 = 0x10, MOVE_BIT_8 = 0x100;
// Creature flags (FLAGS).
constexpr uint32_t ZONE_BOUND = 0x1000;
// State flags (STATE).
constexpr uint16_t STATE_KEEP_ZONE = 0x2, STATE_GLIDE_DRAIN = 0x40, STATE_ZONE_CLEARED = 0x100,
                   STATE_IDLE_STILL = 0x400;

// Hostility kinds.
constexpr uint8_t PLAYER = 0, RIDES_WITH_OWNER = 6, ORPHANED = 7, SUMMONED = 8;

// Races.
constexpr int32_t BOMB = 0x90, WATER_BUBBLE = 0x11d, RISER = 0x12e;

// Abilities, by what this step does with them.
enum : uint8_t {
    NO_ABILITY = 0,
    GUARD = 0x09,
    HEAL_ALLY = 0x1d,
    DIVE = 0x37,
    SNEAK = 0x50,
    WALK_DRAINING = 0x51,
    LONG_GUARD = 0x5f,
    GUARD_6E = 0x6e,
    OWNER_RIDES = 0x76,
    RIDE = 0x77,
    GLIDE = 0x78,
    LOCAL_DRAIN = 0x7a,
    FOLLOW_OWNER = 0x80,
    LAND_ENDS = 0x8a,
    CIRCLE_TARGET = 0x8c,
    CARRIED = 0x91,
    MANA_DRAIN = 0x92,
    KEEPS_STAMINA = 0x93,
    TIMED_RECOVERY = 0x9f,
};

// Buff types.
constexpr uint8_t BUFF_UNSTUN = 0x01, BUFF_HIDDEN = 0x03, BUFF_POISON = 0x04, BUFF_EVADED = 0x0b,
                  BUFF_SHADOW_CLONE = 0x0f, BUFF_HEAL = 0x11, BUFF_KEEP_A = 0x1e, BUFF_KEEP_B = 0x1f,
                  BUFF_GROUP_FIGHT = 0x20, BUFF_MANA_COST = 0x22;

// Sounds.
constexpr int32_t SOUND_ROLL = 0x1a, SOUND_SPLASH = 0x2d, SOUND_DROWN = 0x2f, SOUND_DROP = 0x3c;

// Damage-number kinds (byte +0x45 of a number).
constexpr uint8_t NUMBER_PLAIN = 0, NUMBER_MISSED = 4;

// Units.
constexpr float UNITS_PER_BLOCK = 65536.0f;
constexpr float RAND_MAX_F = 32767.0f;
constexpr float MS_PER_DAY = 86400000.0f;

// Rules.
constexpr float GROUP_BUFF_MS = 10000.0f;
constexpr float RISE_CEILING = 100.0f, RISE_PER_MS = 0.01f;
constexpr float HEIGHT_EPSILON = 0.01f;
constexpr float MOUNT_HEIGHT = 0.75f;
constexpr float FOCUS_RADIUS = -0.5f;
constexpr float BOMB_DRAIN = 0.00025f, BUBBLE_DRAIN = 0.00005f, BUBBLE_EMPTY = 0.1f;
constexpr float RECOVERY_DIVISOR = 180.0f;
constexpr float DECAY_RATE = 0.0075f;
constexpr float KEEP_THRESHOLD = 0.00001f;
constexpr int POISON_TICK_MS = 400, HEAL_TICK_MS = 1000, DROWN_TICK_MS = 500;
constexpr float DIVE_PUSH_DOWN = -60.0f;
constexpr int DIVE_EFFECT_MS = 400, DIVE_MIDDLE_MS = 100, DIVE_RECOVERY_MS = 400;
constexpr float MANA_REGEN = 0.0001f, MANA_DRAIN_92 = 0.0002f, MANA_DRAIN_IDLE = 0.00005f;
constexpr float BLOCK_POWER_MS = 2000.0f;
constexpr float CARRIED_TIMER = 100.0f;
constexpr float STAMINA_LOCK_AFTER_DRAIN = 800.0f;
constexpr float LOCAL_DRAIN_RATE = 0.00025f, LONG_GUARD_DRAIN = 0.00025f;
constexpr float WALK_DRAIN_THRESHOLD = 0.01f, WALK_DRAIN_FACTOR = 0.25f;
constexpr float GLIDE_DRAIN = 0.0001625f, GLIDE_MIN_STAMINA = 0.001f, GLIDE_GAIN_FACTOR = -0.1f;
constexpr float STAMINA_REGEN = 0.0001f;
constexpr double DROWN_SHARE = 0.05;
constexpr float STEALTH_RATE = 0.0005f, STEALTH_IDLE_GAIN = 0.00005f, STEALTH_TO_MANA = 0.00025f;
constexpr float STEALTH_LIGHT_FACTOR = 0.75f, STEALTH_SPEED_PENALTY = 0.05f;
constexpr int SNEAK_SKILL_LEVEL = 1000; // the skill level query always answers this
constexpr float DODGE_SOUND_AT = 590.0f;
constexpr float EVADE_MANA = 0.25f, EVADE_BUFF_MS = 30000.0f;

// ---------------------------------------------------------------------------------------
// Small helpers.

template <class T> T get(const uint8_t *record, size_t offset) {
    T v;
    std::memcpy(&v, record + offset, sizeof v);
    return v;
}
template <class T> void put(uint8_t *record, size_t offset, T v) { std::memcpy(record + offset, &v, sizeof v); }

// Float to integer toward zero as the processor does it: out of range gives the
// "integer indefinite" value instead of undefined behaviour.
int32_t trunc32(float v) { return _mm_cvttss_si32(_mm_set_ss(v)); }
int64_t trunc64(float v) { return _mm_cvttss_si64(_mm_set_ss(v)); }

int64_t blocksToUnits(float blocks) { return trunc64(blocks * UNITS_PER_BLOCK); }

// A rand() draw spread over [base, base + range].
float randomIn(World &w, float range, float base) {
    const int r = w.rand ? w.rand() : 0;
    return ((float)r * range) / RAND_MAX_F + base;
}

nativeai::Creature *creatureById(World &w, int64_t id) { return w.byId ? w.byId(id) : nullptr; }

void addSound(World &w, const uint8_t *record, int32_t id, float pitch, int64_t owner) {
    std::array<uint8_t, 0x30> s{};
    std::memcpy(&s[0x00], record + POSITION, 24);
    put(s.data(), 0x18, id);
    put(s.data(), 0x1c, pitch);
    put(s.data(), 0x20, 1.0f); // volume
    put(s.data(), 0x28, owner);
    w.sounds.push_back(s);
}

// A damage number shown at `at`'s position.
void addNumber(World &w, int64_t from, int64_t to, float value, const uint8_t *at, uint8_t kind) {
    std::array<uint8_t, 0x48> n{};
    put(n.data(), 0x00, from);
    put(n.data(), 0x08, to);
    put(n.data(), 0x10, value);
    std::memcpy(&n[0x20], at + POSITION, 24);
    n[0x45] = kind;
    w.numbers.push_back(n);
}

float maxHealth(const nativeai::Creature &c) {
    const std::vector<nativerules::Buff> buffs = nativebuffs::forStats(c);
    return nativerules::maxHealth(c.record, buffs.data(), buffs.size());
}

bool hasBuff(const nativeai::Creature &c, uint8_t type) { return nativebuffs::find(c, type) != nullptr; }

// The current ability's whole duration, in ms, as a float to compare with the clock.
float abilityDuration(const nativeai::Creature &c) {
    const nativerules::HasBuff has = [&](int type) { return hasBuff(c, (uint8_t)type); };
    return (float)nativerules::abilityDuration(c.record, has);
}

// Where a rider sits on its carrier, in blocks, by the carrier's race (0x65040).
void mountOffset(int32_t race, float &x, float &y, float &z) {
    x = 0.0f, y = 0.0f, z = 0.0f;
    switch (race) {
    case 0x1a: case 0x1e: z = -0.9f; break;
    case 0x21: z = -0.2f; break;
    case 0x4a: case 0xc8: case 0xc9: z = -0.3f; break;
    case 0x4b: case 0x117: z = 0.1f; break;
    case 0x97: z = -0.35f; break;
    case 0x125: case 0x126: z = -0.4f; break;
    case 0x127: z = 0.4f; break;
    case 0xd2: y = -0.3f, z = -0.3f; break;
    case 0xd3: z = -0.6f; break;
    case 0xd5: z = 0.25f; break;
    case 0xd7: z = 0.4f; break;
    case 0x106: z = -1.0f; break;
    case 0x107: z = -2.0f; break;
    case 0x10e: z = -1.6f; break;
    default: break;
    }
}

// The height difference that keeps a rider standing on its carrier, in position units.
int64_t heightGap(const uint8_t *upper, const uint8_t *lower) {
    return trunc64((((get<float>(upper, SIZE_Z) * 0.5f) - (get<float>(lower, SIZE_Z) * 0.5f)) + HEIGHT_EPSILON) *
                   UNITS_PER_BLOCK);
}

// Places `rider`'s position (and its second position) at `carrier`'s, shifted by the
// offsets (already in position units) and lowered by `gap`.
void placeOn(uint8_t *rider, const uint8_t *carrier, const int64_t offset[3], int64_t gap) {
    for (size_t base : {POSITION, SECOND_POSITION}) {
        put(rider, base + 0, offset[0] + get<int64_t>(carrier, base + 0));
        put(rider, base + 8, offset[1] + get<int64_t>(carrier, base + 8));
        put(rider, base + 16, offset[2] + (get<int64_t>(carrier, base + 16) - gap));
    }
}

void copyFloat(uint8_t *to, const uint8_t *from, size_t offset) { std::memcpy(to + offset, from + offset, 4); }

// 1 plus the stamina regeneration of every worn item (0x647b0).
float staminaFactor(const uint8_t *record) { return nativerules::staminaFactor(record); }

// Mana regeneration factor (0x5f8d0): the mana-cost buff slows it.
float manaFactor(const nativeai::Creature &c) {
    const nativebuffs::Buff *b = nativebuffs::find(c, BUFF_MANA_COST);
    return b ? 1.0f - nativebuffs::strength(*b) * 0.1f : 1.0f;
}

// Whether the creature lives on mana rather than losing it (0x5edd0): mages, two special
// classes, two races, and anyone holding a magic weapon in the right hand.
bool usesMana(const uint8_t *record) {
    const uint8_t cls = record[CLASS];
    const int32_t race = get<int32_t>(record, RACE);
    if (cls == 3 || cls == 0x97 || cls == 0x98 || race == 0x75 || race == 0x56)
        return true;
    const uint32_t subtype = get<uint32_t>(record, RIGHT_HAND + 4);
    return record[RIGHT_HAND] == 3 && (subtype == 10 || subtype == 11 || subtype == 12);
}

// Whether the creature is guarding (0x5ed30).
bool isGuarding(const nativeai::Creature &c) {
    const uint8_t *r = c.record;
    if (get<float>(r, STAMINA_LOCK) > 0.0f)
        return false;
    switch (r[ABILITY]) {
    case GUARD: case GUARD_6E: return true;
    case LONG_GUARD: return get<float>(r, ABILITY_TIME) < abilityDuration(c);
    case 0x3c: case 0x40: return r[CLASS] == 1 && r[SPECIALISATION] == 1;
    default: return false;
    }
}

// How fast swimming drains stamina (0x51190); only players tire.
float swimDrain(const uint8_t *record) {
    if (record[HOSTILITY] != PLAYER)
        return 0.0f;
    const float skill = (float)get<int32_t>(record, SWIMMING_SKILL);
    return (1.0f - (1.0f - 1.0f / (skill / 20.0f + 1.0f))) * 0.1f;
}

// The stealth gain at a skill level (0x656a0).
float sneakBase(int level) {
    return (1.0f - 1.0f / ((float)(level - 1) / 50.0f + 1.0f)) * 0.4f + 0.8f;
}

// Whether the inventory holds a special item (type 0x18) of `subtype` for the creature's
// region (0x5e890 with the item comparison 0x4df60).
bool carriesSpecialItem(const nativeai::Creature &c, uint32_t subtype) {
    constexpr uint8_t SPECIAL_ITEM = 0x18;
    std::array<uint8_t, 0xa0> wanted{};
    wanted[0] = SPECIAL_ITEM;
    put(wanted.data(), 4, subtype);
    std::memcpy(&wanted[0xc], c.record + REGION, 8);
    auto same = [&](const uint8_t *item) {
        if (item[0] != wanted[0] || item[0x14] != wanted[0x14] || item[0x1c] != wanted[0x1c] ||
            item[0x9d] != wanted[0x9d])
            return false;
        for (size_t o : {size_t{4}, size_t{8}, size_t{0xc}, size_t{0x10}, size_t{0x18}})
            if (std::memcmp(item + o, &wanted[o], 4) != 0)
                return false;
        return std::memcmp(item + 0x20, &wanted[0x20], 4 * (size_t)item[0x9d]) == 0;
    };
    int64_t count = 0;
    for (const auto &tab : c.inventory)
        for (const auto &slot : tab)
            if (same(slot.data() + 4))
                count += get<int32_t>(slot.data(), 0);
    return count > 0;
}

// The reward for evading an attack (0x4ee50), given to the one who evaded: a dodging
// specialist gets mana back and a buff.
void rewardEvasion(World &w, nativeai::Creature &v) {
    uint8_t *r = v.record;
    if (get<float>(r, DODGE) == 0.0f || r[CLASS] != 4 || r[SPECIALISATION] != 1)
        return;
    float mana = get<float>(r, MANA) + EVADE_MANA;
    if (mana > 1.0f)
        mana = 1.0f;
    put(r, MANA, mana);
    const nativebuffs::Buff b = nativebuffs::make(BUFF_EVADED, 0.0f, EVADE_BUFF_MS, 0);
    nativebuffs::give(v, b, false);
    const int64_t id = get<int64_t>(r, ID);
    w.buffNotes.push_back(nativebuffs::notice(id, id, b, 0));
}

// ---------------------------------------------------------------------------------------
// The steps.

struct Upkeep {
    World &w;
    nativeai::Creature &c;
    uint8_t *r;
    float dt;
    bool authoritative;
    bool local;

    int64_t id() const { return get<int64_t>(r, ID); }
    float f(size_t o) const { return get<float>(r, o); }
    void set(size_t o, float v) { put(r, o, v); }
    uint8_t ability() const { return r[ABILITY]; }
    void endAbility() { r[ABILITY] = NO_ABILITY; }
    uint32_t moveFlags() const { return get<uint32_t>(r, MOVE_FLAGS); }
    float intentSquared() const {
        const float x = f(INTENT), y = f(INTENT + 4), z = f(INTENT + 8);
        return ((x * x) + (y * y)) + (z * z);
    }
    void capHealth() {
        if (f(HEALTH) > maxHealth(c))
            set(HEALTH, maxHealth(c));
    }

    // Step 1a: a monster that several living players are angry at gets more health.
    void groupFightBuff() {
        int players = 0;
        for (const auto &[otherId, anger] : c.aggro) {
            const nativeai::Creature *o = creatureById(w, otherId);
            if (o && get<float>(o->record, HEALTH) > 0.0f && o->record[HOSTILITY] == PLAYER)
                players++;
        }
        if (players <= 1)
            return;
        const float strength = (float)(players - 1) * 0.5f + 1.0f;
        const nativebuffs::Buff *now = nativebuffs::find(c, BUFF_GROUP_FIGHT);
        if (now && nativebuffs::strength(*now) == strength)
            return;
        const nativebuffs::Buff b = nativebuffs::make(BUFF_GROUP_FIGHT, strength, GROUP_BUFF_MS, id());
        nativebuffs::give(c, b, true);
        w.buffNotes.push_back(nativebuffs::notice(id(), id(), b, 1));
    }

    // Step 1c: a summon whose owner died is on its own.
    void orphanSummon() {
        const int64_t owner = get<int64_t>(r, OWNER);
        if (r[HOSTILITY] == SUMMONED && owner != 0 && w.dead && w.dead(owner))
            r[HOSTILITY] = ORPHANED;
    }

    // Step 1d: a zone-bound creature notes when every creature of its linked zones is dead.
    void zoneCleared() {
        if (!(get<uint32_t>(r, FLAGS) & ZONE_BOUND) || !w.zoneLinks)
            return;
        std::vector<World::Link> links;
        if (!w.zoneLinks(get<int64_t>(r, POSITION), get<int64_t>(r, POSITION + 8), links))
            return;
        uint16_t state = get<uint16_t>(r, STATE);
        if (links.empty()) {
            if (state & STATE_KEEP_ZONE)
                state &= (uint16_t)~STATE_ZONE_CLEARED;
            else
                state |= STATE_ZONE_CLEARED;
            put(r, STATE, state);
            return;
        }
        for (const World::Link &link : links) {
            if (!link.loaded)
                return;
            for (int64_t other : link.ids)
                if (!w.dead || !w.dead(other))
                    return;
        }
        put(r, STATE, (uint16_t)(state | STATE_ZONE_CLEARED));
    }

    // Step 1e: race 0x12e rises while there is air or water above it, clicking every 2 units.
    void rise() {
        if (get<int32_t>(r, RACE) != RISER || r[RISING] == 0)
            return;
        const int64_t z = get<int64_t>(r, POSITION + 16) + blocksToUnits(f(RISE)) + 0x10000;
        const nativeai::Block above = w.blockAt ? w.blockAt(get<int64_t>(r, POSITION), get<int64_t>(r, POSITION + 8), z)
                                                : nativeai::Block{0xc8, 0xc8, 0xc8, 0, 1, 0};
        const uint8_t type = above[4];
        if ((type != 0 && type != 2) || f(RISE) > RISE_CEILING)
            return;
        const float was = f(RISE);
        const float now = dt * RISE_PER_MS + was;
        if (trunc32(now) / 2 != trunc32(was) / 2)
            addSound(w, r, SOUND_DROP, randomIn(w, 0.4f, 0.8f), 0);
        set(RISE, now);
    }

    // Step 2: a rider sits on its mount, or stops riding when the mount can't carry it.
    void ride() {
        if (ability() != RIDE)
            return;
        const int64_t mountId = get<int64_t>(r, LINKED);
        const nativeai::Creature *mount = mountId ? creatureById(w, mountId) : nullptr;
        if (!mount || !(get<float>(mount->record, HEALTH) > 0.0f) || !(get<float>(mount->record, STUN) <= 0.0f)) {
            endAbility();
            return;
        }
        const uint8_t *m = mount->record;
        float ox, oy, oz;
        mountOffset(get<int32_t>(m, RACE), ox, oy, oz);
        oz = oz + get<float>(m, SIZE_Z) * MOUNT_HEIGHT;
        const int64_t offset[3] = {blocksToUnits(ox), blocksToUnits(oy), blocksToUnits(oz)};
        placeOn(r, m, offset, heightGap(m, r));
        for (size_t o : {ORIENTATION, ORIENTATION + 4, HEADING, SHARED_A, SHARED_B, SHARED_C})
            copyFloat(r, m, o);
        std::memset(r + VELOCITY, 0, INTENT + 12 - VELOCITY); // velocity and intent
    }

    // Step 3: a carried creature follows its carrier, and keeps the carrier's timer up.
    void carried() {
        if (ability() != CARRIED)
            return;
        const int64_t carrierId = get<int64_t>(r, LINKED);
        nativeai::Creature *carrier = carrierId ? creatureById(w, carrierId) : nullptr;
        if (!carrier || !(get<float>(carrier->record, HEALTH) > 0.0f))
            return;
        uint8_t *k = carrier->record;
        for (size_t base : {POSITION, SECOND_POSITION})
            for (size_t i = 0; i < 3; i++)
                put(r, base + 8 * i, get<int64_t>(k, base + 8 * i) + blocksToUnits(f(CARRIED_OFFSET + 4 * i)));
        if (CARRIED_TIMER > get<float>(k, TIMER_A))
            put(k, TIMER_A, CARRIED_TIMER);
    }

    // Step 4: circling a focus target, beside it on its left (by its heading).
    void circleTarget() {
        if (ability() != CIRCLE_TARGET)
            return;
        const float duration = abilityDuration(c);
        const int64_t targetId = get<int64_t>(r, LINKED);
        const nativeai::Creature *target = targetId ? creatureById(w, targetId) : nullptr;
        const bool targetAlive = target && get<float>(target->record, HEALTH) > 0.0f;
        if (!(duration > f(ABILITY_TIME)) || !targetAlive) {
            // (The game also prints "FOCUS <id>" to its console here.)
            endAbility();
            if (targetAlive)
                std::memcpy(r + POSITION, target->record + POSITION, 24);
            return;
        }
        const uint8_t *t = target->record;
        float ox, oy, oz;
        mountOffset(get<int32_t>(t, RACE), ox, oy, oz);
        oz = oz + get<float>(t, SIZE_Z) * MOUNT_HEIGHT;
        // In double: the target's heading in radians, a quarter turn on.
        const double PI = 3.141592653589793, HALF_PI = 1.5707963267948966;
        const float angle = (float)((double)get<float>(t, HEADING) * PI / 180.0 + HALF_PI);
        const float sx = get<float>(t, SIZE_X), sy = get<float>(t, SIZE_Y);
        const float radius = (sx > sy ? sy : sx) * FOCUS_RADIUS;
        const float ax = nativecrt::cosf(angle) * radius;
        const float ay = radius * nativecrt::sinf(angle);
        const float az = radius * 0.0f;
        const int64_t offset[3] = {blocksToUnits(ax) + blocksToUnits(ox), blocksToUnits(ay) + blocksToUnits(oy),
                                   blocksToUnits(az) + blocksToUnits(oz)};
        placeOn(r, t, offset, heightGap(t, r));
        for (size_t o : {ORIENTATION, ORIENTATION + 4, HEADING, SHARED_A})
            copyFloat(r, t, o);
        std::memset(r + SHARED_B, 0, 8);
        std::memset(r + VELOCITY, 0, 0x58 - VELOCITY); // velocity, intent and the three after
        set(DODGE, 0.0f);
    }

    // Step 5: hostility kind 6 rides along with its owner while the owner uses 0x76.
    void rideWithOwner() {
        if (r[HOSTILITY] != RIDES_WITH_OWNER)
            return;
        const nativeai::Creature *owner = creatureById(w, get<int64_t>(r, OWNER));
        if (!owner || owner->record[ABILITY] != OWNER_RIDES)
            return;
        const uint8_t *o = owner->record;
        const int64_t gap = heightGap(r, o); // (this creature's size first, unlike riding)
        put(r, SECOND_POSITION, get<int64_t>(o, SECOND_POSITION));
        put(r, SECOND_POSITION + 8, get<int64_t>(o, SECOND_POSITION + 8));
        put(r, SECOND_POSITION + 16, get<int64_t>(o, SECOND_POSITION + 16) + gap);
        std::memcpy(r + OWNER_COPY, o + OWNER_COPY, 12);
        put(r, POSITION, get<int64_t>(o, POSITION));
        put(r, POSITION + 8, get<int64_t>(o, POSITION + 8));
        put(r, POSITION + 16, get<int64_t>(o, POSITION + 16) + gap);
        for (size_t f4 : {ORIENTATION, ORIENTATION + 4, HEADING})
            copyFloat(r, o, f4);
        put<uint32_t>(r, OWNER_COPY_RESET, 0);
        copyFloat(r, o, SHARED_A);
        copyFloat(r, o, SHARED_B);
        set(SHARED_C, (get<float>(o, SHARED_C) * get<float>(o, SIZE_X)) / f(SIZE_X));
        std::memset(r + VELOCITY, 0, INTENT + 12 - VELOCITY);
    }

    // Step 6: bombs burn down and water bubbles run dry.
    void selfDrain() {
        const float health = f(HEALTH);
        if (!(health > 0.0f) || !authoritative)
            return;
        const int32_t race = get<int32_t>(r, RACE);
        if (race == BOMB) {
            set(HEALTH, health - ((maxHealth(c) * BOMB_DRAIN) * dt));
            if (0.0f > f(HEALTH)) {
                set(HEALTH, 0.0f);
                set(ABILITY_TIME, 0.0f);
            }
        } else if (race == WATER_BUBBLE) {
            set(MANA, f(MANA) - dt * BUBBLE_DRAIN);
            if (BUBBLE_EMPTY > f(MANA)) {
                set(HEALTH, 0.0f);
                set(ABILITY_TIME, 0.0f);
                addSound(w, r, SOUND_SPLASH, 1.0f, 0);
            }
        }
    }

    // Step 7: the short timers.
    void timers() {
        for (size_t o : {STAMINA_LOCK, TIMER_A, TIMER_B}) {
            set(o, f(o) - dt);
            if (0.0f > f(o))
                set(o, 0.0f);
        }
        if (0.0f > f(RECOVERING)) {
            set(RECOVERING, ((dt * 0.001f) / RECOVERY_DIVISOR) + f(RECOVERING));
            if (f(RECOVERING) > 0.0f)
                set(RECOVERING, 0.0f);
        }
        const float k = gamemath::blendFactor((double)dt, DECAY_RATE);
        const float old = f(DECAYING);
        set(DECAYING, (k * (0.0f - old)) + old);
    }

    // A poison tick: the buff's strength as a hit from its source.
    void poisonHit(const nativebuffs::Buff &b) {
        Hit h;
        h.target = id();
        const int64_t source = nativebuffs::source(b);
        h.attacker = creatureById(w, source) ? source : 0;
        h.damage = nativebuffs::strength(b);
        std::memcpy(h.where, r + HIT_POINT, sizeof h.where);
        h.type = 6;
        w.hits.push_back(h);
        if (w.hit)
            w.hit(h);
    }

    // What a live buff does this step.
    void buffEffect(size_t index, float left) {
        const nativebuffs::Buff b = c.buffs[index];
        switch (b[0]) {
        case BUFF_UNSTUN:
            if (f(STUN) > 0.0f)
                set(STUN, 0.0f);
            break;
        case BUFF_POISON: // a hit every 400 ms of the time left
            if (authoritative && trunc32(left) / POISON_TICK_MS != trunc32(dt + left) / POISON_TICK_MS &&
                nativebuffs::strength(b) > 0.0f)
                poisonHit(b);
            break;
        case BUFF_HEAL: // heals a share of the maximum health every second
            if (authoritative && trunc32(left) / HEAL_TICK_MS != trunc32(dt + left) / HEAL_TICK_MS) {
                const float heal = maxHealth(c) * nativebuffs::strength(b);
                addNumber(w, nativebuffs::source(b), id(), -heal, r, NUMBER_PLAIN);
                set(HEALTH, heal + f(HEALTH));
                capHealth();
            }
            break;
        default: break;
        }
    }

    // Step 8: buffs count down, act, and expire.
    void buffs() {
        if (!(0.0f < f(HEALTH)))
            c.buffs.clear();
        std::vector<size_t> expired;
        // By index: a hit applied during the walk may add buffs, which the walk then visits.
        for (size_t i = 0; i < c.buffs.size(); i++) {
            float left;
            {
                nativebuffs::Buff &b = c.buffs[i];
                left = nativebuffs::duration(b) - dt;
                put(b.data(), 8, left);
            }
            const uint8_t type = c.buffs[i][0];
            if (type == BUFF_KEEP_A || type == BUFF_KEEP_B) { // kept until their strength runs out
                if (KEEP_THRESHOLD > nativebuffs::strength(c.buffs[i]))
                    expired.push_back(i);
                else
                    buffEffect(i, left);
            } else if (0.0f < left) {
                buffEffect(i, left);
            } else {
                if (type == BUFF_SHADOW_CLONE) // a shadow clone lasts as long as its aura
                    set(HEALTH, 0.0f);
                expired.push_back(i);
            }
        }
        // Remove the expired ones, keeping the others in order.
        std::sort(expired.begin(), expired.end());
        for (size_t k = expired.size(); k-- > 0;)
            if (expired[k] < c.buffs.size())
                c.buffs.erase(c.buffs.begin() + (std::ptrdiff_t)expired[k]);
    }

    // Step 9: a non-player's health keeps its share of the maximum when the maximum changes.
    void followMaxHealth(float oldMax) {
        if (!authoritative || r[HOSTILITY] == PLAYER)
            return;
        const float newMax = maxHealth(c);
        if (!(oldMax > 0.0f))
            return;
        const float health = (newMax / oldMax) * f(HEALTH);
        set(HEALTH, newMax > health ? health : newMax);
    }

    // Step 10: the stun counts down (below zero too) and stops whatever is under way.
    void stun() {
        set(STUN, f(STUN) - dt);
        if (f(STUN) > 0.0f) {
            set(ABILITY_TIME, 0.0f);
            endAbility();
        }
    }

    // Step 11: the ability clock. A negative time with an ability set is a queued start.
    void abilityClock() {
        const bool runs = local || (r[HOSTILITY] != PLAYER && authoritative);
        if (!runs || !(f(HEALTH) > 0.0f) || !(f(ABILITY_TIME) >= 0.0f || ability() == NO_ABILITY))
            return;
        set(ABILITY_TIME, dt + f(ABILITY_TIME));
        // The dive pushes down between the end of its effect and the end of its middle part.
        const float t = f(ABILITY_TIME);
        if (ability() == DIVE && t > (float)DIVE_EFFECT_MS &&
            (float)(DIVE_EFFECT_MS + DIVE_MIDDLE_MS + DIVE_RECOVERY_MS) > t &&
            (float)(DIVE_EFFECT_MS + DIVE_MIDDLE_MS) > t)
            set(VERTICAL_VELOCITY, DIVE_PUSH_DOWN);
    }

    // Step 12: when an attack is over, those it struck but did not count evaded it.
    void attackOver() {
        if (ability() != NO_ABILITY)
            return;
        for (int64_t otherId : c.struck) {
            if (c.counted.count(otherId))
                continue;
            nativeai::Creature *v = creatureById(w, otherId);
            if (!v)
                continue;
            rewardEvasion(w, *v);
            addNumber(w, id(), otherId, 0.0f, v->record, NUMBER_MISSED);
        }
        c.counted.clear();
        c.struck.clear();
    }

    // Step 13: following an owner ends when the owner is gone.
    void followOwner() {
        if (ability() != FOLLOW_OWNER)
            return;
        const nativeai::Creature *owner = creatureById(w, get<int64_t>(r, OWNER));
        if (!owner || !(0.0f < get<float>(owner->record, HEALTH)))
            endAbility();
    }

    // Step 14: mana fills for casters, drains for others.
    void mana() {
        const uint8_t a = ability();
        float m = f(MANA);
        if (a == MANA_DRAIN) {
            m = m - dt * MANA_DRAIN_92;
            if (!(0.0f < m))
                m = 0.0f;
        } else if (usesMana(r)) {
            // Spells that are still being cast hold the mana where it is.
            static constexpr uint8_t CASTING[] = {0x20, 0x22, 0x23, 0x26, 0x2c, 0x2e, 0x30, 0x69, 0x6a, 0x86, 0x87, 0x88};
            const bool casting = std::find(std::begin(CASTING), std::end(CASTING), a) != std::end(CASTING) &&
                                 abilityDuration(c) > f(ABILITY_TIME);
            if (a != HEAL_ALLY && !casting)
                m = manaFactor(c) * (dt * MANA_REGEN) + m;
        } else {
            // Abilities that keep a non-caster's mana from draining.
            static constexpr uint8_t KEEPING[] = {0x09, 0x19, 0x1a, 0x1c, 0x3c, 0x40, 0x41};
            if (std::find(std::begin(KEEPING), std::end(KEEPING), a) == std::end(KEEPING))
                m = m - dt * MANA_DRAIN_IDLE;
        }
        if (0.0f > m)
            m = 0.0f;
        if (m > 1.0f)
            m = 1.0f;
        set(MANA, m);
    }

    // Step 15: block power recovers while not guarding.
    void blockPower() {
        if (r[HOSTILITY] == ORPHANED)
            return;
        if (!isGuarding(c) || ability() == LONG_GUARD || ability() == TIMED_RECOVERY)
            set(BLOCK_POWER, dt / BLOCK_POWER_MS + f(BLOCK_POWER));
        if (f(BLOCK_POWER) > 1.0f)
            set(BLOCK_POWER, 1.0f);
    }

    // Step 16: wanting to move clears the "still" flag and ends abilities 0x57..0x5a.
    void movementIntent() {
        if (intentSquared() > 0.0f)
            put(r, STATE, (uint16_t)(get<uint16_t>(r, STATE) & ~STATE_IDLE_STILL));
        const uint8_t a = ability();
        if (a >= 0x57 && a <= 0x5a && intentSquared() > 0.0f)
            endAbility();
    }

    // Step 17: the local player's ability 0x7a drains stamina while moving in the air
    // without the special item 0x12.
    void localDrain() {
        constexpr uint32_t ITEM_SUBTYPE = 0x12;
        if (ability() != LOCAL_DRAIN || !local || carriesSpecialItem(c, ITEM_SUBTYPE) || (moveFlags() & MOVE_BIT_8) ||
            !(intentSquared() > 0.0f) || ability() == GLIDE || (moveFlags() & (ON_GROUND | MOVE_BIT_2)) != MOVE_BIT_2)
            return;
        set(STAMINA, f(STAMINA) - dt * LOCAL_DRAIN_RATE);
        if (!(0.0f < f(STAMINA))) {
            set(STAMINA, 0.0f);
            endAbility();
            set(STAMINA_LOCK, STAMINA_LOCK_AFTER_DRAIN);
        }
    }

    // Step 18: ability 0x8a ends on landing.
    void landing() {
        if (ability() == LAND_ENDS && (moveFlags() & ON_GROUND))
            endAbility();
    }

    // Step 19: stealth grows while hiding (less in light and when moving fast) and fades
    // otherwise.
    void stealth() {
        const uint8_t a = ability(), cls = r[CLASS], spec = r[SPECIALISATION];
        const bool dodging = f(DODGE) > 0.0f;
        const bool hiding = a == SNEAK || (cls == 4 && spec == 0 && dodging) ||
                            (cls == 2 && spec == 0 && (a == 0x19 || a == 0x1a || a == 0x1c || dodging) &&
                             SNEAK_SKILL_LEVEL > 0);
        if (hiding) {
            if (moveFlags() & IN_WATER) {
                if (a == SNEAK)
                    endAbility();
                return;
            }
            // Night is 1 at midnight, 0 at noon.
            const float u = nativecrt::powf(((w.day + w.day) / MS_PER_DAY) - 1.0f, 2.0f);
            const float night = nativecrt::powf(1.0f - u, 2.0f);
            const int light = w.light ? w.light((int)(get<int64_t>(r, POSITION) >> 16),
                                                (int)(get<int64_t>(r, POSITION + 8) >> 16),
                                                (int)(get<int64_t>(r, POSITION + 16) >> 16))
                                      : 0;
            const float lit = ((float)light / 255.0f) * night;
            const float base = sneakBase(SNEAK_SKILL_LEVEL);
            float rate;
            if (f(DODGE) > 0.0f) {
                rate = base - lit * STEALTH_LIGHT_FACTOR;
            } else {
                const float vx = f(VELOCITY), vy = f(VELOCITY + 4), vz = f(VELOCITY + 8);
                const float speed = std::sqrt(((vx * vx) + (vy * vy)) + (vz * vz));
                rate = (base - lit * STEALTH_LIGHT_FACTOR) - speed * STEALTH_SPEED_PENALTY;
            }
            float s = rate * (dt * STEALTH_RATE) + f(STEALTH);
            if (0.0f > s)
                s = 0.0f;
            if (s > 1.0f)
                s = 1.0f;
            set(STEALTH, s);
        } else if (get<uint16_t>(r, STATE) & STATE_IDLE_STILL) {
            float s = dt * STEALTH_IDLE_GAIN + f(STEALTH);
            if (s > 1.0f)
                s = 1.0f;
            set(STEALTH, s);
        } else {
            float s = f(STEALTH) - dt * STEALTH_RATE;
            if (0.0f > s)
                s = 0.0f;
            set(STEALTH, s);
        }
    }

    // Steps 20 and 21: buff 3 hides completely; hiding gives mana.
    void stealthMana() {
        if (hasBuff(c, BUFF_HIDDEN))
            set(STEALTH, 1.0f);
        float m = manaFactor(c) * ((dt * STEALTH_TO_MANA) * f(STEALTH)) + f(MANA);
        if (m > 1.0f)
            m = 1.0f;
        set(MANA, m);
    }

    // Step 22, case 5's drowning: a share of the maximum health every 500 ms of world time.
    void drown() {
        const float loss = (float)((double)maxHealth(c) * DROWN_SHARE); // in double
        set(HEALTH, f(HEALTH) - loss);
        addSound(w, r, SOUND_DROWN, randomIn(w, 0.1f, 0.5f), 0);
        if (0.0f > f(HEALTH))
            set(HEALTH, 0.0f);
        addNumber(w, 0, id(), loss, r, NUMBER_PLAIN);
    }

    // Step 22: stamina. The first case that matches applies.
    void stamina() {
        const uint8_t a = ability();
        const uint32_t move = moveFlags();
        float s = f(STAMINA);
        if (a == GLIDE && !(move & ON_GROUND)) {
            if (get<uint16_t>(r, STATE) & STATE_GLIDE_DRAIN) {
                s = s - dt * GLIDE_DRAIN;
            } else if (s > GLIDE_MIN_STAMINA) { // falling while gliding wins some back
                float gain = f(VERTICAL_VELOCITY) * GLIDE_GAIN_FACTOR;
                if (!(gain > 0.0f))
                    gain = 0.0f;
                if (!(1.0f > gain))
                    gain = 1.0f;
                s = (dt * STAMINA_REGEN) * gain + s;
            }
        } else if (a == LOCAL_DRAIN) {
            // (drained in step 17)
        } else if (a == WALK_DRAINING && intentSquared() > WALK_DRAIN_THRESHOLD) {
            s = s - (dt * 0.001f) * WALK_DRAIN_FACTOR;
        } else if (a == LONG_GUARD) {
            s = s - dt * LONG_GUARD_DRAIN;
            if (!(0.0f < s))
                endAbility();
        } else if (local && (move & (IN_WATER | MOVE_BIT_4)) == IN_WATER) { // the local player swimming
            s = s - swimDrain(r) * (dt * 0.001f);
            const float playerHealth = w.local ? get<float>(w.local, HEALTH) : 0.0f;
            if (!(0.0f < s) && playerHealth > 0.0f &&
                trunc32(w.time - dt) / DROWN_TICK_MS != trunc32(w.time) / DROWN_TICK_MS)
                drown();
        } else if (a == KEEPS_STAMINA) {
            // no change
        } else if (f(STAMINA_LOCK) == 0.0f && (a != TIMED_RECOVERY || f(ABILITY_TIME) > abilityDuration(c))) {
            s = staminaFactor(r) * (dt * STAMINA_REGEN) + s;
        }
        if (0.0f > s)
            s = 0.0f;
        if (s > 1.0f)
            s = 1.0f;
        set(STAMINA, s);
    }

    // Steps 23 and 24: the dodge's roll sound, then its countdown.
    void dodge() {
        const float d = f(DODGE);
        set(DODGE_BEFORE, d);
        const bool simulated = (authoritative && r[HOSTILITY] != PLAYER) || local;
        if (simulated && d >= DODGE_SOUND_AT && DODGE_SOUND_AT > (d - dt))
            addSound(w, r, SOUND_ROLL, randomIn(w, 0.2f, 0.9f), id());
        if (f(DODGE) != 0.0f)
            set(DODGE, f(DODGE) - dt);
        if (0.0f > f(DODGE))
            set(DODGE, 0.0f);
    }

    void run() {
        const float oldMax = maxHealth(c); // step 0
        if (authoritative) {                // step 1
            groupFightBuff();
            capHealth();
            orphanSummon();
            zoneCleared();
            rise();
        }
        ride();
        carried();
        circleTarget();
        rideWithOwner();
        selfDrain();
        timers();
        buffs();
        followMaxHealth(oldMax);
        stun();
        abilityClock();
        attackOver();
        followOwner();
        mana();
        blockPower();
        movementIntent();
        localDrain();
        landing();
        stealth();
        stealthMana();
        stamina();
        dodge();
    }
};

} // namespace

std::array<float, 3> ridingOffset(int32_t race) {
    float x, y, z;
    mountOffset(race, x, y, z);
    return {x, y, z};
}
float ridingOffsetZ(int32_t race) {
    return ridingOffset(race)[2];
}

void upkeep(World &world, nativeai::Creature *creature, float dt) {
    Upkeep u{world, *creature, creature->record, dt, !world.client, world.local == creature->record};
    u.run();
}

} // namespace nativeupkeep
