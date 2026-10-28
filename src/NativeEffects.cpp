// The effect objects' step (0x2dba40), written from docs/specs/effect-objects.md §0-§2.
//
// Once per world step every effect object moves, meets the creatures and blocks in its way
// and then either flies on, bursts, turns into a lasting area, or runs out. Section numbers
// in the comments refer to that spec; RVAs are only citations of the game's routines.
#include "NativeEffects.h"

#include "GameMath.h"
#include "NativeBuffs.h"
#include "worldgen/Residents.h"
#include "NativeRules.h"

#include <cmath>
#include <cstring>

namespace nativeeffects {
namespace {

using nativeai::Creature;

// ---------------------------------------------------------------------------------------
// Record layouts

// The effect record (§0.1).
constexpr size_t FX_OWNER = 0x00, FX_POSITION = 0x08, FX_TARGET = 0x20, FX_VELOCITY = 0x44, FX_DAMAGE = 0x50,
                 FX_RADIUS = 0x54, FX_SIZE = 0x58, FX_CHARGE = 0x5c, FX_NO_CONTACT = 0x64, FX_KIND = 0x68,
                 FX_HIT_TYPE = 0x6c, FX_AGE = 0x70, FX_LIFETIME = 0x74, FX_FREEZE = 0x78;

// Creature fields beyond those worldgen::creature names (§0.3, creature-hit.md §0.1).
constexpr size_t CR_ABILITY_TIME = 0x6c, CR_COMBO = 0x70, CR_SINCE_HIT = 0x74, CR_SIZE = 0x84, CR_DODGE = 0x12c,
                 CR_FREEZE = 0x138, CR_AIM = 0x174, CR_MANA = 0x188, CR_WEAPON_B = 0x6d8;

// A sound request (0x30 bytes), a particle burst (0x48), a damage number (0x48) and a
// block-change record (0x20) as the world lists keep them (creature-hit.md §0.3-§0.4, §0.5).
constexpr size_t SND_POSITION = 0x00, SND_ID = 0x18, SND_PITCH = 0x1c, SND_VOLUME = 0x20, SND_EMITTER = 0x28;
constexpr size_t PT_POSITION = 0x00, PT_VELOCITY = 0x18, PT_COLOUR = 0x24, PT_SIZE = 0x34, PT_COUNT = 0x38,
                 PT_STYLE = 0x3c, PT_SPREAD = 0x40;
constexpr size_t NUM_ATTACKER = 0x00, NUM_TARGET = 0x08, NUM_AMOUNT = 0x10, NUM_CRITICAL = 0x14, NUM_POSITION = 0x20;
constexpr size_t BC_BLOCK = 0x00, BC_OLD = 0x0c, BC_NEW = 0x12, BC_WORLD = 0x18;

// Effect kinds (§0.2).
enum Kind : int32_t {
    ARROW = 0, ARROW_B = 1, ARROW_C = 2, // gravity, stop at the first creature
    SPELL = 3,                           // straight, bursts on contact or a wall
    HOMING = 4,                          // homes on its target point, bursts there
    BOOMERANG = 5,                       // flies out and comes back to its owner
    AREA = 6,                            // stays put and works on whoever stands in it
    BOLT = 7,                            // like an arrow
    DRIFT = 8,                           // just moves
    HOVER = 9,                           // floats above the terrain
    HOMING_AREA = 10,                    // homes like 4, then becomes an area
};

// Hit types with meaning here (§0.1).
constexpr uint8_t HIT_PHYSICAL = 0, HIT_DAZE = 1, HIT_HEAL = 2, HIT_SLOW = 4, HIT_POISON = 6;

// Buff types used here.
constexpr uint8_t BUFF_POISON = 4, BUFF_SPIRIT_9 = 9, BUFF_SPIRIT_10 = 0x0a, BUFF_STATUS = 0x0e, BUFF_MANA_DRAIN = 0x22;

// Sound ids.
constexpr int32_t SOUND_WHOOSH = 0x0f, SOUND_HIT_ARROW = 0x14, SOUND_DEBRIS = 0x1a, SOUND_HOMING_AREA = 0x22,
                  SOUND_BURST = 0x2a, SOUND_BURST_HEAL = 0x2d, SOUND_POISON = 0x2d, SOUND_SLOW = 0x30,
                  SOUND_PROC = 0x32, SOUND_HOMING = 0x54;

// Constants (§4).
constexpr float FIXED = 65536.0f, TO_BLOCKS = 1.0f / 65536.0f, RAND_MAX_F = 32767.0f, MS = 0.001f;
constexpr int64_t BLOCK = 0x10000;
constexpr float GRAVITY = 30.0f, GRAVITY_SHARE = 0.25f;
constexpr float HOMING_SPEED = 50.0f, HOMING_AREA_SPEED = 20.0f, HOMING_VELOCITY_RATE = 0.002f,
                HOMING_POSITION_RATE = 0.005f, HOMING_START = 500.0f, HOMING_REACH = 2.0f;
constexpr float BOOMERANG_GONE = 1200.0f, BOOMERANG_PULL = 1100.0f, BOOMERANG_RETURN = 800.0f,
                BOOMERANG_RETURN_SPEED = 10.0f, BOOMERANG_AIM_SPEED = 5.0f, BOOMERANG_RATE = 0.05f,
                BOOMERANG_STRIKE_AGE = 300.0f;
constexpr float HOVER_RATE = 0.01f, HOVER_RISE = 1000.0f;
constexpr int64_t HOVER_LIFT = 150 * BLOCK;
constexpr float SIGHT_RANGE = 200.0f, DESTROY_RADIUS_SQUARED = 16.0f;
constexpr int DESTROY_REACH = 4;
constexpr float PROC_CHANCE = 0.15f;
constexpr float AREA_RADIUS_HOMING = 10.0f, AREA_RADIUS = 5.0f, AREA_FROM_HOMING = 16.0f, AREA_DAMAGE = 0.05f,
                AREA_FRESH_AGE = 0.0f, AREA_LEFTOVER_AGE = 3000.0f;

// ---------------------------------------------------------------------------------------
// Small helpers

template <class T> T read(const uint8_t *p, size_t offset) {
    T v;
    std::memcpy(&v, p + offset, sizeof v);
    return v;
}
template <class T> void write(uint8_t *p, size_t offset, T v) { std::memcpy(p + offset, &v, sizeof v); }

float toBlocks(int64_t fixed) { return (float)fixed * TO_BLOCKS; }
int64_t toFixed(float blocks) { return (int64_t)(blocks * FIXED); }
int floorBlock(int64_t v) { return (int)(v >> 16); }
int truncBlock(int64_t v) { return (int)(v / BLOCK); }
float fromRand(int r) { return (float)r; }

// Whether the n-ms boundary was crossed during this step (§0.6), `age` already advanced.
bool crossed(float age, float dt, int n) { return (int)(age - dt) / n != (int)age / n; }

// 0xf3e20 (creature-hit.md §4.2): whether q is farther than d blocks from p.
bool farther(const int64_t *p, const int64_t *q, float d) {
    const double dx = (double)(p[0] - q[0]) * (1.0 / 65536.0), dy = (double)(p[1] - q[1]) * (1.0 / 65536.0),
                 dz = (double)(p[2] - q[2]) * (1.0 / 65536.0);
    return std::fabs(dx) > d || std::fabs(dy) > d || dx * dx + dy * dy + dz * dz > (double)(d * d);
}

// The creature's view the stat rules want.
float f32(const Creature *c, size_t offset) { return read<float>(c->record, offset); }
int64_t idOf(const Creature *c) { return read<int64_t>(c->record, worldgen::creature::ID); }
const int64_t *positionOf(const Creature *c) {
    return reinterpret_cast<const int64_t *>(c->record + worldgen::creature::POSITION);
}
uint8_t hostilityOf(const Creature *c) { return c->record[worldgen::creature::HOSTILITY]; }

nativerules::HasBuff hasBuffOf(const Creature *c) {
    return [c](int type) { return nativebuffs::find(*c, (uint8_t)type) != nullptr; };
}

// 0x66460, the attack-speed factor.
float attackSpeed(const Creature *c) { return nativerules::attackSpeed(c->record, hasBuffOf(c)); }

// 0x5f8d0 (stat-rules.md §6.1): mana gains shrink with a type-0x22 buff's strength.
float manaFactor(const Creature *c) {
    const nativebuffs::Buff *b = nativebuffs::find(*c, BUFF_MANA_DRAIN);
    return b ? 1.0f - nativebuffs::strength(*b) * 0.1f : 1.0f;
}

// 0x5e820 (§2.3): a proc roll, weaker for some weapons in slot B, against the rand `r`.
bool procRoll(const Creature *c, float chance, int r) {
    const uint8_t *weapon = c->record + CR_WEAPON_B;
    if (weapon[0] == 3) {
        const int32_t subtype = read<int32_t>(weapon, 4);
        if (subtype == 0 || subtype == 1 || subtype == 2 || subtype == 12)
            chance = chance * 0.5f;
        else if (subtype == 8 || subtype == 10)
            chance = chance * 0.3f;
    }
    return chance > fromRand(r) / RAND_MAX_F;
}

// 0x50d30 (§2.1): the chance that the owner's hit on `target` is critical.
float critChance(const Creature *owner, const Creature *target, bool magic) {
    const std::vector<nativerules::Buff> buffs = nativebuffs::forStats(*target);
    return nativerules::critChance(owner->record, hasBuffOf(owner), target->record, buffs.data(), buffs.size(), magic);
}

// A wall or a lost arrow ends the owner's combo, keeping its ability's progress in step with
// the slower attack speed that follows (§1.4 step 3a, §1.8 step 7).
void resetCombo(Creature *c) {
    const float before = attackSpeed(c);
    write<int32_t>(c->record, CR_COMBO, 0);
    const float after = attackSpeed(c);
    write<float>(c->record, CR_ABILITY_TIME, (before / after) * f32(c, CR_ABILITY_TIME));
}

// A landed hit raises the owner's combo (§1.8 step 8, §1.9 step 4). (0x603f0, called here by
// the game, changes nothing.)
void raiseCombo(Creature *c) {
    write<int32_t>(c->record, CR_COMBO, read<int32_t>(c->record, CR_COMBO) + 1);
    write<float>(c->record, CR_SINCE_HIT, 0.0f);
}

// ---------------------------------------------------------------------------------------
// The effect as the step works on it

struct Shot {
    int64_t owner = -1;
    int64_t position[3]{}, target[3]{};
    float velocity[3]{};
    float damage = 0, radius = 0, charge = 0;
    uint8_t noContact = 0; // flag F (§0.1)
    int32_t kind = 0;
    uint8_t hitType = 0;
    float age = 0;
    int32_t lifetime = 0, freeze = 0;
};

Shot load(const Effect &e) {
    const uint8_t *p = e.data();
    Shot s;
    s.owner = read<int64_t>(p, FX_OWNER);
    for (int i = 0; i < 3; i++) {
        s.position[i] = read<int64_t>(p, FX_POSITION + 8 * i);
        s.target[i] = read<int64_t>(p, FX_TARGET + 8 * i);
        s.velocity[i] = read<float>(p, FX_VELOCITY + 4 * i);
    }
    s.damage = read<float>(p, FX_DAMAGE);
    s.radius = read<float>(p, FX_RADIUS);
    s.charge = read<float>(p, FX_CHARGE);
    s.noContact = p[FX_NO_CONTACT];
    s.kind = read<int32_t>(p, FX_KIND);
    s.hitType = p[FX_HIT_TYPE];
    s.age = read<float>(p, FX_AGE);
    s.lifetime = read<int32_t>(p, FX_LIFETIME);
    s.freeze = read<int32_t>(p, FX_FREEZE);
    return s;
}

// Writes back what the step may change: place, velocity, age, and what becoming an area sets.
void store(const Shot &s, Effect &e) {
    uint8_t *p = e.data();
    for (int i = 0; i < 3; i++) {
        write<int64_t>(p, FX_POSITION + 8 * i, s.position[i]);
        write<float>(p, FX_VELOCITY + 4 * i, s.velocity[i]);
    }
    write<float>(p, FX_DAMAGE, s.damage);
    write<float>(p, FX_RADIUS, s.radius);
    write<float>(p, FX_CHARGE, s.charge);
    p[FX_NO_CONTACT] = s.noContact;
    write<int32_t>(p, FX_KIND, s.kind);
    write<float>(p, FX_AGE, s.age);
}

bool stopsAtFirst(int32_t kind) { // kinds 0-3 and 7: the first creature met ends the flight
    return kind == ARROW || kind == ARROW_B || kind == ARROW_C || kind == SPELL || kind == BOLT;
}
bool isHoming(int32_t kind) { return kind == HOMING || kind == HOMING_AREA; }

// ---------------------------------------------------------------------------------------
// One call of the step: the world, the sight test over its blocks, and the stack values the
// game reuses between requests it builds (§1.7.5 and Open questions 1, 2).

struct Step {
    World &world;
    nativeai::World sight;
    int32_t lastSoundId = 0;
    int64_t lastNoticeSource = 0;

    explicit Step(World &w) : world(w) {
        sight.block = w.block;
        sight.zone = w.zone;
    }

    int rand() { return world.rand ? world.rand() : 0; }
    bool isLocal(const Creature *c) const { return c && world.local && c->record == world.local; }
    Creature *byId(int64_t id) const { return world.byId ? world.byId(id) : nullptr; }

    void sound(const int64_t *at, int32_t id, float pitch, float volume, int64_t emitter = 0) {
        std::array<uint8_t, 0x30> s{};
        std::memcpy(&s[SND_POSITION], at, 24);
        write<int32_t>(s.data(), SND_ID, id);
        write<float>(s.data(), SND_PITCH, pitch);
        write<float>(s.data(), SND_VOLUME, volume);
        write<int64_t>(s.data(), SND_EMITTER, emitter);
        world.sounds.push_back(s);
        lastSoundId = id;
    }

    struct Burst {
        float velocity[3];
        float colour[4];
        float size;
        int32_t count, style;
        float spread;
    };
    void particles(const int64_t *at, const Burst &b) {
        std::array<uint8_t, 0x48> p{};
        std::memcpy(&p[PT_POSITION], at, 24);
        std::memcpy(&p[PT_VELOCITY], b.velocity, 12);
        std::memcpy(&p[PT_COLOUR], b.colour, 16);
        write<float>(p.data(), PT_SIZE, b.size);
        write<int32_t>(p.data(), PT_COUNT, b.count);
        write<int32_t>(p.data(), PT_STYLE, b.style);
        write<float>(p.data(), PT_SPREAD, b.spread);
        world.particles.push_back(p);
    }

    // Gives a buff (0x65390) and tells the world of it. Without a source creature the
    // notice's source is whatever an earlier notice of this call left (Open question 2).
    void giveBuff(Creature *to, const Creature *from, const nativebuffs::Buff &b, bool refresh) {
        nativebuffs::give(*to, b, refresh);
        if (from)
            lastNoticeSource = idOf(from);
        world.buffNotes.push_back(nativebuffs::notice(lastNoticeSource, idOf(to), b, 0));
    }

    // Line of sight 0x2b5d90 with "glass blocks sight" on (§1.7.3).
    bool sees(const int64_t *from, const Creature *c) {
        return nativeai::canSee(sight, from, positionOf(c), true, SIGHT_RANGE);
    }

    bool visit(Effect &record, float dt);
};

// ---------------------------------------------------------------------------------------
// One visit of one effect (§1.2-§1.9)

struct Visit {
    Step &step;
    World &world;
    Shot &s;
    Creature *owner; // O (§1.2), null when gone
    float dt;
    float push[3]{};
    bool touched = false, landed = false;
    Creature *struck = nullptr;

    // §1.3, kinds 4 and 10: steer toward the target point, and after half a second pull the
    // effect itself toward it.
    void home(float speed) {
        float d[3];
        for (int i = 0; i < 3; i++)
            d[i] = toBlocks(s.target[i] - s.position[i]);
        const float lengthSquared = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
        if (lengthSquared > 1.0f) {
            const float q = 1.0f / std::sqrt(lengthSquared);
            for (float &v : d)
                v = v * q;
        }
        const float f = gamemath::blendFactor(dt, HOMING_VELOCITY_RATE);
        for (int i = 0; i < 3; i++)
            s.velocity[i] = s.velocity[i] + (d[i] * speed - s.velocity[i]) * f;
        if (s.age > HOMING_START) {
            const float g = gamemath::blendFactor(dt, HOMING_POSITION_RATE);
            float e[3];
            for (int i = 0; i < 3; i++)
                e[i] = toBlocks(s.target[i] - s.position[i]);
            for (int i = 0; i < 3; i++)
                s.position[i] += toFixed(e[i] * g);
        }
    }

    // §1.3, kind 5: out along the owner's aim, then back to the owner, pulled in at the end.
    // False when the boomerang is gone.
    bool returnToOwner() {
        if (!owner || s.age > BOOMERANG_GONE)
            return false;
        const int64_t *home = positionOf(owner);
        if (s.age > BOOMERANG_PULL) {
            // The pull grows with the square of how far into its last 100 ms it is.
            const int64_t m = toFixed((s.age - BOOMERANG_PULL) / 100.0f);
            int64_t gap[3];
            for (int i = 0; i < 3; i++)
                gap[i] = home[i] - s.position[i];
            for (int i = 0; i < 3; i++) {
                int64_t t = (int64_t)((uint64_t)gap[i] * (uint64_t)m) / BLOCK;
                t = (int64_t)((uint64_t)t * (uint64_t)m) / BLOCK;
                s.position[i] += t;
            }
        } else if (s.age > BOOMERANG_RETURN) {
            for (int i = 0; i < 3; i++)
                s.velocity[i] = toBlocks(home[i] - s.position[i]) * BOOMERANG_RETURN_SPEED;
        } else {
            float change[3];
            for (int i = 0; i < 3; i++) {
                const int64_t aimed = home[i] + toFixed(f32(owner, CR_AIM + 4 * i));
                change[i] = toBlocks(aimed - s.position[i]) * BOOMERANG_AIM_SPEED - s.velocity[i];
            }
            const float f = gamemath::blendFactor(dt, BOOMERANG_RATE);
            for (int i = 0; i < 3; i++)
                s.velocity[i] = change[i] * f + s.velocity[i];
        }
        return true;
    }

    void fall() { s.velocity[2] = s.velocity[2] - ((dt * MS) * GRAVITY) * GRAVITY_SHARE; }

    // §1.5, kind 9: drift, and ease the height toward a point above the terrain that sinks
    // as the effect ages.
    void hover() {
        const float f = 1.0f - (s.age + s.age) / (float)s.lifetime;
        const float f4 = (f * f) * (f * f);
        float ground = world.ground ? world.ground(truncBlock(s.position[0]), truncBlock(s.position[1])) : 0.0f;
        if (!(ground > 0.0f))
            ground = 0.0f;
        const int64_t goal = toFixed(ground) + (toFixed(f4 * HOVER_RISE) + HOVER_LIFT);
        const float g = gamemath::blendFactor(dt, HOVER_RATE);
        s.position[2] += (int64_t)((float)(goal - s.position[2]) * g);
    }

    // §1.7.2: whether creature C's box and a box of `radius` around the effect overlap.
    bool touches(const Creature *c, float radius) const {
        const int64_t r = toFixed(radius);
        const int64_t *at = positionOf(c);
        int64_t half[3];
        for (int i = 0; i < 3; i++)
            half[i] = toFixed(f32(c, CR_SIZE + 4 * i) * 0.5f);
        for (int i = 0; i < 3; i++)
            if (s.position[i] + r < at[i] - half[i])
                return false;
        for (int i = 0; i < 3; i++)
            if (at[i] + half[i] <= s.position[i] - r)
                return false;
        return true;
    }

    // §1.7.4: a dodging creature is never struck; the owner remembers it dodged.
    bool dodges(Creature *c, bool retestHostility) {
        if (f32(c, CR_DODGE) == 0.0f)
            return false;
        if (owner && (!retestHostility || nativerules::hostile(owner->record, c->record)))
            owner->struck.insert(idOf(c));
        return true;
    }

    nativeupkeep::Hit hitOn(const Creature *c, float damage, bool critical) const {
        nativeupkeep::Hit h;
        h.target = idOf(c);
        h.attacker = owner ? idOf(owner) : 0;
        h.damage = damage;
        h.critical = critical;
        h.flag = s.noContact;
        h.charge = s.charge;
        std::copy(s.position, s.position + 3, h.where);
        std::copy(push, push + 3, h.push);
        h.type = s.hitType;
        h.ability = 0;
        h.extra = 0; // (the game passes the effect's address; the hit does not read it)
        h.last = 1;  // blockable
        return h;
    }

    bool strike(const Creature *c, float damage, bool critical) {
        return world.hit && world.hit(hitOn(c, damage, critical));
    }

    // §1.7.5: an area of hit type 1, 4 or 6 dazes, slows or poisons whoever stands in it.
    void statusArea(Creature *c) {
        if (s.hitType == HIT_POISON) {
            if (!nativebuffs::find(*c, BUFF_POISON))
                step.giveBuff(c, owner, nativebuffs::make(BUFF_POISON, s.damage, 4000.0f, 0), true);
        } else {
            step.giveBuff(c, owner, nativebuffs::make(BUFF_STATUS, 0.0f, 200.0f, 0), true);
        }
        if (!crossed(s.age, dt, 500))
            return;
        Step::Burst b{{0.0f, 0.0f, 5.0f}, {0.6f, 0.5f, 0.3f, 1.0f}, 0.4f, 15, 0, 3.0f};
        if (s.hitType == HIT_DAZE)
            b.style = 1, b.colour[0] = 1.0f, b.colour[1] = 0.5f, b.colour[2] = 0.3f;
        else if (s.hitType == HIT_POISON)
            b.style = 2, b.colour[0] = 0.2f, b.colour[1] = 1.0f, b.colour[2] = 0.1f;
        step.particles(positionOf(c), b);
        const float pitch = (fromRand(step.rand()) * 0.1f) / RAND_MAX_F + 0.8f;
        // A daze area's sound id is never set: it repeats the last one this call made.
        const int32_t id =
            s.hitType == HIT_SLOW ? SOUND_SLOW : s.hitType == HIT_POISON ? SOUND_POISON : step.lastSoundId;
        step.sound(positionOf(c), id, pitch, 1.0f);
    }

    // §1.7.7: a contact hit gives its owner a little mana.
    void gainMana() {
        const int r1 = step.rand();
        float g = (1.0f - (fromRand(r1) + fromRand(r1)) / RAND_MAX_F) * 0.05f + 0.1f;
        if (s.kind == BOOMERANG)
            g = g * 0.2f;
        g = g * ((float)nativerules::abilityDuration(owner->record, hasBuffOf(owner)) / 300.0f);
        float m = manaFactor(owner) * g;
        if (step.rand() % 8 == 0)
            m = m + m;
        if (!world.client || step.isLocal(owner)) {
            float mana = f32(owner, CR_MANA) + m;
            if (mana > 1.0f)
                mana = 1.0f;
            write<float>(owner->record, CR_MANA, mana);
        }
    }

    // §1.7.8 and §1.9 step 2.5: the critical roll, its rand drawn first.
    bool rollCritical(const Creature *c) {
        if (!owner)
            return false;
        const int r = step.rand();
        return critChance(owner, c, s.hitType != HIT_PHYSICAL) > fromRand(r) / RAND_MAX_F;
    }

    // §1.7.9: the owner's procs on a contact hit.
    void ownerProcs() {
        if (!(step.isLocal(owner) || (!world.client && hostilityOf(owner) != 0)))
            return;
        if (!procRoll(owner, PROC_CHANCE, step.rand()) || s.noContact)
            return;
        auto proc = [&](uint8_t type) {
            step.giveBuff(owner, owner, nativebuffs::make(type, 0.0f, 30000.0f, 0), false);
            step.sound(positionOf(owner), SOUND_PROC, 1.0f, 1.0f);
        };
        if (owner->record[worldgen::creature::CLASS] == 2 && owner->record[worldgen::creature::SPEC] == 1)
            proc(BUFF_SPIRIT_10);
        if (s.kind == SPELL && s.hitType == HIT_DAZE)
            proc(BUFF_SPIRIT_9);
    }

    // §1.7.11, hit type 2: a friendly creature is healed by the damage.
    void heal(Creature *c, float amount, bool critical) {
        if (!owner || nativerules::hostile(owner->record, c->record))
            return;
        const uint8_t kind = hostilityOf(c);
        if (kind == 7 || kind == 8)
            return;
        std::array<uint8_t, 0x48> number{};
        write<int64_t>(number.data(), NUM_ATTACKER, s.owner);
        write<int64_t>(number.data(), NUM_TARGET, idOf(c));
        write<float>(number.data(), NUM_AMOUNT, -amount);
        number[NUM_CRITICAL] = critical;
        std::memcpy(&number[NUM_POSITION], positionOf(c), 24);
        world.numbers.push_back(number);
        if (!world.client || (step.isLocal(owner) && step.isLocal(c))) {
            world.numbers.push_back(number);
            if (world.apply)
                world.apply(number);
        }
    }

    // §1.7.6-§1.7.12 for a creature the effect reached. False when the contact must go on to
    // the next creature without the stop rule (§1.7.10).
    bool contact(Creature *c) {
        if (owner && s.noContact == 0 && s.kind != AREA && s.kind != BOLT && s.kind != SPELL)
            gainMana();
        const bool critical = rollCritical(c);
        float damage = s.damage;
        if (critical)
            damage = damage + damage;
        if (owner)
            ownerProcs();
        const float dealt = damage > 0.0f ? damage : 0.0f;
        if (owner && hostilityOf(owner) == 0 && !step.isLocal(owner))
            return false; // another player's effect: their own game applies it
        if (s.hitType == HIT_HEAL) {
            heal(c, dealt, critical);
        } else {
            struck = c;
            if (strike(c, dealt, critical))
                landed = true;
        }
        if ((float)s.freeze > f32(c, CR_FREEZE))
            write<float>(c->record, CR_FREEZE, (float)s.freeze);
        return true;
    }

    // Whether this visit's contact strikes (§1.7.6).
    bool strikesNow() const {
        if (stopsAtFirst(s.kind) || s.kind == HOMING)
            return s.noContact == 0;
        if (s.kind == AREA)
            return crossed(s.age, dt, 500);
        return s.age > BOOMERANG_STRIKE_AGE && s.kind == BOOMERANG && crossed(s.age, dt, 100);
    }

    // §1.7: the creature pass of one sub-step.
    void creaturePass() {
        touched = landed = false;
        struck = nullptr;
        if (isHoming(s.kind))
            return; // homing shots meet no creature
        const bool healingArea = s.kind == AREA && s.hitType == HIT_HEAL;
        for (size_t i = 0; i < world.creatures.size(); i++) {
            Creature *c = world.creatures[i];
            if (f32(c, worldgen::creature::HEALTH) <= 0.0f)
                continue;
            if (!healingArea && owner && (c == owner || !nativerules::hostile(owner->record, c->record)))
                continue;
            if (!touches(c, s.radius) || !step.sees(s.position, c)) {
                if (touched)
                    return;
                continue;
            }
            if (dodges(c, true))
                continue;
            if (s.kind == AREA && !world.client &&
                (s.hitType == HIT_DAZE || s.hitType == HIT_SLOW || s.hitType == HIT_POISON))
                statusArea(c);
            if (s.hitType == HIT_SLOW)
                continue;
            if (strikesNow()) {
                if (s.kind != AREA && s.kind != BOOMERANG)
                    touched = true;
                if (!contact(c))
                    continue;
            }
            // §1.7.13: the stop rule.
            if (stopsAtFirst(s.kind) || s.kind == HOMING_AREA) {
                touched = true;
                return;
            }
        }
    }

    // §1.8 step 6: blocks near a stopped effect that effects destroy are broken.
    void breakBlocks() {
        bool any = false;
        for (int a = -DESTROY_REACH; a <= DESTROY_REACH; a++)
            for (int b = -DESTROY_REACH; b <= DESTROY_REACH; b++)
                for (int c = -DESTROY_REACH; c <= DESTROY_REACH; c++) {
                    const int64_t q[3] = {s.position[0] + a * BLOCK, s.position[1] + b * BLOCK,
                                          s.position[2] + c * BLOCK};
                    const float ox = toBlocks(q[0] - s.position[0]), oy = toBlocks(q[1] - s.position[1]),
                                oz = toBlocks(q[2] - s.position[2]);
                    if (!(ox * ox + oy * oy + oz * oz < DESTROY_RADIUS_SQUARED))
                        continue;
                    const nativeai::Block old = world.blockAt ? world.blockAt(q[0], q[1], q[2]) : nativeai::Block{};
                    if (!(old[5] & 1))
                        continue;
                    const float k = 1.0f / 128.0f;
                    step.particles(q, {{0.0f, 0.0f, 10.0f}, {old[0] * k, old[1] * k, old[2] * k, 1.0f}, 0.5f, 3, 0, 3.0f});
                    if (world.removeBlock)
                        world.removeBlock(floorBlock(q[0]), floorBlock(q[1]), floorBlock(q[2]));
                    std::array<uint8_t, 0x20> change{};
                    const int32_t cell[3] = {floorBlock(q[0]), floorBlock(q[1]), floorBlock(q[2])};
                    std::memcpy(&change[BC_BLOCK], cell, 12);
                    std::memcpy(&change[BC_OLD], old.data(), 6);
                    static const uint8_t empty[6] = {0xff, 0xff, 0xff, 0, 0, 0};
                    std::memcpy(&change[BC_NEW], empty, 6);
                    write<uint32_t>(change.data(), BC_WORLD, world.changeCount);
                    world.blockChanges.push_back(change);
                    any = true;
                }
        if (!any)
            return;
        step.sound(s.position, SOUND_DEBRIS, (fromRand(step.rand()) * 0.4f) / RAND_MAX_F + 0.5f, 1.0f);
        if (world.rebuild)
            world.rebuild(truncBlock(s.position[0] - DESTROY_REACH * BLOCK),
                          truncBlock(s.position[1] - DESTROY_REACH * BLOCK),
                          truncBlock(s.position[0] + DESTROY_REACH * BLOCK),
                          truncBlock(s.position[1] + DESTROY_REACH * BLOCK));
    }

    // §1.8 step 5: whether the moved effect ran into a block. A solid block puts it back;
    // an open one is checked with a ray back along the whole step's path.
    bool hitsBlock(const int64_t move[3]) {
        const nativeai::Block b =
            world.blockAt ? world.blockAt(s.position[0], s.position[1], s.position[2]) : nativeai::Block{};
        if (b[4] != 0 && b[4] != 2) {
            for (int i = 0; i < 3; i++)
                s.position[i] -= move[i];
            if (s.kind == ARROW || s.kind == ARROW_B || s.kind == ARROW_C)
                step.particles(s.position, {{0.0f, 0.0f, 10.0f}, {0.5f, 0.5f, 0.5f, 1.0f}, 0.1f, 4, 0, 3.0f});
            return true;
        }
        const float *v = s.velocity;
        const float travelled = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) * (dt * MS);
        const float q = 1.0f / std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        const float back[3] = {q * v[0] * -1.0f, q * v[1] * -1.0f, q * v[2] * -1.0f};
        return travelled > nativeai::ray(step.sight, s.position, back, travelled, true);
    }

    // §1.8: moving one sub-step. True when the effect bursts.
    bool move(const float stepMove[3]) {
        if (s.kind == AREA)
            return false;
        if (s.kind == BOOMERANG && crossed(s.age, dt, 100))
            step.sound(s.position, SOUND_WHOOSH, (fromRand(step.rand()) * 0.1f) / RAND_MAX_F + 1.5f, 0.7f);
        else if (s.kind == HOMING_AREA && crossed(s.age, dt, 200))
            step.sound(s.position, SOUND_WHOOSH, (fromRand(step.rand()) * 0.1f) / RAND_MAX_F + 1.0f, 0.7f);
        int64_t moved[3];
        for (int i = 0; i < 3; i++) {
            moved[i] = toFixed(stepMove[i]);
            s.position[i] += moved[i];
        }
        bool blocked = false;
        if (!isHoming(s.kind)) {
            blocked = hitsBlock(moved);
            if (blocked && !world.client)
                breakBlocks();
        }
        if (!touched && blocked && owner)
            resetCombo(owner);
        if (s.kind == BOOMERANG) {
            if (landed)
                raiseCombo(owner);
            if (blocked)
                s.velocity[0] = s.velocity[1] = s.velocity[2] = 0.0f;
            return false;
        }
        const bool arrived = isHoming(s.kind) && !farther(s.position, s.target, HOMING_REACH) && s.age >= HOMING_START;
        return touched || blocked || arrived;
    }

    // §1.9 step 2: the burst strikes every enemy within its radius.
    void areaPass(float radius) {
        for (size_t i = 0; i < world.creatures.size(); i++) {
            Creature *c = world.creatures[i];
            if (c == struck)
                continue;
            if (owner) {
                if (!world.client && hostilityOf(owner) == 0 && !step.isLocal(owner))
                    return; // another player's burst
                if (c == owner || !nativerules::hostile(owner->record, c->record) ||
                    f32(c, worldgen::creature::HEALTH) <= 0.0f)
                    continue;
            }
            if (!touches(c, radius) || !step.sees(s.position, c))
                continue;
            if (dodges(c, false))
                continue;
            const bool critical = rollCritical(c);
            float damage = s.damage;
            if (critical)
                damage = damage + damage;
            if (strike(c, damage, critical))
                landed = true;
        }
    }

    // §1.9: the burst. True when the effect stays (as an area).
    bool burst() {
        const bool visible = s.kind == SPELL || isHoming(s.kind) || s.noContact != 0;
        if (visible) {
            if (s.kind == HOMING)
                step.sound(s.position, SOUND_HOMING, (fromRand(step.rand()) * 0.4f) / RAND_MAX_F + 1.0f, 1.0f);
            else if (s.kind == HOMING_AREA)
                step.sound(s.position, SOUND_HOMING_AREA, (fromRand(step.rand()) * 0.4f) / RAND_MAX_F + 1.5f, 1.0f);
            const bool spell = s.kind == SPELL || isHoming(s.kind);
            if (spell) {
                Step::Burst b{{0.0f, 0.0f, 10.0f}, {0.25f, 0.5f, 1.0f, 1.0f}, 0.2f, 8, 2, 3.0f};
                if (isHoming(s.kind))
                    b.count = 20, b.size = 0.3f;
                if (s.kind == HOMING || s.hitType == HIT_DAZE)
                    b.style = 1, b.colour[0] = 1.0f, b.colour[1] = 1.0f, b.colour[2] = 0.0f;
                else if (s.kind == HOMING_AREA)
                    b.colour[0] = 0.3f, b.colour[1] = 0.8f, b.colour[2] = 0.2f;
                step.particles(s.position, b);
            }
            if (!touched) {
                const float pitch = (fromRand(step.rand()) * 0.4f) / RAND_MAX_F + 1.0f;
                int32_t id = SOUND_HIT_ARROW;
                if (spell)
                    id = s.hitType == HIT_HEAL || s.hitType == HIT_POISON ? SOUND_BURST_HEAL : SOUND_BURST;
                step.sound(s.position, id, pitch, 1.0f);
            }
            areaPass(isHoming(s.kind) ? AREA_RADIUS_HOMING : AREA_RADIUS);
        }
        if (landed && s.kind != AREA && owner)
            raiseCombo(owner);
        if ((s.kind == SPELL && s.hitType == HIT_HEAL) || s.kind == HOMING_AREA) {
            // A healing spell or a homing area leaves a lasting area where it burst.
            s.radius = s.kind == HOMING_AREA ? AREA_FROM_HOMING : s.charge * 5.0f + 5.0f;
            s.kind = AREA;
            s.age = s.noContact ? AREA_FRESH_AGE : AREA_LEFTOVER_AGE;
            s.damage = s.damage * AREA_DAMAGE;
            s.charge = 0.0f;
            s.noContact = 0;
            return true;
        }
        return false;
    }

    // §1.3-§1.9. False when the effect is to be removed.
    bool run() {
        // §1.3: movement before the sub-steps.
        switch (s.kind) {
        case SPELL:
        case DRIFT:
        case HOVER:
            break;
        case HOMING:
            home(HOMING_SPEED);
            break;
        case HOMING_AREA:
            home(HOMING_AREA_SPEED);
            break;
        case BOOMERANG:
            if (!returnToOwner())
                return false;
            break;
        default:
            fall();
            break;
        }
        // §1.4: the step's displacement, age and expiry.
        const float k = dt * MS;
        const float d[3] = {s.velocity[0] * k, s.velocity[1] * k, s.velocity[2] * k};
        s.age = dt + s.age;
        if (s.kind != BOOMERANG && s.age > (float)s.lifetime) {
            if (s.kind != AREA && owner)
                resetCombo(owner);
            return false;
        }
        // §1.5: kinds that only move.
        if (s.kind == DRIFT || s.kind == HOVER) {
            for (int i = 0; i < 3; i++)
                s.position[i] += toFixed(d[i]);
            if (s.kind == HOVER)
                hover();
            return true;
        }
        // §1.6: sub-steps of at most half a block.
        const int n = (int)(std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]) * 2.0f + 1.0f);
        if (n < 1)
            return true;
        const float stepMove[3] = {d[0] / (float)n, d[1] / (float)n, d[2] / (float)n};
        float hx = s.velocity[0], hy = s.velocity[1];
        const float lengthSquared = s.velocity[0] * s.velocity[0] + s.velocity[1] * s.velocity[1] +
                                    s.velocity[2] * s.velocity[2];
        if (lengthSquared > 0.0f) {
            const float q = 1.0f / std::sqrt(lengthSquared);
            hx = s.velocity[0] * q, hy = s.velocity[1] * q;
        }
        const float c = s.charge * 5.0f;
        push[0] = hx * c, push[1] = hy * c, push[2] = s.charge * 3.0f;
        for (int i = 0; i < n; i++) {
            creaturePass();
            if (move(stepMove))
                return burst();
        }
        return true;
    }
};

// Visits one effect; false when it is to be removed.
bool Step::visit(Effect &record, float dt) {
    Shot s = load(record);
    Visit v{*this, world, s, byId(s.owner), dt};
    const bool keep = v.run();
    store(s, record);
    return keep;
}

} // namespace

Effect made() {
    Effect e{};
    write<int64_t>(e.data(), FX_OWNER, -1);
    write<float>(e.data(), FX_RADIUS, 0.5f);
    write<float>(e.data(), FX_SIZE, 1.0f);
    write<float>(e.data(), FX_CHARGE, 1.0f);
    write<int32_t>(e.data(), FX_LIFETIME, 5000);
    return e;
}

void step(World &world, std::list<Effect> &effects, float dt) {
    Step call(world);
    std::vector<std::list<Effect>::iterator> removed;
    for (auto it = effects.begin(); it != effects.end(); ++it)
        if (!call.visit(*it, dt))
            removed.push_back(it);
    for (auto it : removed)
        effects.erase(it);
}

} // namespace nativeeffects
