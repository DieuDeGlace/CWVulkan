// The game's creature AI, written from docs/specs/creature-ai.md. Section numbers (§) below
// are that spec's; the game's routines are cited by RVA for reference only.
#include "NativeAI.h"

#include "worldgen/StructureTables.h"
#include "NativeBuffs.h"
#include "NativeCrt.h"
#include "NativeRules.h"

#include <cmath>
#include <cstring>

namespace nativeai {

Entry &Entry::operator=(const Entry &o) {
    if (this == &o)
        return *this;
    raw = o.raw;
    waiting = o.waiting;
    steps = o.steps;
    open = o.open;
    path = o.path;
    taken = o.taken;
    other = o.other;
    // The best step points into our own copy of the map.
    best = o.best == o.steps.end() ? steps.end() : steps.find(o.best->first);
    return *this;
}

namespace {

// --- Record layouts ----------------------------------------------------------------------

// The creature record (§1.3).
constexpr size_t ID = 0x08;              // i64
constexpr size_t POSITION = 0x10;        // i64 x3, dots
constexpr size_t HEADING = 0x30;         // f32, degrees
constexpr size_t VELOCITY = 0x34;        // f32 x3
constexpr size_t INTENT = 0x40;          // f32 x3: where it wants to go
constexpr size_t MOVE_FLAGS = 0x5c;      // u32
constexpr size_t KIND = 0x60;            // u8: hostility kind
constexpr size_t RACE = 0x64;            // i32
constexpr size_t ABILITY = 0x68;         // u8
constexpr size_t ABILITY_TIME = 0x6c;    // f32, ms
constexpr size_t COMBO = 0x70;           // i32
constexpr size_t FLAGS = 0x80;           // u32: creature flags
constexpr size_t SIZE = 0x84;            // f32 x3, blocks
constexpr size_t LOOK_HEAD = 0x90;       // u16 appearance ids, used by races 0x76 and 0x77
constexpr size_t LOOK_BODY = 0x94;
constexpr size_t LOOK_HANDS = 0x98;
constexpr size_t LOOK_FEET = 0x9a;
constexpr size_t STATE = 0x128;          // u16: state flags
constexpr size_t DODGE_TIMER = 0x12c;    // f32
constexpr size_t STUN = 0x130;           // f32
constexpr size_t CLASS = 0x14c;          // u8
constexpr size_t SPECIALISATION = 0x14d; // u8
constexpr size_t CHARGE = 0x158;         // f32: how far a strong attack is charged
constexpr size_t AIM = 0x174;            // f32 x3: offset to what it looks at, blocks
constexpr size_t HEALTH = 0x180;         // f32
constexpr size_t MANA = 0x188;           // f32
constexpr size_t STEALTH = 0x18c;        // f32
constexpr size_t OWNER = 0x1a0;          // i64
constexpr size_t LINKED = 0x1a8;         // i64: the creature it acts on
constexpr size_t HOME = 0x1b8;           // i64 x3
constexpr size_t ITEM = 0x1d8;           // 0xa0 bytes: the item in use
constexpr size_t ITEM_SUBTYPE = 0x1dc;   // i32
constexpr size_t ITEM_MODIFIER = 0x1e0;  // i32
constexpr size_t SLOT_A = 0x638;         // equipped weapon A: type u8, subtype i32 at +4
constexpr size_t SLOT_B = 0x6d8;         // equipped weapon B
constexpr size_t STAMINA = 0x9a0;        // f32
constexpr size_t MOUNT = 0x9d8;          // i64
constexpr size_t ANSWERING = 0x9e0;      // i64: whose call it answers
constexpr size_t TRAVEL_TIME = 0x9e8;    // f32: skill 0x37's flight
constexpr size_t AIM_POINT = 0xae0;      // i64 x3: remembered aim point (staff users)
constexpr size_t TARGET_POINT = 0xb10;   // i64 x3
constexpr size_t AI_FLAGS = 0xbcc;       // u32 (§3.2)

// Movement flags (+0x5c).
constexpr uint32_t ON_GROUND = 0x1, IN_WATER = 0x2, OPENS_DOORS = 0x4000;
// Creature flags (+0x80).
constexpr uint32_t SHOOTS_FROM_BODY = 1u << 2, LEAPER = 0x02 | 0x10, ELITE = 0x40200, NO_COWER = 1u << 11,
                   CALLS_OUT = 1u << 13, PEACEFUL = 1u << 15, NO_DODGE = 1u << 18, AI_OFF = 1u << 22;
// State flags (+0x128).
constexpr uint16_t NIGHT = 1u << 0, ASLEEP = 1u << 1, STATE_HOSTILE = 1u << 5, HURRY = 1u << 6, SPARED = 1u << 7,
                   CONTROLLED = 1u << 9, PROVOKED = 1u << 12;
constexpr uint16_t BUSY = ASLEEP | (1u << 4); // asleep or otherwise out of it
// AI flags (+0xbcc, §3.2).
constexpr uint32_t STAYS_HOME = 0x2, IDLE_STAND = 0x4, IDLE_SLEEP = 0x8, IDLE_STROLL = 0x10, IDLE_SEAT = 0x20,
                   COMPANION = 0x80, IDLE_DANCE = 0x100, ATTACHES = 0x200, RIDES_ALONG = 0x400, IDLE_CELL = 0x1000,
                   GUARDS = 0x4000;

// Hostility kinds (+0x60).
constexpr uint8_t PLAYER = 0, MONSTER = 1, TOWNSPERSON = 3, PET = 6, OBJECT_A = 7, OBJECT_B = 8;

// Races and classes the AI singles out.
constexpr int32_t RACE_LEASHED = 0x114, RACE_MOUNT = 0x106, RACE_PEACEFUL = 0x12f;
constexpr int32_t RACE_HEAD_74 = 0x74, RACE_HEAD_76 = 0x76, RACE_HEAD_77 = 0x77, RACE_65 = 0x65, RACE_68 = 0x68,
                  RACE_123 = 0x123;
constexpr uint8_t CLASS_STILL = 0x99, CLASS_SWIMMER = 0x9f, CLASS_FLYER = 0xa0, CLASS_PEACEFUL = 0xa1;

// Abilities the AI starts or tests.
constexpr int GREET = 1, DRINK = 0x54, COWER = 0x55, CALLED = 0x56, SLEEP = 0x57, SIT = 0x58, HEAL_OWNER = 0x1d,
              LEAP = 0x35, FLIGHT = 0x37, MOUNTED = 0x76, RIDING = 0x77, LAND = 0x78, SWIM_OFF = 0x79, JUMP = 0x7a,
              ANSWER = 0x7e, DANCE = 0x82, CARRIED = 0x91, STILL = 0x66, ENRAGE = 0x8b, WAVE = 0xa1, TELEPORT = 0x9b,
              STRONG_CHARGE = 0x32, TALKING = 0x8c;

// Buff types.
constexpr uint8_t BUFF_REMOVED = 7, BUFF_FROZEN = 8, BUFF_FAST_CAST = 9, BUFF_BOOST = 13, BUFF_TAUNT = 0x12,
                  BUFF_GLIDER = 0x13, BUFF_GUARDED = 0x20, BUFF_SHARED = 3;

// Item types.
constexpr uint8_t ITEM_CONSUMABLE = 1, ITEM_WEAPON = 3, ITEM_CALL = 0x15, ITEM_TAKE_OFF = 0x18;

// The AI record (§1.1), at node offsets.
constexpr size_t E_KEY = 0x20;           // i64: the creature id
constexpr size_t E_IDLE_TIMER = 0x28;    // i32
constexpr size_t E_CHASE_TIMER = 0x2c;   // i32
constexpr size_t E_GIVE_UP = 0x30;       // i32
constexpr size_t E_SAW_PLAYER = 0x34;    // u8
constexpr size_t E_WAS_ASLEEP = 0x35;    // u8
constexpr size_t E_CALL_TIME = 0x38;     // u32
constexpr size_t E_FOCUS = 0x40;         // i64
constexpr size_t E_SKILL_TIMER = 0x48;   // i32
constexpr size_t E_DIRECTION = 0x4c;     // f32 x3
constexpr size_t E_AIM = 0x58;           // f32 x3
constexpr size_t E_UNUSED = 0x64;        // i32 x2
constexpr size_t E_AHEAD = 0x6c;         // u8
constexpr size_t E_MOVING = 0x6d;        // u8
constexpr size_t E_JUMP = 0x6e;          // u8
constexpr size_t E_GLIDER = 0x6f;        // u8
constexpr size_t E_BUDGET = 0x70;        // i32
constexpr size_t E_MATCH_SPEED = 0x74;   // f32
constexpr size_t E_MOVE_FLAGS = 0x88;    // u32
constexpr size_t E_STATE = 0x8c;         // u32
constexpr size_t E_POSITION = 0x90;      // i64 x3
constexpr size_t E_SIZE = 0xa8;          // f32 x3
constexpr size_t E_START_BLOCK = 0xe0;   // i32 x3
constexpr size_t E_START = 0xf0;         // i64 x3
constexpr size_t E_GOAL = 0x108;         // i64 x3
constexpr size_t E_RADIUS = 0x120;       // f32
constexpr size_t E_TARGET = 0x128;       // i64
constexpr size_t E_SEARCH_MARK = 0x130;  // i32
constexpr size_t E_OBJECT_ZONE = 0x168;  // i32 x2
constexpr size_t E_OBJECT_INDEX = 0x170; // i32

// --- Numbers -----------------------------------------------------------------------------

constexpr float DOT = 1.0f / 65536.0f; // dots to blocks
constexpr float BLOCK = 65536.0f;      // blocks to dots
constexpr int64_t ONE_BLOCK = 65536, HALF_BLOCK = 32768;
constexpr float WALK_SPEED = 80.0f;
constexpr float SIGHT_RANGE = 20.0f, SIGHT_LIMIT = 200.0f;
constexpr float RAND_SCALE = 32767.0f;
constexpr float DEGREES = 0.017453292f;
constexpr int GIVE_UP_AFTER = 15000, GIVE_UP_COOLDOWN = 3000, SKILL_WAIT = 10000;
constexpr int BUDGET_MIN = 50, BUDGET_CAP = 500, BUDGET_CONTROLLED = 2000;
constexpr float DAY_START = 25200000.0f, DAY_END = 72000000.0f;

template <class T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <class T> void put(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }

using Dots = std::array<int64_t, 3>;
using Vec = std::array<float, 3>;

int64_t trunc64(float v) { return (int64_t)v; }
int64_t trunc64(double v) { return (int64_t)v; }
// Floor-block: the block holding a coordinate in dots.
int32_t blockOf(int64_t v) { return (int32_t)(v >> 16); }
int floorDiv(int v, int n) { return v >= 0 ? v / n : -((-v + n - 1) / n); }
bool isSolid(const Block &b) { return (b[4] & ~2) != 0; }
bool isWater(const Block &b) { return b[4] == 2; }

// Short names for a creature's fields.
int64_t idOf(const Creature *c) { return get<int64_t>(c->record, ID); }
uint8_t kindOf(const Creature *c) { return c->record[KIND]; }
int32_t raceOf(const Creature *c) { return get<int32_t>(c->record, RACE); }
uint8_t classOf(const Creature *c) { return c->record[CLASS]; }
uint8_t specOf(const Creature *c) { return c->record[SPECIALISATION]; }
int abilityOf(const Creature *c) { return c->record[ABILITY]; }
void setAbility(Creature *c, int a) { c->record[ABILITY] = (uint8_t)a; }
float timeOf(const Creature *c) { return get<float>(c->record, ABILITY_TIME); }
void setTime(Creature *c, float t) { put(c->record, ABILITY_TIME, t); }
float healthOf(const Creature *c) { return get<float>(c->record, HEALTH); }
float manaOf(const Creature *c) { return get<float>(c->record, MANA); }
float stunOf(const Creature *c) { return get<float>(c->record, STUN); }
float dodgeOf(const Creature *c) { return get<float>(c->record, DODGE_TIMER); }
uint32_t flagsOf(const Creature *c) { return get<uint32_t>(c->record, FLAGS); }
uint32_t moveFlagsOf(const Creature *c) { return get<uint32_t>(c->record, MOVE_FLAGS); }
uint32_t aiFlagsOf(const Creature *c) { return get<uint32_t>(c->record, AI_FLAGS); }
uint16_t stateOf(const Creature *c) { return get<uint16_t>(c->record, STATE); }
void setState(Creature *c, uint16_t s) { put(c->record, STATE, s); }
void setStateBit(Creature *c, uint16_t bit, bool on) {
    setState(c, on ? (uint16_t)(stateOf(c) | bit) : (uint16_t)(stateOf(c) & ~bit));
}
float sizeOf(const Creature *c, int axis) { return get<float>(c->record, SIZE + 4 * axis); }
int64_t ownerOf(const Creature *c) { return get<int64_t>(c->record, OWNER); }
Dots dotsAt(const uint8_t *p, size_t at) { return {get<int64_t>(p, at), get<int64_t>(p, at + 8), get<int64_t>(p, at + 16)}; }
void putDots(uint8_t *p, size_t at, const Dots &d) {
    for (int i = 0; i < 3; i++)
        put(p, at + 8 * i, d[i]);
}
Vec vecAt(const uint8_t *p, size_t at) { return {get<float>(p, at), get<float>(p, at + 4), get<float>(p, at + 8)}; }
void putVec(uint8_t *p, size_t at, const Vec &v) {
    for (int i = 0; i < 3; i++)
        put(p, at + 4 * i, v[i]);
}
Dots positionOf(const Creature *c) { return dotsAt(c->record, POSITION); }
bool inWater(const Creature *c) { return (moveFlagsOf(c) & IN_WATER) != 0; }
bool onGround(const Creature *c) { return (moveFlagsOf(c) & ON_GROUND) != 0; }
bool groundOrWater(const Creature *c) { return (moveFlagsOf(c) & (ON_GROUND | IN_WATER)) != 0; }
uint8_t slotType(const Creature *c, size_t slot) { return c->record[slot]; }
int32_t slotSubtype(const Creature *c, size_t slot) { return get<int32_t>(c->record, slot + 4); }

// The offset from one place to another, in blocks.
Vec blocksBetween(const Dots &from, const Dots &to) {
    return {(float)(to[0] - from[0]) * DOT, (float)(to[1] - from[1]) * DOT, (float)(to[2] - from[2]) * DOT};
}
float lengthSquared(const Vec &v) { return v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; }

// The stat rules' view of a creature's buffs.
nativerules::HasBuff buffTest(const Creature *c) {
    return [c](int type) { return nativebuffs::find(*c, (uint8_t)type) != nullptr; };
}
bool hasBuff(const Creature *c, uint8_t type) { return nativebuffs::find(*c, type) != nullptr; }
// 0x4f220 for the current ability, compared as a float.
float duration(const Creature *c) { return (float)nativerules::abilityDuration(c->record, buffTest(c)); }
int effectTime(const Creature *c, int ability) { return nativerules::abilityEffectTime(c->record, ability, buffTest(c)); }
int middleTime(const Creature *c, int ability) { return nativerules::abilityMiddleTime(c->record, ability, buffTest(c)); }
int recoveryTime(const Creature *c, int ability) {
    return nativerules::abilityRecoveryTime(c->record, ability, buffTest(c));
}
float maxHealth(const Creature *c) {
    const std::vector<nativerules::Buff> buffs = nativebuffs::forStats(*c);
    return nativerules::maxHealth(c->record, buffs.data(), buffs.size());
}
float manaCost(const Creature *c, int ability) { return nativerules::manaCost(ability, buffTest(c)); }
float staminaCost(const Creature *c, int ability) { return nativerules::staminaCost(ability, buffTest(c)); }

// Copies an item (0xa0 bytes) the way the game's item copy does: field by field, leaving
// the padding bytes (1..3, 0x15..0x17, 0x9e, 0x9f) of the destination as they were.
void copyItem(uint8_t *to, const uint8_t *from) {
    to[0] = from[0];
    std::memcpy(to + 4, from + 4, 0x11);      // +4..+0x14
    std::memcpy(to + 0x18, from + 0x18, 0x86); // +0x18..+0x9d
}

// --- Geometry over the world's blocks (§5.9-§5.11, effect-objects.md §2.5) ----------------

Block blockAt(World &w, int x, int y, int z) { return w.block(x, y, z); }
Block blockAtDots(World &w, const Dots &p) { return w.block(blockOf(p[0]), blockOf(p[1]), blockOf(p[2])); }

// 0xf3e20: whether two places are farther apart than `d` blocks (in double).
bool fartherThan(const Dots &p, const Dots &q, float d) {
    const double k = 1.0 / 65536.0;
    const double dx = (double)(p[0] - q[0]) * k, dy = (double)(p[1] - q[1]) * k, dz = (double)(p[2] - q[2]) * k;
    if (std::fabs(dx) > d || std::fabs(dy) > d)
        return true;
    return dx * dx + dy * dy + dz * dz > (double)(d * d);
}

// The grid walk shared by the block ray 0x2d1d20 and the AI's own ray 0x12b10: how far from
// `start` along the unit direction `u` it gets through blocks `passes` lets through, at most
// `most` blocks. It stops at the first block it may not pass or outside the loaded zones.
template <class Passes> float gridWalk(World &w, const Dots &start, const Vec &u, float most, Passes passes) {
    int32_t cell[3] = {blockOf(start[0]), blockOf(start[1]), blockOf(start[2])};
    int zx = floorDiv(cell[0], 64), zy = floorDiv(cell[1], 64);
    bool loaded = w.zone ? w.zone(zx, zy) : true;
    float t = 0.0f;
    if (!(most > 0.0f))
        return most < 0.0f ? most : 0.0f;
    int steps = 0;
    for (;;) {
        if (!loaded)
            break;
        if (!passes(blockAt(w, cell[0], cell[1], cell[2])))
            break;
        if (steps > 500)
            break;
        steps++;
        int64_t p[3];
        for (int i = 0; i < 3; i++)
            p[i] = start[i] + trunc64((u[i] * t) * BLOCK);
        float step = 10.0f;
        int axis = 0;
        for (int i = 0; i < 3; i++) {
            if (0.000001f > u[i] * u[i])
                continue;
            const int64_t boundary = u[i] > 0.0f ? ((int64_t)cell[i] + 1) * ONE_BLOCK : (int64_t)cell[i] * ONE_BLOCK;
            float s = (float)(boundary - p[i]) / u[i];
            s = (float)trunc64(s) * DOT;
            if (s < step)
                step = s, axis = i;
        }
        cell[axis] += u[axis] > 0.0f ? 1 : -1;
        const int nzx = floorDiv(cell[0], 64), nzy = floorDiv(cell[1], 64);
        if (nzx != zx || nzy != zy) {
            zx = nzx, zy = nzy;
            loaded = w.zone ? w.zone(zx, zy) : true;
        }
        t = t + step;
        if (!(most > t))
            break;
    }
    return most < t ? most : t;
}

// 0x12b10's pass test: air or water, and not a closed barrier.
bool aiRayPasses(const Block &b) {
    return (b[4] == 0 || b[4] == 2) && (!(b[5] & 0x40) || (b[5] & 0x04) || (b[5] & 0x80));
}

// §5.9, 0x121a0: whether the straight line from `from` to `to` is open, if no longer than r.
bool openLine(World &w, const Dots &from, const Dots &to, float r) {
    const Vec d = blocksBetween(from, to);
    const float s = lengthSquared(d);
    if (s < 0.0001f)
        return true;
    if (s > r * r)
        return false;
    const float len = std::sqrt(s);
    const Vec u = {d[0] / len, d[1] / len, d[2] / len};
    const float t = gridWalk(w, from, u, len, aiRayPasses);
    return len - 0.01f <= t;
}

// §5.10, 0x2b7ad0: whether a box of `size` blocks centred at `centre` touches anything.
bool boxTouches(World &w, const Dots &centre, const Vec &size) {
    int32_t lo[3], hi[3];
    for (int i = 0; i < 3; i++) {
        const int64_t half = trunc64(size[i] * 0.5f * BLOCK);
        lo[i] = blockOf(centre[i] - half);
        hi[i] = blockOf(centre[i] + half);
    }
    for (int x = lo[0]; x <= hi[0]; x++)
        for (int y = lo[1]; y <= hi[1]; y++)
            for (int z = lo[2]; z <= hi[2]; z++) {
                const bool interior = x > lo[0] && x < hi[0] && y > lo[1] && y < hi[1] && z > lo[2] && z < hi[2];
                if (interior)
                    continue;
                const Block b = blockAt(w, x, y, z);
                if (isSolid(b) || ((b[5] & 0x40) && !(b[5] & 0x80)) || isWater(b))
                    return true;
            }
    return false;
}

// §5.11, 0x2a1250: whether A sees B within `range` blocks (less when B sneaks).
bool inSight(World &w, const Creature *a, const Creature *b, float range) {
    const float stealth = get<float>(b->record, STEALTH);
    if (stealth > 0.0f) {
        float k = 1.0f - stealth;
        if (0.1f > k)
            k = 0.1f;
        range = range * k;
    }
    const Dots from = positionOf(a), to = positionOf(b);
    return canSee(w, from.data(), to.data(), false, range);
}

// §5.12, 0x4ef80: whether A and B are enemies, as the AI judges it.
bool enemies(const Creature *a, const Creature *b) {
    const auto peaceable = [](uint8_t k) { return k == PLAYER || k == TOWNSPERSON || k == PET; };
    const uint8_t ka = kindOf(a), kb = kindOf(b);
    if (peaceable(ka) && peaceable(kb))
        return false;
    if (classOf(a) == CLASS_PEACEFUL || classOf(b) == CLASS_PEACEFUL || raceOf(a) == RACE_PEACEFUL ||
        raceOf(b) == RACE_PEACEFUL)
        return false;
    if ((stateOf(a) & ASLEEP) || (stateOf(b) & ASLEEP))
        return false;
    if (ka == OBJECT_A || ka == OBJECT_B || kb == OBJECT_A || kb == OBJECT_B)
        return false;
    if ((ka == PLAYER && (stateOf(a) & SPARED)) || (kb == PLAYER && (stateOf(b) & SPARED)))
        return false;
    if ((stateOf(a) & STATE_HOSTILE) || (stateOf(b) & STATE_HOSTILE))
        return true;
    const auto monster = [](const Creature *c) { return kindOf(c) == MONSTER && !(flagsOf(c) & PEACEFUL); };
    return monster(a) != monster(b);
}

// §5.13, 0x127f0: scales a vector to length 1.
Vec normalised(Vec v) {
    const float k = 1.0f / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    return {v[0] * k, k * v[1], k * v[2]};
}

// §5.3: the ground two blocks ahead of a creature, if there is footing there.
bool groundAhead(World &w, const Creature *c, Dots &q) {
    const float a = get<float>(c->record, HEADING) * DEGREES;
    const float cs = nativecrt::cosf(a), sn = nativecrt::sinf(a);
    const Dots p = positionOf(c);
    q = {p[0] + trunc64(cs * 2.0f * BLOCK), p[1] + trunc64(sn * 2.0f * BLOCK), p[2]};
    const auto solidAt = [&](int64_t z) { return isSolid(blockAtDots(w, {q[0], q[1], z})); };
    for (int i = 0; i < 3 && !solidAt(q[2] - ONE_BLOCK); i++)
        q[2] -= ONE_BLOCK;
    for (int i = 0; i < 3 && solidAt(q[2]); i++)
        q[2] += ONE_BLOCK;
    return solidAt(q[2] - ONE_BLOCK) && !solidAt(q[2]);
}

// Lowers a goal under creature T until T's box, a half block lower, rests on something,
// then down to T's feet (§2.2 step 5). The game has no limit; a world with nothing below
// would never end, so this stops after a generous depth.
void settle(World &w, const Creature *t, Dots &g) {
    const Vec size = {sizeOf(t, 0), sizeOf(t, 1), sizeOf(t, 2)};
    for (int i = 0; i < 4096; i++) {
        if (boxTouches(w, {g[0], g[1], g[2] - HALF_BLOCK}, size))
            break;
        g[2] -= ONE_BLOCK;
    }
    g[2] -= trunc64((sizeOf(t, 2) * 0.5f - 0.1f) * BLOCK);
}

// --- The AI record and its search (§1.1, §5.4-§5.8) ---------------------------------------

struct Record {
    Entry &e;
    uint8_t *raw() { return e.raw.data(); }
    template <class T> T f(size_t at) { return get<T>(e.raw.data(), at); }
    template <class T> void set(size_t at, T v) { put(e.raw.data(), at, v); }
    Dots position() { return dotsAt(raw(), E_POSITION); }
    float size(int axis) { return f<float>(E_SIZE + 4 * axis); }
    uint32_t state() { return f<uint32_t>(E_STATE); }
    bool glider() { return f<uint8_t>(E_GLIDER) != 0; }
    void stop() { putVec(raw(), E_DIRECTION, {0, 0, 0}); }
    IntVec startBlock() { return {f<int32_t>(E_START_BLOCK), f<int32_t>(E_START_BLOCK + 4), f<int32_t>(E_START_BLOCK + 8)}; }
    IntVec goalBlock() {
        const Dots g = dotsAt(raw(), E_GOAL);
        return {blockOf(g[0]), blockOf(g[1]), blockOf(g[2])};
    }
};

// "Forget" (0xba20): drop the search, its path and what it had taken.
void forget(Entry &e) {
    e.steps.clear();
    e.open.clear();
    e.path.clear();
    e.best = e.steps.end();
    e.taken.clear();
}

// §1.2: the search's estimate between two blocks.
int32_t estimate(const IntVec &a, const IntVec &goal) {
    const int32_t dx = std::abs(a[0] - goal[0]), dy = std::abs(a[1] - goal[1]), dz = std::abs(a[2] - goal[2]);
    const int32_t flat = dy < dx ? dx * 10 + dy * 4 : dx * 4 + dy * 10;
    return (flat + dz * 10) * 2;
}

// §5.5, 0x13160: records a block's step and opens it; it becomes the best when nearer.
void recordStep(Entry &e, const IntVec &b, const Step &s) {
    auto it = e.steps.insert_or_assign(b, s).first;
    e.open.insert(b);
    if (e.best == e.steps.end() || e.best->second.h > s.h)
        e.best = it;
}

// §5.4, 0x133d0: starts a search from the record's start toward its goal.
void startSearch(Entry &e) {
    Record r{e};
    forget(e);
    const Dots start = dotsAt(r.raw(), E_START);
    const IntVec sb = {blockOf(start[0]), blockOf(start[1]), blockOf(start[2])};
    for (int i = 0; i < 3; i++)
        r.set<int32_t>(E_START_BLOCK + 4 * i, sb[i]);
    r.set<int32_t>(E_SEARCH_MARK, 0);
    Step s;
    s.g = 1;
    s.h = s.f = estimate(sb, r.goalBlock());
    // The spec calls the start's "from" x and y stack contents (nothing reads them, §5.7);
    // every recording shows the start block itself there.
    s.from = sb;
    recordStep(e, sb, s);
    e.taken.clear();
}

// The start of a search: the record's position, lowered to just above its feet.
void setStartFromPosition(Record &r) {
    Dots start = r.position();
    start[2] -= trunc64((r.size(2) * 0.5f - 0.1f) * BLOCK);
    putDots(r.raw(), E_START, start);
}

// §5.1: go to a fixed place (the caller then starts the search).
void goTo(Entry &e, const Dots &place) {
    Record r{e};
    putDots(r.raw(), E_GOAL, place);
    setStartFromPosition(r);
    r.set<float>(E_RADIUS, 5.0f);
}

// §5.6.1: offers block n to the search at `cost`, coming from c; whether it was taken.
bool offer(Entry &e, const IntVec &n, int32_t cost, const IntVec &c, const IntVec &goal) {
    auto it = e.steps.find(n);
    if (it == e.steps.end()) {
        Step s;
        s.g = cost;
        s.h = estimate(n, goal);
        s.f = cost + s.h;
        s.from = c;
        recordStep(e, n, s);
        return true;
    }
    if (!e.open.count(n) || it->second.g <= cost)
        return false;
    it->second.g = cost;
    it->second.f = cost + it->second.h;
    it->second.from = c;
    return true;
}

// The four corners of a body's box, in the order (x0,y0), (x1,y0), (x0,y1), (x1,y1).
enum Corner { C00, C10, C01, C11 };

// §5.6, 0x15640: expands the open block with the lowest cost.
void expand(World &w, Entry &e) {
    Record r{e};
    if (e.steps.empty() || e.open.empty())
        return;
    // The open block whose step costs least (the first in block order on ties).
    const IntVec *pick = nullptr;
    int32_t lowest = 0;
    for (const IntVec &b : e.open) {
        auto it = e.steps.find(b);
        if (it == e.steps.end())
            continue;
        if (!pick || it->second.f < lowest)
            pick = &b, lowest = it->second.f;
    }
    if (!pick)
        return;
    const IntVec c = *pick;
    e.open.erase(c);
    const IntVec goal = r.goalBlock();
    if (c == goal)
        return;
    const int32_t g = e.steps[c].g;

    // The body's box standing in c, one block wider all round.
    const int64_t cx = (int64_t)c[0] * ONE_BLOCK + HALF_BLOCK, cy = (int64_t)c[1] * ONE_BLOCK + HALF_BLOCK;
    const int64_t cz = (int64_t)c[2] * ONE_BLOCK + trunc64((double)r.size(2) * 0.5 * 65536.0);
    const int64_t hx = trunc64(r.size(0) * 0.5f * BLOCK), hy = trunc64(r.size(1) * 0.5f * BLOCK),
                  hz = trunc64(r.size(2) * 0.5f * BLOCK);
    const int32_t x0 = blockOf(cx - hx) - 1, x1 = blockOf(cx + hx) + 1;
    const int32_t y0 = blockOf(cy - hy) - 1, y1 = blockOf(cy + hy) + 1;
    const int32_t z0 = blockOf(cz - hz + 655) - 1, z1 = blockOf(cz + hz) + 2;
    const uint8_t here = blockAtDots(w, {cx, cy, cz + 65})[4];
    const bool glider = r.glider();

    bool under = false, below = false, roof = false;
    bool floorCorner[4] = {}, edge[4] = {};
    bool floorMX = false, floorPX = false, floorMY = false, floorPY = false;
    bool wallMX = false, wallPX = false, wallMY = false, wallPY = false;
    const auto barrier = [glider](const Block &b) { return (b[5] & 0x40) && (!glider || !(b[5] & 0x80)); };
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++)
            for (int z = z0; z <= z1; z++) {
                const bool innerX = x0 < x && x < x1, innerY = y0 < y && y < y1;
                if (innerX && innerY && z0 < z && z < z1)
                    continue;
                const Block b = blockAt(w, x, y, z);
                const bool inner = innerX && innerY;
                if (inner && z == z0) {
                    under |= isSolid(b) || barrier(b) || isWater(b);
                    const Block lower = blockAt(w, x, y, z0 - 1);
                    below |= isSolid(lower) || barrier(lower);
                }
                const bool blocks = isSolid(b) || ((b[5] & 0x40) && !(b[5] & 0x04) && (!glider || !(b[5] & 0x80)));
                if (!blocks)
                    continue;
                const bool atFloor = z == z0, atSide = z0 < z && z < z1;
                int corner = -1;
                if (x == x0 && y == y0)
                    corner = C00;
                else if (x == x1 && y == y0)
                    corner = C10;
                else if (x == x0 && y == y1)
                    corner = C01;
                else if (x == x1 && y == y1)
                    corner = C11;
                if (corner >= 0) {
                    if (atFloor)
                        floorCorner[corner] = true;
                    if (atSide)
                        edge[corner] = true;
                }
                if (inner && z == z1)
                    roof = true;
                bool *floorSide = nullptr, *wall = nullptr;
                if (x == x0 && innerY)
                    floorSide = &floorMX, wall = &wallMX;
                else if (x == x1 && innerY)
                    floorSide = &floorPX, wall = &wallPX;
                else if (y == y0 && innerX)
                    floorSide = &floorMY, wall = &wallMY;
                else if (y == y1 && innerX)
                    floorSide = &floorPY, wall = &wallPY;
                if (floorSide && atFloor)
                    *floorSide = true;
                if (wall && atSide)
                    *wall = true;
            }

    const bool controlled = (r.state() & CONTROLLED) != 0;
    if (!below && controlled && e.steps.size() > 1)
        return;
    const bool water = here == 2;
    const int32_t side = under ? g + 10 : g + 20, diagonal = under ? g + 14 : g + 24;
    const auto at = [&](int dx, int dy, int dz) { return IntVec{c[0] + dx, c[1] + dy, c[2] + dz}; };
    bool okMX = false, okPX = false, okMY = false, okPY = false;
    if (!wallMX && (under || floorMX || wallMY || wallPY || edge[C00] || edge[C01] || water))
        okMX = offer(e, at(-1, 0, 0), side, c, goal);
    if (!wallPX && (under || floorPX || wallMY || wallPY || edge[C10] || edge[C11] || water))
        okPX = offer(e, at(1, 0, 0), side, c, goal);
    if (!wallMY && (under || floorMY || wallMX || wallPX || edge[C00] || edge[C10] || water))
        okMY = offer(e, at(0, -1, 0), side, c, goal);
    if (!wallPY && (under || floorPY || wallMX || wallPX || edge[C01] || edge[C11] || water))
        okPY = offer(e, at(0, 1, 0), side, c, goal);
    if (!edge[C00] && okMX && okMY && (under || floorCorner[C00] || water))
        offer(e, at(-1, -1, 0), diagonal, c, goal);
    if (!edge[C10] && okPX && okMY && (under || floorCorner[C10] || water))
        offer(e, at(1, -1, 0), diagonal, c, goal);
    if (!edge[C01] && okMX && okPY && (under || floorCorner[C01] || water))
        offer(e, at(-1, 1, 0), diagonal, c, goal);
    if (!edge[C11] && okPX && okPY && (under || floorCorner[C11] || water))
        offer(e, at(1, 1, 0), diagonal, c, goal);
    if (!under)
        offer(e, at(0, 0, -1), g, c, goal);
    if (roof || (!under && controlled))
        return;
    if (!(wallMX || wallPX || wallMY || wallPY) && !water)
        return;
    offer(e, at(0, 0, 1), under ? g + 14 : g + 28, c, goal);
}

// §5.7, 0xedb0: the path from the start to the best block so far (or the goal).
void rebuildPath(Entry &e) {
    Record r{e};
    e.path.clear();
    IntVec cur = e.best != e.steps.end() ? e.best->first : r.goalBlock();
    e.path.push_front(cur);
    const IntVec start = r.startBlock();
    for (int32_t k = 0;; k++) {
        if (cur == start || k > (int32_t)e.steps.size())
            break;
        auto it = e.steps.find(cur);
        if (it == e.steps.end() || it->second.from == cur)
            break;
        cur = it->second.from;
        e.path.push_front(cur);
    }
}

// §5.8, 0xa950: whether a body of `size` at `pos` can walk straight toward `goal` until
// within r of it (or as far as 50 blocks), with footing all the way.
bool walkable(World &w, const Vec &size, const Dots &pos, const Dots &goal, float r, bool glider, uint32_t flags) {
    const auto passes = [glider](const Block &b) {
        return !isSolid(b) && !((b[5] & 0x40) && !(b[5] & 0x04) && (!glider || !(b[5] & 0x80)));
    };
    const bool controlled = (flags & CONTROLLED) != 0;
    int32_t zc = blockOf(pos[2] + trunc64((0.1f - size[2] * 0.5f) * BLOCK));
    int64_t cell[2] = {blockOf(pos[0]), blockOf(pos[1])};
    const float fx = (float)(goal[0] - pos[0]) * DOT, fy = (float)(goal[1] - pos[1]) * DOT;
    const float len = std::sqrt(fx * fx + fy * fy + 0.0f);
    const float dir[2] = {fx / len, fy / len};
    const float most = len <= 50.0f ? len : 50.0f;
    float t = 0.0f;
    bool footing = false;
    if (0.0f < most) {
        for (;;) {
            const int64_t px = pos[0] + trunc64((dir[0] * t) * BLOCK), py = pos[1] + trunc64((dir[1] * t) * BLOCK);
            const int32_t x0 = blockOf(px - trunc64((double)size[0] * 0.5 * 65536.0));
            const int32_t x1 = blockOf(px - trunc64((double)size[0] * 0.5 * -65536.0));
            const int32_t y0 = blockOf(py - trunc64((double)size[1] * 0.5 * 65536.0));
            const int32_t y1 = blockOf(py - trunc64((double)size[1] * 0.5 * -65536.0));
            const int32_t top = blockOf(trunc64((((float)zc + size[2]) + 1.0f) * BLOCK));
            // Room for the body above its feet.
            if (t > 0.0f)
                for (int x = x0; x <= x1; x++)
                    for (int y = y0; y <= y1; y++)
                        for (int z = zc + 1; z <= top; z++)
                            if (!passes(blockAt(w, x, y, z)))
                                return false;
            // A step up, when there is room above it.
            bool step = false;
            for (int x = x0; x <= x1; x++)
                for (int y = y0; y <= y1; y++)
                    if (isSolid(blockAt(w, x, y, zc))) {
                        for (int z = zc + 1; z <= top; z++)
                            if (!passes(blockAt(w, x, y, z)) && t > 0.0f)
                                return false;
                        step = true;
                    }
            if (step)
                zc += 1;
            // Footing under the feet (water carries too).
            footing = false;
            bool lower = false;
            for (int x = x0; x <= x1; x++)
                for (int y = y0; y <= y1; y++) {
                    if (isSolid(blockAt(w, x, y, zc - 1)) || isWater(blockAt(w, x, y, zc)))
                        footing = true;
                    if (controlled && isSolid(blockAt(w, x, y, zc - 2)))
                        lower = true;
                }
            if (controlled && !lower)
                return false;
            if (isWater(blockAt(w, (int)cell[0], (int)cell[1], zc))) {
                const int32_t level = (int32_t)(size[2] * 0.5f + (float)zc);
                const int64_t want = goal[2] / ONE_BLOCK;
                if (want < level)
                    zc -= 1;
                else if (level < want)
                    zc += 1;
            } else if (!footing) {
                zc -= 1;
            }
            // On to the next column the line crosses.
            float stepLength = 10.0f;
            int axis = 0;
            const int64_t p[2] = {px, py};
            for (int i = 0; i < 2; i++) {
                if (!(dir[i] * dir[i] >= 0.000001f))
                    continue;
                const int64_t l = dir[i] <= 0.0f ? cell[i] * ONE_BLOCK - p[i] : (cell[i] + 1) * ONE_BLOCK - p[i];
                const float s = ((float)l * DOT) / dir[i];
                if (i == 0 ? s < 10.0f : s < stepLength)
                    stepLength = s, axis = i;
            }
            cell[axis] += dir[axis] <= 0.0f ? -1 : 1;
            t += stepLength;
            if (most <= t)
                break;
        }
    }
    const Dots end = {cell[0] * ONE_BLOCK + HALF_BLOCK, trunc64(((double)cell[1] + 0.5) * 65536.0),
                      trunc64((size[2] * 0.5f + (float)zc) * BLOCK)};
    if (!(footing || isWater(blockAt(w, (int)cell[0], (int)cell[1], zc))))
        return false;
    const float ex = (float)(end[0] - goal[0]) * DOT, ey = (float)(end[1] - goal[1]) * DOT;
    return r * r < ey * ey + ex * ex || openLine(w, end, goal, r);
}

// --- Attack choices (§5.15), also the player's -------------------------------------------

// "within(X)": the ability is X and it has not run past its length (and a little).
bool within(const Creature *c, int ability) {
    return abilityOf(c) == ability && timeOf(c) < (float)(nativerules::abilityDuration(c->record, buffTest(c)) + 200);
}
bool boosted(const Creature *c, bool force) { return force || hasBuff(c, BUFF_BOOST); }
bool weaponIs(const Creature *c, size_t slot, int32_t subtype) {
    return slotType(c, slot) == ITEM_WEAPON && slotSubtype(c, slot) == subtype;
}

// 0x60120: the strong (charged) attack the creature would start.
int chargeMove(const Creature *c, bool force) {
    const uint8_t cls = classOf(c);
    const bool spec1 = specOf(c) == 1;
    if (boosted(c, force)) {
        if (cls == 3 && spec1)
            return 0x86;
        if (cls == 4)
            return 0x32;
    }
    if (raceOf(c) == RACE_123)
        return 0x9e;
    switch (cls) {
    case 0x91: return 0x15;
    case 0x92: return 0x41;
    case 0x93: case 0x94: return 0x40;
    case 0x95: return 0x19;
    case 0x96: return 0x18;
    case 0x97: return 0x69;
    case 0x98: return spec1 ? 0x2c : 0x26;
    default: break;
    }
    const int32_t w = slotSubtype(c, SLOT_B);
    if (w == 0x17)
        return 0x4c;
    if (w == 0x16)
        return abilityOf(c) == 2 && timeOf(c) < (float)(nativerules::abilityDuration(c->record, buffTest(c)) + 200) ? 3 : 2;
    if (weaponIs(c, SLOT_A, 6) || weaponIs(c, SLOT_B, 6))
        return 0x1a;
    if (weaponIs(c, SLOT_A, 7) || weaponIs(c, SLOT_B, 7))
        return 0x19;
    if (weaponIs(c, SLOT_A, 8) || weaponIs(c, SLOT_B, 8))
        return 0x1c;
    if (!nativerules::usesMana(c->record)) {
        if (weaponIs(c, SLOT_B, 5))
            return 6;
        if (weaponIs(c, SLOT_B, 3))
            return 0x12;
        if (weaponIs(c, SLOT_A, 13))
            return 9;
        if (nativerules::holdsTwoHanded(c->record))
            return 0x3c;
        if (w == 4 || slotType(c, SLOT_B) == 0)
            return 0x15;
        return 0x40;
    }
    if (w == 0xb)
        return spec1 ? 0x6a : 0x69;
    if (w == 10)
        return spec1 ? 0x22 : 0x20;
    return spec1 ? 0x2c : 0x26;
}

// --- The tick ----------------------------------------------------------------------------

class Brain {
public:
    Brain(World &w, std::map<int64_t, Entry> &ai, uint32_t dt) : w(w), ai(ai), dt(dt) {}
    void run();

private:
    World &w;
    std::map<int64_t, Entry> &ai;
    uint32_t dt;

    int rand() { return w.rand ? w.rand() : 0; }
    bool runsHere(const Creature *c) const { return !w.client || c == w.local; }
    Creature *find(int64_t id) const {
        auto it = w.byId.find(id);
        return it == w.byId.end() ? nullptr : it->second;
    }
    Entry &entryOf(Creature *c);
    void buffNotice(Creature *c, const nativebuffs::Buff &b, bool refresh);
    void aggroNotice(Creature *a, Creature *x);
    void objectInteraction(std::list<Interaction> &to, int zx, int zy, int index);

    // §2: the phases.
    void turns();
    void move(Entry &e);
    void followPath(Entry &e);
    void walkStraight(Entry &e, const Dots &goal);
    void handOver();
    void dropRecords();

    // §3, §4: one creature's turn and its behaviours.
    void turn(Creature *c);
    void callOut(Creature *c);
    bool rideAlong(Creature *c);
    void healPet(Creature *c);
    void attach(Creature *c);
    bool guard(Creature *c);
    bool fight(Creature *c);
    bool greet(Creature *c);
    bool followLeader(Creature *c, Creature *leader, float far2, float near2, float radius, bool ahead);
    bool followGroup(Creature *c);
    bool followOwner(Creature *c);
    bool idle(Creature *c);
    void openDoors(Creature *c);

    // §4.4 parts.
    Creature *preferredTarget(Creature *c);
    bool callsFor(const Creature *x, const Creature *c) const;
    void attack(Creature *c, Creature *t, Vec &d, float s);
    void leap(Creature *c, Creature *t);
    void dodge(Creature *c, Creature *t);
    void takeOff(Creature *c, int ability, int kind);

    // §5.2: follow a creature.
    void follow(Entry &e, Creature *t, float radius, bool ahead, bool matchSpeed);
};

Entry &Brain::entryOf(Creature *c) {
    const int64_t id = idOf(c);
    auto it = ai.find(id);
    if (it != ai.end())
        return it->second;
    // A new record as 0x96e0 sets it up.
    Entry &e = ai[id];
    Record r{e};
    r.set<int64_t>(E_KEY, id);
    r.set<int32_t>(E_SKILL_TIMER, SKILL_WAIT);
    r.set<int32_t>(E_UNUSED, -1);
    r.set<int32_t>(E_UNUSED + 4, -1);
    r.set<int32_t>(E_BUDGET, BUDGET_MIN);
    putVec(r.raw(), E_SIZE, {1.0f, 1.0f, 1.0f});
    // The game leaves the object reference as the allocator gave it; no object (open
    // question 1).
    r.set<int64_t>(E_OBJECT_ZONE, -1);
    r.set<int32_t>(E_OBJECT_INDEX, -1);
    return e;
}

// 0x29d2f0: gives a creature a buff and tells the world, the creature as both ends.
void Brain::buffNotice(Creature *c, const nativebuffs::Buff &b, bool refresh) {
    nativebuffs::give(*c, b, refresh);
    w.buffNotes.push_back(nativebuffs::notice(idOf(c), idOf(c), b, refresh ? 1 : 0));
}

// 0x2c3200 (creature-hit.md §4.1): X tells the world how angry it is at A, and X's allies
// within 8 blocks who weren't angry at A yet become a little angry.
void Brain::aggroNotice(Creature *a, Creature *x) {
    if (!(healthOf(x) > 0.0f))
        return;
    const auto note = [&](int64_t who, float anger) {
        std::array<uint8_t, 0x18> n{};
        put(n.data(), 0, idOf(a));
        put(n.data(), 8, who);
        put(n.data(), 0x10, anger);
        w.aggroNotes.push_back(n);
    };
    note(idOf(x), x->aggro[idOf(a)]);
    for (Creature *c : w.creatures) {
        if (!c || c == a || c == x || !nativerules::hostile(c->record, a->record) ||
            fartherThan(positionOf(c), positionOf(x), 8.0f))
            continue;
        float &anger = c->aggro[idOf(a)];
        if (anger == 0.0f) {
            anger = 0.5f;
            note(idOf(c), 0.5f);
        }
    }
}

// §1.5: an object interaction (kind 5) for object `index` of zone (zx, zy).
void Brain::objectInteraction(std::list<Interaction> &to, int zx, int zy, int index) {
    Interaction i{};
    put<int32_t>(i.data(), 0xa0, zx);
    put<int32_t>(i.data(), 0xa4, zy);
    put<int32_t>(i.data(), 0xa8, index);
    i[0xc4] = 5;
    put<uint16_t>(i.data(), 0xc6, 1);
    to.push_back(i);
}

void Brain::run() {
    turns();
    for (auto &[id, e] : ai)
        move(e);
    handOver();
    dropRecords();
}

// §2.1 Phase A.
void Brain::turns() {
    for (Creature *c : w.creatures) {
        if (!runsHere(c))
            continue;
        if (kindOf(c) == PLAYER && !(stateOf(c) & CONTROLLED))
            continue;
        if (!(healthOf(c) > 0.0f) || !(stunOf(c) <= 0.0f))
            continue;
        Entry &e = entryOf(c);
        Record r{e};
        putDots(r.raw(), E_POSITION, positionOf(c));
        putVec(r.raw(), E_SIZE, vecAt(c->record, SIZE));
        r.set<uint32_t>(E_MOVE_FLAGS, moveFlagsOf(c));
        r.set<uint32_t>(E_STATE, stateOf(c));
        putVec(r.raw(), E_AIM, vecAt(c->record, AIM));
        e.waiting.clear();
        r.set<uint8_t>(E_GLIDER, hasBuff(c, BUFF_GLIDER) ? 1 : 0);
        if (w.turn)
            w.turn(idOf(c));
        turn(c);
        putVec(r.raw(), E_AIM, vecAt(c->record, AIM));
    }
}

// §2.2 Phase B: one record toward its goal.
void Brain::move(Entry &e) {
    Record r{e};
    r.set<uint8_t>(E_MOVING, 0);
    r.set<uint8_t>(E_JUMP, 0);
    if (r.state() & ASLEEP) {
        forget(e);
        r.set<int64_t>(E_TARGET, 0);
        r.set<float>(E_MATCH_SPEED, 0.0f);
        return;
    }
    if (500.0f < (float)r.f<int32_t>(E_BUDGET))
        r.set<int32_t>(E_BUDGET, BUDGET_CAP);
    if (e.open.empty())
        return;

    // Where to go: the goal, or where the creature it follows now stands.
    Dots goal = dotsAt(r.raw(), E_GOAL);
    if (Creature *t = find(r.f<int64_t>(E_TARGET))) {
        goal = positionOf(t);
        if (r.f<uint8_t>(E_AHEAD))
            groundAhead(w, t, goal);
        settle(w, t, goal);
    }
    const Dots aimAt = {goal[0], goal[1], trunc64(r.size(2) * 0.5f * BLOCK) + goal[2]};
    const Vec size = {r.size(0), r.size(1), r.size(2)};
    if (walkable(w, size, r.position(), aimAt, r.f<float>(E_RADIUS), r.glider(), r.state())) {
        walkStraight(e, goal);
        return;
    }

    // Search: expand within the budget until the path ends near enough the goal.
    const int budget = (r.state() & CONTROLLED) ? BUDGET_CONTROLLED : r.f<int32_t>(E_BUDGET);
    setStartFromPosition(r);
    putDots(r.raw(), E_GOAL, goal);
    startSearch(e);
    const auto pathEndDistance = [&] {
        const IntVec p = e.path.back(), g = r.goalBlock();
        return (float)((p[0] - g[0]) * (p[0] - g[0]) + (p[1] - g[1]) * (p[1] - g[1]) + (p[2] - g[2]) * (p[2] - g[2]));
    };
    for (int i = 0; i < budget; i++) {
        if (!e.path.empty() && pathEndDistance() <= r.size(0) * r.size(0))
            break;
        expand(w, e);
        rebuildPath(e);
    }
    if (!e.path.empty() && r.size(0) * r.size(0) < pathEndDistance())
        r.set<int32_t>(E_BUDGET, r.f<int32_t>(E_BUDGET) + r.f<int32_t>(E_BUDGET) / 10);
    r.stop();
    if (!(r.state() & CONTROLLED) || e.path.size() > 10)
        followPath(e);
}

// §2.2 step 7.6: head for the farthest block on the path that can be walked to straight.
void Brain::followPath(Entry &e) {
    Record r{e};
    const Dots me = r.position();
    const Vec size = {r.size(0), r.size(1), r.size(2)};
    const int64_t lift = trunc64(r.size(2) * 0.5f * BLOCK);
    const std::vector<IntVec> path(e.path.begin(), e.path.end());
    for (int left = (int)path.size() - 1; left >= 0; left--) {
        const IntVec &b = path[left];
        const Dots p = {(int64_t)b[0] * ONE_BLOCK + HALF_BLOCK, (int64_t)b[1] * ONE_BLOCK + HALF_BLOCK,
                        (int64_t)b[2] * ONE_BLOCK + lift};
        if (!(left < 2 || walkable(w, size, me, p, 2.0f, r.glider(), r.state())))
            continue;
        Vec d = blocksBetween(me, p);
        if (!isWater(blockAtDots(w, me)) && me[2] < p[2])
            d[2] = 0.0f;
        const float len2 = lengthSquared(d);
        if (len2 > 0.0f) {
            const float k = 1.0f / std::sqrt(len2);
            d = {d[0] * k, k * d[1], k * d[2]};
        }
        for (float &v : d)
            v *= WALK_SPEED;
        if (left < 4 && left + 1 < (int)path.size()) {
            const IntVec &n = path[left + 1];
            if (!isSolid(blockAt(w, n[0], n[1], n[2] - 1))) {
                // A drop or a gap next: jump, slowly.
                r.set<uint8_t>(E_JUMP, 1);
                for (float &v : d)
                    v *= 0.1f;
            }
            if (me[2] < lift + (int64_t)n[2] * ONE_BLOCK)
                d[2] = 20.0f;
        }
        putVec(r.raw(), E_DIRECTION, d);
        return;
    }
}

// §2.2 step 8: the goal can be walked to straight.
void Brain::walkStraight(Entry &e, const Dots &goal) {
    Record r{e};
    forget(e);
    r.set<int64_t>(E_TARGET, 0);
    const Dots me = r.position();
    Vec d = blocksBetween(me, goal);
    const float radius = r.f<float>(E_RADIUS);
    float speed = WALK_SPEED;
    const float match = r.f<float>(E_MATCH_SPEED);
    if (0.01f < match && lengthSquared(d) < ((radius * radius) * 10.0f) * 10.0f)
        speed = match;
    const float flat = d[0] * d[0] + d[1] * d[1];
    if (d[2] * d[2] + flat < (radius * radius) * 4.0f)
        speed *= 0.5f;
    if (!isWater(blockAtDots(w, me)))
        d[2] = 0.0f;
    const float len2 = d[2] * d[2] + flat;
    if (len2 > 0.0f) {
        const float k = 1.0f / std::sqrt(len2);
        d = {d[0] * k, d[1] * k, d[2] * k};
    }
    const Vec direction = {d[0] * speed, d[1] * speed, d[2] * speed};
    putVec(r.raw(), E_DIRECTION, direction);
    if (0.001f < lengthSquared(direction))
        r.set<uint8_t>(E_MOVING, 1);
    int32_t budget = r.f<int32_t>(E_BUDGET);
    budget -= budget / 4;
    r.set<int32_t>(E_BUDGET, budget < BUDGET_MIN ? BUDGET_MIN : budget);
}

// §2.3 Phase C.
void Brain::handOver() {
    for (Creature *c : w.creatures) {
        if (!runsHere(c))
            continue;
        if (kindOf(c) == PLAYER && !(stateOf(c) & CONTROLLED))
            continue;
        if (!(healthOf(c) > 0.0f) || !(stunOf(c) <= 0.0f))
            continue;
        setStateBit(c, SPARED, false);
        auto it = ai.find(idOf(c));
        if (it == ai.end())
            continue;
        Entry &e = it->second;
        Record r{e};
        putVec(c->record, INTENT, vecAt(r.raw(), E_DIRECTION));
        if (r.f<uint8_t>(E_JUMP))
            setAbility(c, JUMP);
        else if (abilityOf(c) == JUMP)
            setAbility(c, 0);
        c->interactions.insert(c->interactions.end(), e.waiting.begin(), e.waiting.end());
        // Arrived at the object it was going to (a seat): use it.
        const int32_t index = r.f<int32_t>(E_OBJECT_INDEX);
        if (index < 0)
            continue;
        const int zx = r.f<int32_t>(E_OBJECT_ZONE), zy = r.f<int32_t>(E_OBJECT_ZONE + 4);
        const std::vector<Object> *objects = w.objects ? w.objects(zx, zy) : nullptr;
        if (!objects || index >= (int32_t)objects->size())
            continue;
        const Vec d = blocksBetween(dotsAt((*objects)[index].data(), 8), positionOf(c));
        if (!(lengthSquared(d) < 25.0f))
            continue;
        objectInteraction(c->interactions, zx, zy, index);
        putVec(c->record, INTENT, {0, 0, 0});
        r.stop();
        r.set<int64_t>(E_OBJECT_ZONE, -1);
        r.set<int32_t>(E_OBJECT_INDEX, -1);
    }
}

// §2.4 Phase D.
void Brain::dropRecords() {
    std::vector<int64_t> gone;
    for (auto &[id, e] : ai) {
        Creature *c = find(id);
        if (!c || (w.client && c != w.local))
            gone.push_back(id);
    }
    for (int64_t id : gone)
        ai.erase(id);
}

// §3.1, 0x14e00: one creature's turn.
void Brain::turn(Creature *c) {
    if (flagsOf(c) & AI_OFF)
        return;
    if (classOf(c) == CLASS_STILL) {
        if (duration(c) < timeOf(c)) {
            setAbility(c, STILL);
            setTime(c, 0.0f);
        }
        return;
    }
    // Riding its owner, a mount.
    if (kindOf(c) != OBJECT_A && kindOf(c) != OBJECT_B && ownerOf(c) != 0) {
        Creature *o = find(ownerOf(c));
        if (o && healthOf(o) > 0.0f && !(stunOf(o) > 0.0f) && raceOf(o) == RACE_MOUNT) {
            setAbility(c, RIDING);
            put(c->record, LINKED, idOf(o));
            return;
        }
    }
    if ((flagsOf(c) & CALLS_OUT) && kindOf(c) == TOWNSPERSON)
        callOut(c);
    if (stateOf(c) & ASLEEP)
        return;

    const uint32_t flags = aiFlagsOf(c);
    if (!(flags & RIDES_ALONG) || !rideAlong(c)) {
        if (kindOf(c) == OBJECT_A || kindOf(c) == OBJECT_B) {
            if (flags & ATTACHES)
                attach(c);
        } else {
            if ((flags & GUARDS) && guard(c))
                return;
            if (!fight(c)) {
                if (!(flags & COMPANION)) {
                    if (!greet(c) && !followGroup(c))
                        idle(c);
                } else {
                    followOwner(c);
                }
            }
        }
    }
    if (lengthSquared(vecAt(c->record, INTENT)) > 0.0f && (moveFlagsOf(c) & OPENS_DOORS))
        openDoors(c);

    // Asleep-at-night marker: set for creatures that keep to the day, at night.
    const bool day = DAY_START <= w.day && w.day < DAY_END;
    const bool awake = abilityOf(c) == SIT || day || nativerules::raceGroup(raceOf(c)) != 3;
    setStateBit(c, NIGHT, !awake);
}

// §3.1 step 4: a townsperson calls out when it first sees a player (at most every 4 s).
void Brain::callOut(Creature *c) {
    const uint32_t now = w.clock ? w.clock() : 0;
    const bool asleep = (stateOf(c) & ASLEEP) != 0;
    Record r{entryOf(c)};
    if (!asleep && r.f<uint8_t>(E_WAS_ASLEEP) != 0) {
        r.set<uint8_t>(E_SAW_PLAYER, 0);
        r.set<uint32_t>(E_CALL_TIME, 0);
    }
    const bool saw = r.f<uint8_t>(E_SAW_PLAYER) != 0;
    r.set<uint8_t>(E_SAW_PLAYER, 0);
    for (Creature *p : w.creatures)
        if (kindOf(p) == PLAYER && inSight(w, c, p, SIGHT_RANGE)) {
            r.set<uint8_t>(E_SAW_PLAYER, 1);
            break;
        }
    if (r.f<uint8_t>(E_SAW_PLAYER) && !saw && now - r.f<uint32_t>(E_CALL_TIME) > 4000) {
        w.calls.push_back(idOf(c));
        r.set<uint32_t>(E_CALL_TIME, now);
    }
    r.set<uint8_t>(E_WAS_ASLEEP, asleep ? 1 : 0);
}

// §4.1 step 5: a pet regains a tenth of its health per second.
void Brain::healPet(Creature *c) {
    if (kindOf(c) != PET)
        return;
    const float h = ((maxHealth(c) * (float)dt) * 0.001f) * 0.1f + healthOf(c);
    put(c->record, HEALTH, h);
    if (maxHealth(c) < h)
        put(c->record, HEALTH, maxHealth(c));
}

// §4.1, 0x11df0: a creature that rides along with its owner does what the owner does.
bool Brain::rideAlong(Creature *c) {
    Creature *o = find(ownerOf(c));
    if (!o)
        return false;
    uint8_t *me = c->record;
    const uint8_t *ow = o->record;
    put(me, STATE, get<uint16_t>(ow, STATE));
    me[ABILITY] = ow[ABILITY];
    put(me, ABILITY_TIME, get<float>(ow, ABILITY_TIME));
    put(me, MANA, get<float>(ow, MANA));
    put(me, CHARGE, get<float>(ow, CHARGE));
    copyItem(me + ITEM, ow + ITEM);
    if (dodgeOf(o) > 0.0f && dodgeOf(c) <= 0.0f) {
        put(me, DODGE_TIMER, dodgeOf(o));
        putVec(me, VELOCITY, vecAt(ow, VELOCITY));
    }
    // The owner's aim, but from where this creature is.
    const Vec ownerAim = vecAt(ow, AIM);
    const Dots op = positionOf(o), cp = positionOf(c);
    putVec(me, AIM, ownerAim);
    put(me, AIM + 0, (float)((op[0] + trunc64(ownerAim[0] * BLOCK)) - cp[0]) * DOT);
    put(me, AIM + 8, (float)((op[2] + trunc64(ownerAim[2] * BLOCK)) - cp[2]) * DOT);
    put(me, AIM + 4, (float)((op[1] + trunc64(ownerAim[1] * BLOCK)) - cp[1]) * DOT);
    // Near its owner (within 32 blocks) or not.
    int64_t sum = 0;
    for (int i = 0; i < 3; i++) {
        const int64_t d = op[i] - cp[i];
        const int64_t sq = (int64_t)((uint64_t)d * (uint64_t)d);
        sum += sq / 65536;
    }
    setStateBit(c, HURRY, !(sum < 0x400001));
    healPet(c);

    const Vec d = blocksBetween(cp, op);
    Entry &e = entryOf(c);
    const float s = lengthSquared(d);
    if (25.0f <= s) {
        if (kindOf(c) != PET || s <= 10000.0f || !(moveFlagsOf(o) & (ON_GROUND | IN_WATER)) || abilityOf(c) == SWIM_OFF) {
            follow(e, o, 5.0f, false, true);
            return true;
        }
        putDots(me, POSITION, op);
        return false;
    }
    Record{e}.stop();
    putVec(me, INTENT, {0, 0, 0});
    return false;
}

// §4.2, 0x13530: an object creature attaches itself to the nearest monster.
void Brain::attach(Creature *c) {
    setAbility(c, 0);
    if (Creature *held = find(get<int64_t>(c->record, LINKED)); held && healthOf(held) > 0.0f) {
        setAbility(c, CARRIED);
        return;
    }
    float nearest = 25.0f;
    const Dots cp = positionOf(c);
    for (Creature *m : w.creatures) {
        if (kindOf(m) != MONSTER)
            continue;
        const Dots mp = positionOf(m);
        const float dx = (float)(mp[0] - cp[0]);
        const float dy = (float)(mp[1] - cp[1]) * DOT, dz = (float)(mp[2] - cp[2]) * DOT;
        const float s = ((dx * DOT) * dx) * DOT + dy * dy + dz * dz;
        if (!(s < nearest))
            continue;
        put(c->record, LINKED, idOf(m));
        setAbility(c, CARRIED);
        const float a = (float)rand(), b = (float)rand(), cc = (float)rand();
        // Somewhere on the monster's body.
        put(c->record, AIM + 4, (1.0f - (b + b) / RAND_SCALE) * sizeOf(m, 1) * 0.5f);
        put(c->record, AIM + 8,
            (1.0f - (a + a) / RAND_SCALE) * (sizeOf(m, 2) - sizeOf(c, 2)) * 0.5f + sizeOf(c, 2) * 0.5f);
        put(c->record, AIM + 0, (1.0f - (cc + cc) / RAND_SCALE) * sizeOf(m, 0) * 0.5f);
        nearest = s;
    }
}

// §4.3, 0xb330: stay with the owner and keep its health up.
bool Brain::guard(Creature *c) {
    if (stateOf(c) & BUSY)
        return false;
    Entry &e = entryOf(c);
    Record r{e};
    Creature *o = find(ownerOf(c));
    if (!o || !(healthOf(o) > 0.0f))
        return false;
    const Dots cp = positionOf(c), op = positionOf(o);
    const bool sees = canSee(w, cp.data(), op.data(), true, SIGHT_LIMIT);
    const float reach = sees ? 30.0f : 1.0f;
    const auto healing = [&] { return abilityOf(c) == HEAL_OWNER && timeOf(c) < duration(c); };
    if (reach * reach < lengthSquared(blocksBetween(cp, op)) || !sees) {
        if (stunOf(c) <= 0.0f)
            r.set<int32_t>(E_CHASE_TIMER, r.f<int32_t>(E_CHASE_TIMER) + (int32_t)dt);
        if (r.f<int32_t>(E_CHASE_TIMER) < GIVE_UP_AFTER + 1 || raceOf(c) == RACE_LEASHED) {
            follow(e, o, reach, false, false);
            return healing();
        }
        c->aggro.clear();
        forget(e);
        r.set<int64_t>(E_TARGET, 0);
        r.set<int32_t>(E_GIVE_UP, GIVE_UP_COOLDOWN);
        r.set<int32_t>(E_CHASE_TIMER, 0);
        return false;
    }
    if (abilityOf(c) != HEAL_OWNER) {
        if (duration(c) <= timeOf(c)) {
            setAbility(c, HEAL_OWNER);
            put(c->record, LINKED, idOf(o));
            setTime(c, 0.0f);
        }
        if (abilityOf(c) != HEAL_OWNER)
            return false;
    }
    putVec(c->record, AIM, blocksBetween(cp, op));
    for (const nativebuffs::Buff &b : o->buffs)
        if (nativebuffs::source(b) == idOf(c) && b[0] == BUFF_GUARDED && nativebuffs::duration(b) > 500.0f)
            return healing();
    const float before = maxHealth(o);
    const nativebuffs::Buff b = nativebuffs::make(BUFF_GUARDED, 1.1f, 3000.0f, idOf(c));
    nativebuffs::give(*o, b, true);
    w.buffNotes.push_back(nativebuffs::notice(idOf(c), idOf(o), b, 1));
    const float after = maxHealth(o);
    put(o->record, HEALTH, (after / before) * healthOf(o));
    return true;
}

// 0x5f920: whom the creature most wants to fight.
Creature *Brain::preferredTarget(Creature *c) {
    if (const nativebuffs::Buff *taunt = nativebuffs::find(*c, BUFF_TAUNT))
        return find(nativebuffs::source(*taunt));
    float best = 0.0f;
    int64_t id = 0;
    for (const auto &[who, anger] : c->aggro)
        if (who != 0 && anger >= best)
            best = anger, id = who;
    return find(id);
}

// "X calls C": X is calling creatures of C's race to it.
bool Brain::callsFor(const Creature *x, const Creature *c) const {
    return abilityOf(x) == CALLED && x->record[ITEM] == ITEM_CALL && get<int32_t>(x->record, ITEM_SUBTYPE) == raceOf(c);
}

// §4.4, 0xbac0: find a foe and fight it.
bool Brain::fight(Creature *c) {
    if (stateOf(c) & BUSY)
        return false;
    setStateBit(c, PROVOKED, false);
    switch (abilityOf(c)) {
    case 0x50: case 0x56: case 0x57: case 0x58: case 0x59: case 0x7e: case 0x8a:
        break;
    default:
        if (timeOf(c) > 10000.0f)
            setAbility(c, 0);
    }
    if (dodgeOf(c) != 0.0f || (abilityOf(c) == DRINK && timeOf(c) < 3000.0f) ||
        (abilityOf(c) == COWER && timeOf(c) < 5000.0f))
        return true;
    if (abilityOf(c) == DANCE) {
        if (timeOf(c) < duration(c)) {
            putVec(c->record, INTENT, {0, 0, 0});
            return true;
        }
        setAbility(c, 0);
    }

    // Drink a potion when badly hurt.
    if (healthOf(c) < maxHealth(c) * 0.25f && duration(c) < timeOf(c)) {
        for (auto &tab : c->inventory)
            for (auto &stack : tab) {
                const int32_t count = get<int32_t>(stack.data(), 0);
                if (count == 0 || stack[4] != ITEM_CONSUMABLE || get<int32_t>(stack.data(), 8) != 1)
                    continue;
                setAbility(c, DRINK);
                setTime(c, 0.0f);
                copyItem(c->record + ITEM, stack.data() + 4);
                if (count - 1 < 1) {
                    put<int32_t>(stack.data(), 0, 0);
                    stack[4] = 0;
                    put<int32_t>(stack.data(), 8, 0);
                } else {
                    put<int32_t>(stack.data(), 0, count - 1);
                }
                const float a = (float)rand(), b = (float)rand();
                putVec(c->record, INTENT, {(b * 4.0f) / RAND_SCALE - 2.0f, (a * 4.0f) / RAND_SCALE - 2.0f, 0.0f});
                return true;
            }
    }

    Entry &e = entryOf(c);
    Record r{e};
    // A focus it was told to fight.
    const int64_t focus = r.f<int64_t>(E_FOCUS);
    if (focus != 0 && find(focus) && healthOf(c) > 0.0f && c->aggro[focus] < 0.1f)
        c->aggro[focus] = 0.1f;
    if (raceOf(c) == RACE_LEASHED)
        for (auto it = c->aggro.begin(); it != c->aggro.end();) {
            Creature *x = find(it->first);
            it = x && kindOf(x) != PLAYER ? c->aggro.erase(it) : std::next(it);
        }
    for (auto it = c->aggro.begin(); it != c->aggro.end();) {
        Creature *x = it->first != focus ? find(it->first) : nullptr;
        it = x && (healthOf(x) <= 0.0f || (stateOf(x) & ASLEEP)) ? c->aggro.erase(it) : std::next(it);
    }
    Creature *t = preferredTarget(c);
    const Dots cp = positionOf(c);
    if (!t && raceOf(c) == RACE_LEASHED && lengthSquared(blocksBetween(cp, dotsAt(c->record, HOME))) > 900.0f)
        putDots(c->record, POSITION, dotsAt(c->record, HOME));
    // A pet fights whatever hurt its owner most.
    if (kindOf(c) == PET) {
        if (Creature *o = find(ownerOf(c))) {
            t = nullptr;
            if (abilityOf(o) == MOUNTED)
                return false;
            float best = 0.0f;
            for (const auto &[who, dealt] : o->damage) {
                if (!(best < dealt))
                    continue;
                Creature *x = find(who);
                if (x && healthOf(x) > 0.0f)
                    t = x, best = dealt;
            }
        }
    }
    // A hurt townsperson with no foe cowers.
    if (!t && healthOf(c) < maxHealth(c) * 0.5f && kindOf(c) == TOWNSPERSON && !(flagsOf(c) & NO_COWER)) {
        setAbility(c, COWER);
        setTime(c, 0.0f);
        c->record[ITEM] = 1;
        put<int32_t>(c->record, ITEM_SUBTYPE, 4);
        putVec(c->record, INTENT, {0, 0, 0});
        return true;
    }
    int32_t skillTimer = r.f<int32_t>(E_SKILL_TIMER) - (int32_t)dt;
    r.set<int32_t>(E_SKILL_TIMER, skillTimer < 0 ? 0 : skillTimer);
    const bool frozen = hasBuff(c, BUFF_FROZEN);

    // Look for a foe in sight.
    if (!t && (kindOf(c) != PET || ownerOf(c) == 0) && r.f<int32_t>(E_GIVE_UP) < 1) {
        for (Creature *x : w.creatures) {
            if (x == c || !(healthOf(x) > 0.0f) || (stateOf(x) & ASLEEP) || kindOf(x) == PET)
                continue;
            if (raceOf(c) == RACE_LEASHED && kindOf(x) != PLAYER)
                continue;
            if (focus != 0 && idOf(x) != focus)
                continue;
            if (frozen)
                continue;
            if (!(enemies(c, x) || (ownerOf(c) == 0 && callsFor(x, c))))
                continue;
            if (!inSight(w, c, x, SIGHT_RANGE))
                continue;
            if ((flagsOf(c) & PEACEFUL) && !callsFor(x, c))
                continue;
            t = x;
        }
        if (t && c->aggro[idOf(t)] < 0.1f) {
            c->aggro[idOf(t)] = 0.1f;
            aggroNotice(t, c);
        }
    }

    // Hurry while chasing, unless mid-way through an attack.
    if (!t) {
        setStateBit(c, HURRY, false);
    } else {
        if (raceOf(c) == RACE_LEASHED)
            setStateBit(c, PROVOKED, true);
        const int recovery = recoveryTime(c, -1), effect = effectTime(c, -1), middle = middleTime(c, -1);
        const bool busy = abilityOf(c) != 0 && timeOf(c) <= (float)(middle + recovery / 2 + effect);
        setStateBit(c, HURRY, !busy);
    }
    int32_t giveUp = r.f<int32_t>(E_GIVE_UP) - (int32_t)dt;
    r.set<int32_t>(E_GIVE_UP, giveUp < 0 ? 0 : giveUp);
    if (!t) {
        r.set<int32_t>(E_CHASE_TIMER, 0);
        r.set<int32_t>(E_SKILL_TIMER, rand() % SKILL_WAIT);
        return false;
    }
    if (abilityOf(c) == SLEEP || abilityOf(c) == SIT) {
        setAbility(c, 0);
        setTime(c, duration(c));
    }

    Vec d = blocksBetween(cp, positionOf(t));
    const float reach = sizeOf(c, 0) + sizeOf(c, 0) + sizeOf(t, 0);
    const Dots tp = positionOf(t);
    bool sees = canSee(w, cp.data(), tp.data(), true, SIGHT_LIMIT);
    if (abilityOf(t) == TALKING && timeOf(t) < duration(t)) {
        c->aggro[idOf(t)] = 0.0f;
        return false;
    }

    // How near it must get.
    float range = reach;
    if (stateOf(c) & PROVOKED) {
        sees = true;
        range = 0.0f;
        if (duration(c) < timeOf(c)) {
            setAbility(c, ENRAGE);
            setTime(c, 0.0f);
        }
    }
    if (nativerules::isRanged(c->record) && sees) {
        const int a = abilityOf(c);
        if (!((a == 0x5f || a == 0x60 || a == 0x62) && timeOf(c) < (float)effectTime(c, -1)))
            range = 50.0f;
    }
    switch (abilityOf(c)) {
    case 0x69: case 0x1d: case 0x2f: case 0x7b:
        if (timeOf(c) < duration(c))
            range = 50.0f;
        break;
    default:
        break;
    }
    if (abilityOf(c) == 0x88)
        range = 4.0f;
    else if (abilityOf(c) == 0x49)
        range = sizeOf(t, 0) + sizeOf(c, 0);
    Creature *mount = find(get<int64_t>(c->record, MOUNT));
    const bool mounted = mount && raceOf(mount) == RACE_MOUNT && healthOf(mount) > 0.0f;
    if (mounted)
        range = 4.0f;
    const float targetStealth = get<float>(t->record, STEALTH);
    if (targetStealth > 0.0f) {
        float k = 1.0f - targetStealth;
        if (0.1f > k)
            k = 0.1f;
        const float v = (k * k) * range;
        range = reach <= v ? v : reach;
    }

    // Now and then a special skill.
    int skill = 0;
    if (r.f<int32_t>(E_SKILL_TIMER) == 0 && !c->skills.empty()) {
        skill = c->skills[(uint64_t)(int64_t)rand() % c->skills.size()];
        if (rand() % 2 == 0) {
            if (skill == 0x45 || skill == 0x46 || skill == 0x64 || skill == 0x67)
                range = 20.0f;
            if (skill == 0x80 && inWater(c))
                skill = 0;
            else if (skill == 0x5f || skill == 0x62)
                range = reach;
        } else {
            skill = 0;
            r.set<int32_t>(E_SKILL_TIMER, SKILL_WAIT);
        }
    }
    if (!sees)
        range = 1.0f;
    float s = lengthSquared(d);

    // Out of reach: chase, or give up after a while.
    if (!(abilityOf(c) == FLIGHT || (s <= range * range && sees && groundOrWater(c)))) {
        if (stunOf(c) <= 0.0f)
            r.set<int32_t>(E_CHASE_TIMER, r.f<int32_t>(E_CHASE_TIMER) + (int32_t)dt);
        if (r.f<int32_t>(E_CHASE_TIMER) > GIVE_UP_AFTER && raceOf(c) != RACE_LEASHED) {
            c->aggro.clear();
            forget(e);
            r.set<int64_t>(E_TARGET, 0);
            r.set<int32_t>(E_GIVE_UP, GIVE_UP_COOLDOWN);
            r.set<int32_t>(E_CHASE_TIMER, 0);
            return false;
        }
        follow(e, t, range, false, false);
        return true;
    }

    // In reach: stand and fight.
    r.set<int32_t>(E_CHASE_TIMER, 0);
    forget(e);
    r.set<int64_t>(E_TARGET, 0);
    r.set<float>(E_MATCH_SPEED, 0.0f);
    r.stop();

    // Answering a creature that calls it.
    const bool hasMaster = ownerOf(c) != 0 && !(flagsOf(c) & NO_COWER);
    if (!frozen && !(flagsOf(c) & ELITE) && callsFor(t, c) && kindOf(c) != PET && !hasMaster && !(range * range < s)) {
        put(c->record, ANSWERING, idOf(t));
        if (duration(c) < timeOf(c)) {
            if (abilityOf(c) != ANSWER)
                put<int32_t>(c->record, COMBO, 0);
            if (get<int32_t>(c->record, COMBO) < 5) {
                setAbility(c, ANSWER);
                setTime(c, 0.0f);
            } else {
                for (auto it = c->buffs.rbegin(); it != c->buffs.rend(); ++it)
                    if ((*it)[0] == BUFF_REMOVED) {
                        nativebuffs::Buff b = *it;
                        put(b.data(), 4, 0.0f);
                        put(b.data(), 8, 0.0f);
                        buffNotice(c, b, true);
                        break;
                    }
                buffNotice(c, nativebuffs::make(BUFF_FROZEN, 0.0f, 20000.0f, idOf(t)), true);
            }
        }
        return true;
    }
    if (!sees) {
        if (abilityOf(c) == 0x69 || abilityOf(c) == HEAL_OWNER)
            setAbility(c, 0);
        return true;
    }

    d = blocksBetween(positionOf(c), positionOf(t));
    s = lengthSquared(d);
    if (mounted) {
        setAbility(c, MOUNTED);
        setTime(c, 0.0f);
    } else if (abilityOf(c) == FLIGHT) {
        if (duration(c) < timeOf(c) && groundOrWater(c))
            setAbility(c, 0);
    } else if (skill != 0) {
        if (skill != DANCE || onGround(c)) {
            r.set<int32_t>(E_SKILL_TIMER, SKILL_WAIT);
            setAbility(c, skill);
            setTime(c, 0.0f);
            if (skill == FLIGHT) {
                // Fly at the target: twice the offset, to arrive in (distance / 50) s.
                const Dots from = positionOf(c), to = positionOf(t);
                Vec v;
                for (int i = 0; i < 3; i++)
                    v[i] = (float)(((to[i] - from[i]) * 131072) / 65536) * DOT;
                putVec(c->record, VELOCITY, v);
                put(c->record, TRAVEL_TIME, (std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) / 50.0f) * 1000.0f);
                put(c->record, VELOCITY + 8, 20.0f);
            }
        }
    } else if (stateOf(c) & PROVOKED) {
        // Provoked without a skill: nothing more (open question 6).
    } else {
        attack(c, t, d, s);
    }
    putVec(c->record, AIM, d);
    return true;
}

// Leap: jump at the target.
void Brain::leap(Creature *c, Creature *t) {
    setTime(c, 0.0f);
    setAbility(c, LEAP);
    Vec v = blocksBetween(positionOf(c), positionOf(t));
    if (lengthSquared(v) > 0.0f) {
        v = normalised(v);
        for (float &x : v)
            x *= 10.0f;
    }
    putVec(c->record, VELOCITY, v);
}

// Dodge: a quick hop toward, away from or sideways to the target.
void Brain::dodge(Creature *c, Creature *t) {
    const Vec d = blocksBetween(positionOf(c), positionOf(t));
    Vec v = {d[0], d[1], 0.0f};
    if (!(v[0] * v[0] + v[1] * v[1] + 0.0f > 0.0f))
        return;
    v = normalised(v);
    Vec out;
    switch (rand() & 3) {
    case 0:
        out = {v[0] * 20.0f, v[1] * 20.0f, v[2] * 20.0f};
        break;
    case 1:
        out = {v[0] * -20.0f, v[1] * -20.0f, v[2] * -20.0f};
        break;
    case 2: // sideways: the cross product with straight up
        out = {(v[1] - v[2] * 0.0f) * 20.0f, (v[2] * 0.0f - v[0]) * 20.0f, (v[0] * 0.0f - v[1] * 0.0f) * 20.0f};
        break;
    default:
        out = {(v[1] - v[2] * 0.0f) * -20.0f, (v[2] * 0.0f - v[0]) * -20.0f, (v[0] * 0.0f - v[1] * 0.0f) * -20.0f};
        break;
    }
    out[2] += 5.0f;
    putVec(c->record, VELOCITY, out);
    put(c->record, DODGE_TIMER, 600.0f);
}

// §4.4.1: the attack, by what the creature is.
void Brain::attack(Creature *c, Creature *t, Vec &d, float s) {
    const int32_t race = raceOf(c);
    const auto ready = [&] { return duration(c) < timeOf(c); };
    const auto pick = [&](std::initializer_list<int> list) {
        const int r = rand();
        return *(list.begin() + (uint64_t)(int64_t)r % list.size());
    };
    const auto start = [&](int ability) {
        setAbility(c, ability);
        setTime(c, 0.0f);
    };
    const auto release = [&](int ability) {
        setAbility(c, ability);
        setTime(c, (float)effectTime(c, ability));
    };
    const Dots cp = positionOf(c), tp = positionOf(t);

    if (race == RACE_HEAD_74) {
        if (ready()) {
            setAbility(c, pick({0x46, 0x4b, 0x4a}));
            setTime(c, 0.0f);
        }
        return;
    }
    if (race == RACE_HEAD_76) {
        if (ready()) {
            const int last = get<uint16_t>(c->record, LOOK_HEAD) == 0xb39 ? 0x69 : 0x5f;
            setAbility(c, pick({8, 7, 0x15, last}));
            setTime(c, 0.0f);
        }
        if (abilityOf(c) == 0x69) {
            int64_t from[3];
            shotPoint(c->record, from);
            d = blocksBetween({from[0], from[1], from[2]}, tp);
        }
        return;
    }
    if (race == RACE_HEAD_77) {
        if (ready()) {
            const auto look = [&](size_t at, uint16_t base) { return (uint16_t)(get<uint16_t>(c->record, at) - base); };
            std::vector<int> list;
            list.push_back(look(LOOK_HEAD, 0xb32) < 3 ? 70 : 69);
            if (look(LOOK_FEET, 0xb36) < 2)
                list.push_back(71);
            list.push_back(look(LOOK_BODY, 0xb2e) < 2 ? 76 : 75);
            if (look(LOOK_HANDS, 0xb2b) < 2)
                list.push_back(78), list.push_back(79);
            if (look(LOOK_HANDS, 0xb26) < 5)
                list.push_back(77);
            const bool notMid = abilityOf(c) != 0x48 && abilityOf(c) != 0x49;
            if (notMid && get<uint16_t>(c->record, LOOK_HANDS) == 0xb24 && rand() % 2 == 1)
                list.push_back(72);
            if (notMid && get<uint16_t>(c->record, LOOK_HANDS) == 0xb25 && rand() % 2 == 1)
                list.push_back(73);
            if (abilityOf(c) != 0x4a)
                list.push_back(74);
            if (!list.empty()) {
                setAbility(c, list[(uint64_t)(int64_t)rand() % list.size()]);
                setTime(c, 0.0f);
            }
        }
        return;
    }
    if (nativerules::usesMana(c->record)) {
        if (!(s < 3600.0f))
            return;
        if (manaOf(c) <= 0.5f)
            startHit(c, false);
        else if (rand() % 2 == 0)
            startHit(c, false);
        else
            startCharge(c, false);
        const int a = abilityOf(c);
        if (a >= 0x1f && a <= 0x22) {
            // A beam stays on the point it was aimed at once it is under way.
            if ((float)effectTime(c, -1) < timeOf(c) - (float)(int32_t)dt)
                d = blocksBetween(cp, dotsAt(c->record, AIM_POINT));
            else
                putDots(c->record, AIM_POINT, dotsAt(t->record, TARGET_POINT));
        }
        if (a == HEAL_OWNER && 25.0f < s) {
            const float angle = timeOf(c) * 0.005f;
            d[0] += nativecrt::cosf(angle) * 1.5f;
            d[1] += nativecrt::sinf(angle) * 1.5f;
        }
        return;
    }
    if (race == RACE_65) {
        if (64.0f <= s) {
            if (s < 3600.0f && ready()) {
                setTime(c, 0.0f);
                setAbility(c, 0x26);
            }
        } else if (timeOf(c) > 2000.0f) {
            if (rand() % 2 == 0 || cp[2] - tp[2] > 524288)
                leap(c, t);
            else
                put(c->record, VELOCITY + 8, 30.0f);
        }
        return;
    }
    if (race == RACE_123) {
        startHit(c, false);
        return;
    }
    if ((flagsOf(c) & LEAPER) == LEAPER) {
        if (s < 64.0f && timeOf(c) > 2000.0f) {
            if (rand() % 2 == 0 || cp[2] - tp[2] > 131072)
                leap(c, t);
            else
                put(c->record, VELOCITY + 8, 5.0f);
        }
        return;
    }
    // A dodge, now and then, for small creatures between attacks.
    const auto mayDodge = [&] {
        return dodgeOf(c) == 0.0f && rand() % 10 == 0 && !(flagsOf(c) & NO_DODGE) && sizeOf(c, 0) < 2.0f &&
               duration(c) <= timeOf(c) && groundOrWater(c);
    };
    const int a = abilityOf(c);
    const int32_t subB = slotSubtype(c, SLOT_B);
    if (classOf(c) == 0x95 || classOf(c) == 0x96 || subB == 6 || subB == 7 || subB == 8) {
        // Bows, crossbows and the like.
        if (!(s < 3600.0f))
            return;
        if ((!(a >= 0x19 && a <= 0x1c) || a == 0x1b) && mayDodge()) {
            dodge(c, t);
            return;
        }
        if (0.8 <= (double)manaOf(c) || a == 0x19 || a == 0x1a || a == 0x1c) {
            if (dodgeOf(c) == 0.0f && ready()) {
                // Hold the draw until charged as far as the mana allows, then shoot.
                const auto draw = [&](int hold, int shoot) {
                    const float most = manaOf(c) * 0.9f < 0.8f ? manaOf(c) * 0.9f : 0.8f;
                    if (abilityOf(c) == hold && most <= get<float>(c->record, CHARGE)) {
                        start(shoot);
                    } else {
                        if (abilityOf(c) != hold)
                            setTime(c, 0.0f);
                        setAbility(c, hold);
                    }
                };
                const int m = chargeMove(c, false);
                if (m == 0x19)
                    draw(0x19, 0x17);
                else if (m == 0x1c)
                    draw(0x1c, 0x1b);
                else if (m == 0x1a)
                    draw(0x1a, 0x38);
                else
                    startCharge(c, false);
            }
        } else {
            startHit(c, false);
        }
        return;
    }
    // Melee.
    const float melee = (sizeOf(t, 0) + sizeOf(c, 0)) * 3.0f;
    if (!(s < melee * melee))
        return;
    if (a != 9 && a != 0x3c && a != 0x40 && a != 0x41 && mayDodge()) {
        dodge(c, t);
        return;
    }
    if (!(0.8f <= manaOf(c) || a == 9 || a == 0x19 || a == 0x1a || a == 0x1c || a == 0x3c || a == 0x40)) {
        startHit(c, false);
        return;
    }
    if (!(dodgeOf(c) == 0.0f && ready()))
        return;
    const int m = chargeMove(c, false);
    const bool charged = manaOf(c) <= get<float>(c->record, CHARGE);
    const bool already = a == m;
    switch (m) {
    case 6:
        start(6);
        break;
    case 0x3c:
        if (!already)
            start(0x3c);
        else if (charged)
            specOf(c) != 1 ? release(0x3d) : start(0x3e);
        break;
    case 9:
        if (!already)
            start(9);
        else if (charged)
            start(0x74);
        break;
    case 0x41:
        if (!already)
            start(0x41);
        else if (charged)
            release(0x11);
        break;
    case 0x40:
        if (!already)
            start(0x40);
        else if (charged)
            release(0x0c);
        break;
    case 0x1a:
        if (!already)
            start(0x1a);
        else if (charged)
            release(0x38);
        break;
    case 0x19:
        if (!already)
            start(0x19);
        else if (charged)
            release(0x17);
        break;
    case 0x1c:
        if (!already)
            start(0x1b); // the game starts the shot itself here, not the draw
        else if (charged)
            release(0x1b);
        break;
    default:
        startCharge(c, false);
        break;
    }
}

// §4.5, 0x12400: greet a player who comes near.
bool Brain::greet(Creature *c) {
    if (stateOf(c) & BUSY)
        return false;
    Creature *p = nullptr;
    float best = 100.0f;
    const Dots cp = positionOf(c);
    for (Creature *x : w.creatures) {
        if (kindOf(x) != PLAYER || !(healthOf(x) > 0.0f) || !inSight(w, c, x, SIGHT_RANGE))
            continue;
        const float s = lengthSquared(blocksBetween(cp, positionOf(x)));
        if (s < best)
            p = x, best = s;
    }
    if (!p || abilityOf(c) == SLEEP || abilityOf(c) == SIT)
        return false;
    if (abilityOf(p) == TALKING && timeOf(p) < duration(p) && get<int64_t>(p->record, LINKED) == idOf(c))
        return false;
    if (duration(c) < timeOf(c)) {
        Entry &e = entryOf(c);
        Record r{e};
        if (kindOf(c) == TOWNSPERSON && abilityOf(c) != WAVE && abilityOf(c) != GREET && rand() % 3 == 0)
            setAbility(c, WAVE);
        else
            setAbility(c, GREET);
        setTime(c, 0.0f);
        putVec(c->record, AIM, blocksBetween(cp, positionOf(p)));
        r.stop();
        forget(e);
        r.set<int64_t>(E_TARGET, 0);
        r.set<float>(E_MATCH_SPEED, 0.0f);
    }
    if (abilityOf(c) == WAVE)
        putVec(c->record, AIM, blocksBetween(cp, positionOf(p)));
    return true;
}

// §4.6: keep near a leader (the group's or the owner).
bool Brain::followLeader(Creature *c, Creature *l, float far2, float near2, float radius, bool ahead) {
    const Dots lp = positionOf(l);
    if (abilityOf(l) == TELEPORT && timeOf(l) < 500.0f && timeOf(l) < 1000.0f &&
        lengthSquared(blocksBetween(positionOf(c), lp)) > 10000.0f)
        putDots(c->record, POSITION, lp);
    healPet(c);
    if (abilityOf(l) == 0x50)
        setAbility(c, 0x50);
    else if (abilityOf(c) == 0x50)
        setAbility(c, 0);
    if (const nativebuffs::Buff *shared = nativebuffs::find(*l, BUFF_SHARED); shared && !hasBuff(c, BUFF_SHARED))
        buffNotice(c, *shared, true);
    Dots q;
    if (!groundAhead(w, l, q))
        q = lp;
    const float s = lengthSquared(blocksBetween(positionOf(c), q));
    setStateBit(c, HURRY, s > far2 || (stateOf(l) & HURRY));
    Entry &e = entryOf(c);
    if (near2 <= s) {
        follow(e, l, radius, ahead, true);
        return true;
    }
    Record r{e};
    r.stop();
    forget(e);
    r.set<int64_t>(E_TARGET, 0);
    r.set<float>(E_MATCH_SPEED, 0.0f);
    return false;
}

// 0x103f0: the first living member of its group leads (none if it comes first itself).
bool Brain::followGroup(Creature *c) {
    for (int64_t id : c->group) {
        if (id == idOf(c))
            return false;
        Creature *l = find(id);
        if (l && healthOf(l) > 0.0f)
            return followLeader(c, l, 256.0f, 64.0f, 8.0f, false);
    }
    return false;
}

// 0xdff0: a companion keeps near its owner.
bool Brain::followOwner(Creature *c) {
    Creature *o = find(ownerOf(c));
    if (!o)
        return false;
    return followLeader(c, o, 64.0f, 16.0f, 4.0f, kindOf(c) == PET);
}

// Take off: an ability that carries the creature (kind 1 flying, 2 swimming).
void Brain::takeOff(Creature *c, int ability, int kind) {
    setAbility(c, ability);
    setTime(c, 0.0f);
    c->record[ITEM] = ITEM_TAKE_OFF;
    put<int32_t>(c->record, ITEM_SUBTYPE, kind);
    uint32_t x = (uint32_t)idOf(c);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    put<int32_t>(c->record, ITEM_MODIFIER, (int32_t)(x % 10 + 1));
}

// §4.7, 0x10b90: what a creature does with nothing else to do.
bool Brain::idle(Creature *c) {
    if (stateOf(c) & BUSY)
        return false;
    setStateBit(c, HURRY, false);
    Entry &e = entryOf(c);
    Record r{e};
    const int32_t timer = r.f<int32_t>(E_IDLE_TIMER) - (int32_t)dt;
    r.set<int32_t>(E_IDLE_TIMER, timer < 0 ? 0 : timer);
    const Dots cp = positionOf(c), home = dotsAt(c->record, HOME);
    const uint32_t flags = aiFlagsOf(c);

    // Edge guard: on a ledge, step back toward home.
    if (sizeOf(c, 2) < 6.0f && classOf(c) != CLASS_FLYER && onGround(c)) {
        const int64_t z = cp[2] - trunc64(sizeOf(c, 2) * 0.5f * BLOCK);
        if (!isSolid(blockAtDots(w, {cp[0], cp[1], z})) && !isSolid(blockAtDots(w, {cp[0], cp[1], z - ONE_BLOCK})) &&
            !isSolid(blockAtDots(w, {cp[0], cp[1], z - 2 * ONE_BLOCK}))) {
            const Vec h = blocksBetween(cp, home);
            const float flat = h[0] * h[0] + h[1] * h[1];
            if (h[2] * h[2] + flat > 0.0f && flat + 0.0f > 0.01f) {
                const float k = 1.0f / std::sqrt(flat + 0.0f);
                putVec(r.raw(), E_DIRECTION, {h[0] * k * 10.0f, h[1] * k * 10.0f, k * 0.0f * 10.0f});
                putVec(c->record, VELOCITY, {0, 0, 0});
            }
        }
    }

    // Too far from home: go back.
    const auto sq = [](int64_t v) { return (int64_t)((uint64_t)v * (uint64_t)v) / 65536; };
    const int64_t away = sq(cp[0] - home[0]) + sq(cp[1] - home[1]);
    if (away > 0x4000000 || ((flags & STAYS_HOME) && away > 0x100000)) {
        goTo(e, home);
        r.set<float>(E_MATCH_SPEED, 0.0f);
        startSearch(e);
        if (abilityOf(c) != 0)
            return true;
        if (classOf(c) == CLASS_SWIMMER && inWater(c))
            takeOff(c, SWIM_OFF, 2);
        if (abilityOf(c) != 0 || classOf(c) != CLASS_FLYER)
            return true;
        if (!inWater(c))
            takeOff(c, LAND, 1);
        return true;
    }
    if (r.f<int32_t>(E_IDLE_TIMER) != 0 || !groundOrWater(c))
        return true;
    if (flags & STAYS_HOME) {
        r.stop();
        setAbility(c, SIT);
        setState(c, stateOf(c) & NIGHT);
        return true;
    }

    // What it may do.
    struct Seat {
        int zx, zy, index;
    };
    std::vector<Seat> seats;
    std::vector<uint32_t> options;
    if (flags & IDLE_SEAT) {
        const auto zoneOf = [](int64_t v) { return (int)((v / ONE_BLOCK) / 64); };
        for (int zx = zoneOf(cp[0] - 0x400000); zx <= zoneOf(cp[0] + 0x400000); zx++)
            for (int zy = zoneOf(cp[1] - 0x400000); zy <= zoneOf(cp[1] + 0x400000); zy++) {
                const std::vector<Object> *objects = w.objects ? w.objects(zx, zy) : nullptr;
                if (!objects)
                    continue;
                for (int i = 0; i < (int)objects->size(); i++) {
                    const Object &o = (*objects)[i];
                    const int32_t type = get<int32_t>(o.data(), 0);
                    const bool seat = type == 0x1c || type == 0x1e || type == 0x1f || type == 0x5f || type == 0x60 ||
                                      type == 0x73;
                    if (get<int64_t>(o.data(), 0x30) == 0 && seat)
                        seats.push_back({zx, zy, i});
                }
            }
        if (!seats.empty())
            options.push_back(IDLE_SEAT);
    }
    for (uint32_t option : {IDLE_STROLL, IDLE_SLEEP, IDLE_DANCE, IDLE_STAND, IDLE_CELL})
        if (flags & option)
            options.push_back(option);

    const auto rest = [&](int ability) {
        r.stop();
        r.set<int32_t>(E_IDLE_TIMER, rand() % 8000 + 5000);
        setAbility(c, ability);
    };
    if (options.empty()) {
        r.stop();
        r.set<int32_t>(E_IDLE_TIMER, rand() % 2000 + 1000);
        setAbility(c, 0);
    } else {
        switch (options[(uint64_t)(int64_t)rand() % options.size()]) {
        case IDLE_SEAT: {
            setStateBit(c, HURRY, false);
            setAbility(c, 0);
            r.set<int32_t>(E_IDLE_TIMER, rand() % 8000 + 5000);
            const Seat seat = seats[(uint64_t)(int64_t)rand() % seats.size()];
            const std::vector<Object> *objects = w.objects ? w.objects(seat.zx, seat.zy) : nullptr;
            if (!objects || seat.index >= (int)objects->size())
                break;
            const Dots at = dotsAt((*objects)[seat.index].data(), 8);
            if (!canSee(w, cp.data(), at.data(), false, 5.0f)) {
                goTo(e, at);
                if (w.objectKey)
                    e.taken.insert(w.objectKey(seat.zx, seat.zy, seat.index));
                r.set<int32_t>(E_OBJECT_ZONE, seat.zx);
                r.set<int32_t>(E_OBJECT_ZONE + 4, seat.zy);
                r.set<int32_t>(E_OBJECT_INDEX, seat.index);
                r.set<float>(E_MATCH_SPEED, 0.0f);
                startSearch(e);
            } else {
                objectInteraction(c->interactions, seat.zx, seat.zy, seat.index);
                putVec(c->record, INTENT, {0, 0, 0});
                r.stop();
            }
            break;
        }
        case IDLE_STAND:
            rest(0);
            break;
        case IDLE_SLEEP:
            rest(SLEEP);
            break;
        case IDLE_STROLL: {
            setStateBit(c, HURRY, false);
            const float a = (float)rand(), b = (float)rand();
            putVec(r.raw(), E_DIRECTION,
                   {((b + b) / RAND_SCALE - 1.0f) * 10.0f, ((a + a) / RAND_SCALE - 1.0f) * 10.0f, 0.0f});
            r.set<int32_t>(E_IDLE_TIMER, rand() % 5000 + 3000);
            setAbility(c, 0);
            break;
        }
        case IDLE_DANCE:
            rest(DANCE);
            setTime(c, 0.0f);
            break;
        case IDLE_CELL: {
            // Walk to a spot in a cell (16 blocks) up to 5 cells away.
            const int cx = rand() % 11 - 5 + floorDiv(blockOf(cp[0]), 16);
            const int cy = rand() % 11 - 5 + floorDiv(blockOf(cp[1]), 16);
            const int zx = floorDiv(cx * 16, 64), zy = floorDiv(cy * 16, 64);
            if (!w.zone || !w.zone(zx, zy))
                break;
            setStateBit(c, HURRY, false);
            r.set<int32_t>(E_IDLE_TIMER, rand() % 10000 + 20000);
            setAbility(c, 0);
            int64_t spot[3];
            if (!w.spot || !w.spot(cx, cy, spot))
                break;
            goTo(e, {spot[0], spot[1], spot[2]});
            r.set<float>(E_MATCH_SPEED, 0.0f);
            startSearch(e);
            break;
        }
        default:
            break;
        }
    }
    if (abilityOf(c) == 0) {
        if (classOf(c) == CLASS_SWIMMER && inWater(c))
            takeOff(c, SWIM_OFF, 2);
        if (abilityOf(c) == 0 && classOf(c) == CLASS_FLYER && onGround(c)) {
            takeOff(c, LAND, 1);
            put(c->record, VELOCITY + 8, 20.0f);
        }
    }
    if (abilityOf(c) == LAND)
        setStateBit(c, HURRY, true);
    return true;
}

// §4.8: a moving creature that opens doors opens every closed door its box touches.
void Brain::openDoors(Creature *c) {
    if (!w.objects)
        return;
    const Dots cp = positionOf(c);
    const auto zoneOf = [](int64_t v) { return (int)(v >> 22); };
    const int64_t ax = trunc64((double)sizeOf(c, 0) * 0.5 * 65536.0), ay = trunc64((double)sizeOf(c, 1) * 0.5 * 65536.0);
    const int64_t bx = trunc64((double)sizeOf(c, 0) * 0.5 * -65536.0), by = trunc64((double)sizeOf(c, 1) * 0.5 * -65536.0);
    const int64_t a[2] = {trunc64(sizeOf(c, 0) * 0.5f * BLOCK), trunc64(sizeOf(c, 1) * 0.5f * BLOCK)};
    const int64_t h = trunc64(sizeOf(c, 2) * 0.5f * BLOCK);
    for (int zx = zoneOf(cp[0] - ax - ONE_BLOCK); zx <= zoneOf(cp[0] - bx + ONE_BLOCK); zx++)
        for (int zy = zoneOf(cp[1] - ay - ONE_BLOCK); zy <= zoneOf(cp[1] - by + ONE_BLOCK); zy++) {
            const std::vector<Object> *objects = w.objects(zx, zy);
            if (!objects)
                continue;
            for (int i = 0; i < (int)objects->size(); i++) {
                const Object &o = (*objects)[i];
                const int32_t type = get<int32_t>(o.data(), 0);
                if (!(type == 3 || type == 4 || type == 5 || type == 0x69) || o[0x28] == 0)
                    continue;
                const worldgen::ObjectSize base = worldgen::objectSize(type);
                std::array<float, 3> size = {base.x, base.y, base.z};
                const float scale = get<float>(o.data(), 0x24);
                for (float &v : size)
                    v *= scale;
                if (o[0x20] & 1)
                    std::swap(size[0], size[1]);
                const Dots op = dotsAt(o.data(), 8);
                bool overlaps = true;
                for (int k = 0; k < 2; k++) {
                    const int64_t b = trunc64(size[k] * 0.5f * BLOCK);
                    overlaps &= op[k] - b <= cp[k] + ONE_BLOCK + a[k] && cp[k] - a[k] - ONE_BLOCK < op[k] + b;
                }
                overlaps &= op[2] <= cp[2] + h && cp[2] - h < trunc64(size[2] * BLOCK) + op[2];
                if (overlaps)
                    objectInteraction(c->interactions, zx, zy, i);
            }
        }
}

// §5.2, 0xfe60: follow creature T until within `radius` of it.
void Brain::follow(Entry &e, Creature *t, float radius, bool ahead, bool matchSpeed) {
    Record r{e};
    Dots goal = positionOf(t);
    if (r.f<uint8_t>(E_AHEAD)) {
        Dots q;
        if (groundAhead(w, t, q))
            goal = q;
    }
    settle(w, t, goal);
    putDots(r.raw(), E_GOAL, goal);
    setStartFromPosition(r);
    r.set<float>(E_RADIUS, (radius + r.size(0)) + sizeOf(t, 0));
    r.set<int64_t>(E_TARGET, idOf(t));
    r.set<uint8_t>(E_AHEAD, ahead ? 1 : 0);
    const Vec intent = vecAt(t->record, INTENT);
    r.set<float>(E_MATCH_SPEED, matchSpeed && !ahead ? std::sqrt(lengthSquared(intent)) : 0.0f);
    startSearch(e);
}

} // namespace


// 0x5fc40: the next hit of the creature's normal attack.
int nextHit(Creature *c, bool force) {
    const uint8_t cls = classOf(c);
    const bool spec1 = specOf(c) == 1;
    if (boosted(c, force)) {
        if (cls == 3)
            return spec1 ? 0x87 : 0x61;
        if (cls == 4 && spec1)
            return 0x6c;
    }
    if (raceOf(c) == RACE_123)
        return 0x9e;
    switch (cls) {
    case 0x91: case 0x92: return within(c, 7) ? 8 : 7;
    case 0x93: return within(c, 2) ? 3 : 2;
    case 0x94: return within(c, 0x42) ? 0x43 : 0x42;
    case 0x95: case 0x96: return 0x17;
    case 0x97: return 0x68;
    case 0x98: return spec1 ? 0x2d : 0x27;
    default: break;
    }
    const int32_t w = slotSubtype(c, SLOT_B);
    if (w == 0x17)
        return 0x4c;
    if (w != 0x16) {
        if (nativerules::usesMana(c->record)) {
            if (w == 0xb)
                return spec1 ? 0x6b : 0x68;
            if (w == 10)
                return spec1 ? 0x21 : 0x1f;
            if (!spec1)
                return within(c, 0x29) ? 0x28 : 0x29;
            return within(c, 0x2b) ? 0x2a : 0x2b;
        }
        if (raceOf(c) == RACE_68 || weaponIs(c, SLOT_B, 6) || weaponIs(c, SLOT_B, 7) || weaponIs(c, SLOT_A, 6) ||
            weaponIs(c, SLOT_A, 7))
            return 0x17;
        if (w == 3)
            return within(c, 0x14) ? 0x13 : 0x14;
        if (w == 4 || slotType(c, SLOT_B) == 0)
            return within(c, 7) ? 8 : 7;
        if (w == 5)
            return within(c, 0xf) ? 0xe : 0xf;
        if (weaponIs(c, SLOT_B, 8))
            return 0x1b;
        if (nativerules::holdsTwoHanded(c->record))
            return within(c, 0x3a) ? 0x44 : within(c, 0x44) ? 0x3b : 0x3a;
        if (weaponIs(c, SLOT_A, 13))
            return within(c, 0xb) ? 10 : 0xb;
    }
    return within(c, 2) ? 3 : 2;
}

// 0x60460: start the next hit, when the last one is far enough along and it can pay for it.
bool startHit(Creature *c, bool force) {
    if (dodgeOf(c) != 0.0f || abilityOf(c) == STRONG_CHARGE)
        return false;
    int t = nativerules::abilityDuration(c->record, buffTest(c));
    if (get<int32_t>(c->record, COMBO) != 0 && abilityOf(c) != 0x1f && abilityOf(c) != 0x21)
        t -= recoveryTime(c, -1);
    const int m = nextHit(c, force);
    if ((float)t <= timeOf(c) && manaCost(c, m) <= manaOf(c) && staminaCost(c, m) <= get<float>(c->record, STAMINA)) {
        setAbility(c, m);
        setTime(c, 0.0f);
        return true;
    }
    return false;
}

// 0x60720: start charging the strong attack.
bool startCharge(Creature *c, bool force) {
    const int a = abilityOf(c);
    if (a == 0x93 || a == STRONG_CHARGE || a == CALLED)
        return false;
    int t = nativerules::abilityDuration(c->record, buffTest(c));
    if (get<int32_t>(c->record, COMBO) != 0)
        t -= recoveryTime(c, -1);
    const int m = chargeMove(c, force);
    const bool held = m == 9 || m == 0x19 || m == 0x1a || m == 0x1c || m == 0x3c || m == 0x40;
    const bool quick = m == 6 || m == 0x12 || m == 0x15;
    if (held || (quick && timeOf(c) < duration(c))) {
        if (m == a)
            return false;
        if (a != 0x0c && a != 0x3d && a != 0x5f && a != 0x74)
            t = a == 0 ? 200 : 0;
    }
    const bool keepsDodge = classOf(c) == 4 && specOf(c) == 1 && quick;
    if (timeOf(c) <= (float)t)
        return false;
    if (dodgeOf(c) != 0.0f && !keepsDodge)
        return false;
    if (manaOf(c) < manaCost(c, m))
        return false;
    if ((quick || m == STRONG_CHARGE) && manaOf(c) < 0.1f)
        return false;
    const int move = chargeMove(c, force);
    setAbility(c, move);
    setTime(c, 0.0f);
    if ((move == 0x20 || move == 0x22 || move == 0x26 || move == 0x30 || move == 0x69) && hasBuff(c, BUFF_FAST_CAST))
        setTime(c, (float)effectTime(c, move));
    if (!keepsDodge)
        put(c->record, DODGE_TIMER, 0.0f);
    return true;
}

bool canUse(Creature *c, int ability) {
    return nativerules::canStartAbility(c->record, ability, buffTest(c), &c->cooldowns);
}

// --- Sight and rays ----------------------------------------------------------------------

bool canSee(World &world, const int64_t *from, const int64_t *to, bool glass, float range) {
    const Dots a = {from[0], from[1], from[2]}, b = {to[0], to[1], to[2]};
    if (fartherThan(a, b, range))
        return false;
    const Vec d = blocksBetween(a, b);
    const float s = (d[0] * d[0] + d[1] * d[1]) + d[2] * d[2];
    if (0.0001f > s)
        return true;
    const float len = std::sqrt(s);
    const float u[3] = {d[0] / len, d[1] / len, d[2] / len};
    return ray(world, from, u, len, glass) >= len - 0.01f;
}

float ray(World &world, const int64_t *from, const float *dir, float limit, bool glass) {
    const auto passes = [glass](const Block &b) { return (b[4] == 0 || b[4] == 2) && (!glass || !(b[5] & 0x40)); };
    return gridWalk(world, {from[0], from[1], from[2]}, {dir[0], dir[1], dir[2]}, limit, passes);
}

// §5.14, 0x654a0.
void shotPoint(const uint8_t *record, int64_t out[3]) {
    const Dots p = dotsAt(record, POSITION);
    out[0] = p[0], out[1] = p[1], out[2] = p[2];
    if (!(get<uint32_t>(record, FLAGS) & SHOOTS_FROM_BODY))
        return;
    const float a = get<float>(record, HEADING) * DEGREES;
    const float u = get<float>(record, SIZE + 4) * 0.5f, v = get<float>(record, SIZE + 8) * 0.35f;
    out[0] += trunc64((-nativecrt::sinf(a) * u) * BLOCK);
    out[1] += trunc64((nativecrt::cosf(a) * u) * BLOCK);
    out[2] += trunc64(v * BLOCK);
}

// §2, 0x13830.
void tick(World &world, std::map<int64_t, Entry> &ai, uint32_t dt) { Brain(world, ai, dt).run(); }

} // namespace nativeai
