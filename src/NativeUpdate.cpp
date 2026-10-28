// A creature's physics step, written from docs/specs/creature-physics.md. The steps run in
// the spec's order; each function says which section it follows. All arithmetic is single
// precision, one rounded operation at a time, in the order the spec gives, unless a comment
// says "in double": the results must match the game's bit for bit.
#include "NativeUpdate.h"

#include "worldgen/Noise.h"
#include "GameMath.h"
#include "NativeCrt.h"
#include "NativeRules.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace nativeupdate {
namespace {

// ---------------------------------------------------------------------------------------
// Where things are in a creature record (byte offsets).

constexpr size_t ID = 0x08;              // i64
constexpr size_t POSITION = 0x10;        // i64 x, y, z (1/65536 block); z is the body's centre
constexpr size_t PITCH = 0x28;           // f32 degrees
constexpr size_t LEAN = 0x2c;            // f32 degrees: the bank into a turn
constexpr size_t YAW = 0x30;             // f32 degrees; forward is (-sin, cos, 0)
constexpr size_t VELOCITY = 0x34;        // f32 x3, blocks a second
constexpr size_t INTENT = 0x40;          // f32 x3: the acceleration the AI or input wants
constexpr size_t KNOCKBACK = 0x4c;       // f32 x3, blocks a second
constexpr size_t TILT = 0x58;            // f32 degrees: the body's tilt
constexpr size_t CONTACT = 0x5c;         // u32 contact flags (below)
constexpr size_t HOSTILITY = 0x60;       // u8
constexpr size_t RACE = 0x64;            // i32 (an object's type)
constexpr size_t ABILITY = 0x68;         // u8, 0 for none
constexpr size_t ABILITY_TIME = 0x6c;    // f32 ms
constexpr size_t COMBO = 0x70;           // i32
constexpr size_t FLAGS = 0x80;           // u32 appearance and physics flags
constexpr size_t SIZE = 0x84;            // f32 x3, blocks
constexpr size_t STATE = 0x128;          // u16 state flags
constexpr size_t DODGE = 0x12c;          // f32 ms left of the dodge
constexpr size_t STUN = 0x130;           // f32 ms left of the stun
constexpr size_t FRICTION_HOLD = 0x134;  // f32: while not zero, no ground friction
constexpr size_t SLOWED = 0x138;         // f32: while not zero, half speed
constexpr size_t HASTED = 0x13c;         // f32: while not zero, 1.5 times the speed
constexpr size_t CLASS = 0x14c;          // u8
constexpr size_t SPECIALISATION = 0x14d; // u8
constexpr size_t CHARGE = 0x158;         // f32
constexpr size_t AIM = 0x174;            // f32 x2
constexpr size_t RISE = 0x17c;           // f32: a ladder object's height
constexpr size_t HEALTH = 0x180;         // f32
constexpr size_t CHARGE_LIMIT = 0x188;   // f32
constexpr size_t GRIP = 0x194;           // f32: the ground's speed factor
constexpr size_t LAMP_RADIUS = 0x198;    // f32
constexpr size_t OWNER = 0x1a0;          // i64 id (pets)
constexpr size_t LINKED = 0x1a8;         // i64 id: the creature it rides or is ridden by
constexpr size_t ITEM_IN_USE = 0x1d8;    // item: type byte +0, subtype i32 +4
constexpr size_t GLIDE_LEVEL = 0x1e0;    // i32: the level the glide factor is taken of
constexpr size_t WEAPON = 0x6d8;         // item: type byte +0, subtype i32 +4
constexpr size_t STEP_OFFSET = 0x990;    // f32 blocks: smooths steps up and down
constexpr size_t STRIDE_STRENGTH = 0x998; // f32
constexpr size_t STRIDE_PHASE = 0x99c;   // f32
constexpr size_t STAMINA = 0x9a0;        // f32
constexpr size_t WETNESS = 0x9a4;        // f32: a pet's owner's, raised while the pet is wet
constexpr size_t WALL_NORMAL = 0x9ac;    // f32 x3
constexpr size_t MOUNT = 0x9d8;          // i64 id
constexpr size_t FACING = 0xb4c;         // f32 x3: the aim it faces
constexpr size_t RELEASED_CHARGE = 0xba8; // f32
constexpr size_t FALL_START = 0xbb0;     // f32 blocks: the height the fall began at
constexpr size_t GLIDE_TIME = 0xbb4;     // f32 ms
constexpr size_t CHARGE_ENDED = 0xbb8;   // u8
constexpr size_t AIR_TIME = 0xbbc;       // f32 ms

// Contact flags (CONTACT). The meanings of the surface kinds are inferred from use.
constexpr uint32_t ON_GROUND = 0x1, IN_WATER = 0x2, ON_WALL = 0x4, AT_SURFACE = 0x10, STUCK = 0x20,
                   IN_PASSAGE = 0x40, ON_LAVA = 0x80, WALL_TYPE_16 = 0x100, ON_SWAMP = 0x200,
                   ON_SURFACE_10 = 0x400, ON_SURFACE_8 = 0x800, WALL_ABOVE_FEET = 0x1000,
                   ON_SURFACE_13 = 0x2000, WALL_FLAG_4 = 0x4000;
constexpr uint32_t SURFACES_CLEARED = 0x2e80; // what a plain block under the feet clears
constexpr uint32_t LADDER = 0x1104;           // on a wall, above the feet, type 16

// Creature flags (FLAGS).
constexpr uint32_t STAYS_LEVEL = 0x1, SLOW_FALL = 0x2, FAST_200 = 0x200, FAST_40000 = 0x40000,
                   NO_UNSTICK = 0x10000, SEES_THROUGH = 0x100000, FIXED_STRIDE = 0x400000,
                   NOT_PUSHED = 0x800000, NO_PUSHING = 0x2000000;
constexpr uint32_t BUOYANT_EXTRA = 0x10100; // without these, summons float higher
// State flags (STATE).
constexpr uint16_t FACES_AIM = 0x4, NO_SIDE_COLLISION = 0x10, SPRINT = 0x40, HELD = 0x1000,
                   STATE_2000 = 0x2000;
constexpr uint16_t NO_GRAVITY_STATES = 0x2012; // also keeps the aim from being faced
constexpr uint16_t NO_STEP_STATES = 0x2002;    // no step-up and no unsticking

// Hostility kinds.
constexpr uint8_t PLAYER = 0, HOSTILITY_3 = 3, FAST_HOSTILE = 5, PET = 6, ORPHANED = 7, SUMMONED = 8;

// Races and object types.
constexpr int32_t WATER_BUBBLE = 0x11d, OBJECT_PASSABLE = 0xfb, OBJECT_LADDER = 0x12e;

// Block types (byte 4) and flags (byte 5).
constexpr uint8_t AIR = 0, WATER = 2, WET = 3, WALL_16 = 0x10, LAVA = 0x11, SWAMP = 0x13;
constexpr uint8_t BLOCK_FLAG_4 = 0x04, BLOCK_PASSAGE = 0x40, BLOCK_PASSABLE_BY_BUFF = 0x80;

// Buff types.
constexpr int BUFF_FRENZY = 0x02, BUFF_POISON = 0x04, BUFF_FULL_CHARGE = 0x0a, BUFF_CHARGE_END = 0x0b,
              BUFF_DOUBLE_SPEED = 0x0c, BUFF_SLOW = 0x0e, BUFF_FIRE_TRAIL = 0x10, BUFF_PASS = 0x13,
              BUFF_LAVA_PROOF = 0x1a, BUFF_COLD_PROOF = 0x1b, BUFF_SWAMP_PROOF = 0x1c, BUFF_COLD = 0x1d;

// Sounds.
constexpr int SOUND_DUST = 1, SOUND_CRASH = 0x17, SOUND_STEP = 0x23, SOUND_WET_STEP = 0x24,
              SOUND_SPLASH = 0x22, SOUND_LAVA = 0x2a;

// Particle styles.
constexpr int STYLE_SWAMP = 0, STYLE_LAVA = 1, STYLE_BLOCK_DUST = 5, STYLE_WATER = 6;

// Units and rules.
constexpr int64_t BLOCK = 65536;
constexpr float BLOCKS_PER_DOT = 1.0f / 65536.0f;
constexpr float RAND_MAX_F = 32767.0f;
constexpr double PI = 3.141592653589793;
constexpr double DEGREES_PER_RADIAN = 57.29577951308232;
constexpr float GRAVITY = 30.0f;
constexpr int64_t STEP_UP_HEIGHT = 66191;     // 1.01 blocks
constexpr int64_t GROUND_PROBE = 13107;       // 0.2 blocks below the feet
constexpr int64_t STEP_DOWN_PROBE = 78643;    // 1.2 blocks below the feet
constexpr int64_t HALF_BLOCK = 32768;
constexpr int64_t FALL_DAMAGE_HEIGHT = 20 * BLOCK;

// The abilities, by what this step does with them. Names are taken from what happens here.
enum : uint8_t {
    LEAP = 0x31,
    CHARGE_RUSH = 0x32,
    RUSH_END = 0x34,
    DIVE = 0x37,
    SPIN = 0x49,
    SNEAK = 0x50,
    RUN_DRAINING = 0x51,
    TIMED_RUSH = 0x52,
    SWIM_SPRINT = 0x53,
    USE_ITEM = 0x55,
    NARROW_57 = 0x57,
    NARROW_58 = 0x58,
    RIDE = 0x76,
    GLIDE = 0x78,
    SAIL = 0x79,
    CLIMB = 0x7a,
    FOLLOW_OWNER = 0x80,
    CARRY = 0x81,
    CARRIED = 0x83,
    STOPS_ON_INTENT = 0x85,
    HOVER = 0x8a,
    CIRCLE = 0x8c,
    FIRE_RUN = 0x92,
    STAMINA_CHARGE = 0x93,
    DASH_94 = 0x94,
    SLEEP = 0x98,
    SLOW_WALK = 0x9a,
    SPECIAL_9C = 0x9c,
    SPECIAL_9D = 0x9d,
    FAST_RUN = 0x9f,
};

// ---------------------------------------------------------------------------------------
// Small helpers.

template <class T> T get(const uint8_t *r, size_t at) {
    T v;
    std::memcpy(&v, r + at, sizeof v);
    return v;
}
template <class T> void put(uint8_t *r, size_t at, T v) { std::memcpy(r + at, &v, sizeof v); }

// A float in blocks to 1/65536 block, toward zero.
int64_t dots(float blocks) { return (int64_t)(blocks * 65536.0f); }
// 1/65536 block to a float in blocks.
float blocks(int64_t dots) { return (float)dots * BLOCKS_PER_DOT; }
// The block a coordinate is in (rounded toward minus infinity).
int32_t floorBlock(int64_t p) { return (int32_t)(p >> 16); }

float smooth(float dt, float rate) { return gamemath::blendFactor((double)dt, rate); }
// Moves x toward a target by a smoothing factor.
void ease(float &x, float target, float factor) { x = factor * (target - x) + x; }
// The same for a 3-vector: the differences first, then each moved (0x16f20).
void ease3(float v[3], const float target[3], float factor) {
    const float d[3] = {target[0] - v[0], target[1] - v[1], target[2] - v[2]};
    for (int i = 0; i < 3; i++)
        v[i] = d[i] * factor + v[i];
}
void easeToZero3(float v[3], float factor) {
    const float zero[3] = {0, 0, 0};
    ease3(v, zero, factor);
}
float lengthSq2(float x, float y) { return x * x + y * y; }
float lengthSq3(const float v[3]) { return (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2]; }
// Normalised with one reciprocal (0x179b0).
void normalise3(float v[3]) {
    const float q = 1.0f / std::sqrt(lengthSq3(v));
    for (int i = 0; i < 3; i++)
        v[i] = v[i] * q;
}
float degrees(float radians) { return (float)((double)radians / PI * 180.0); }
float randUnit(int r, float scale) { return (float)r * scale / RAND_MAX_F; }

// Re-expresses angle b (degrees) as the angle nearest a, moving by `factor` of the way
// (0x17840, section 7.3).
float turn(float a, float b, float factor) {
    const float alpha = (float)((double)(a / 180.0f) * PI);
    const float beta = (float)((double)(b / 180.0f) * PI);
    const double ca = nativecrt::cosf(alpha), sa = nativecrt::sinf(alpha);
    const double cb = nativecrt::cosf(beta), sb = nativecrt::sinf(beta);
    if (cb * ca + sb * sa < -0.9)
        return b;
    float d = (float)(sb * ca - cb * sa);
    d = std::max(std::min(d, 1.0f), -1.0f);
    return (float)((double)nativecrt::asinf(d) * ((double)factor * 180.0 / PI)) + a;
}

// The heading (degrees) of a horizontal unit direction (x, y): forward is (-sin, cos).
float headingOf(float x, float y) {
    const float c = std::max(-1.0f, std::min(1.0f, y));
    const float angle = nativecrt::acosf(c);
    return degrees(x > 0 ? -angle : angle);
}

// The yaw basis (0x13050): right, forward and up of a heading in degrees.
struct Basis {
    float right[3], forward[3], up[3];
};
Basis basisOf(float yaw) {
    const float r = yaw * 0.0174532924f;
    const float c = nativecrt::cosf(r), s = nativecrt::sinf(r);
    return {{c, s, 0}, {-s, c, 0}, {0, 0, 1}};
}

// Races a mount may be (0x5ef80, section 7.6).
bool mountableRace(int32_t race) {
    auto in = [&](int lo, int hi) { return race >= lo && race <= hi; };
    return race == 0x13 || race == 0x14 || race == 0x16 || race == 0x17 || in(0x19, 0x28) || in(0x3f, 0x43) ||
           race == 0x4a || race == 0x4b || in(0x62, 0x64) || in(0x66, 0x69) || race == 0x97 || race == 0x9e ||
           race == 0xa3 || race == 0xa4 || race == 0xa6 || race == 0xad || race == 0xc8 || race == 0xc9 ||
           in(0xd1, 0xd3) || race == 0xd5 || race == 0xd7 || race == 0x106 || race == 0x107 || race == 0x10e ||
           in(0x125, 0x127);
}

// Races that never float (0x53e70 is false for them).
bool sinks(int32_t race) { return race >= 0x83 && race <= 0x8b; }

uint32_t xorshift(uint32_t x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

// The general block rule (section 2.3): solid, or a passage the creature may not use.
bool blocked(const Block &b, bool passageApplies, bool pass) {
    if ((b[4] & 0xfd) != 0)
        return true;
    return (b[5] & BLOCK_PASSAGE) && passageApplies && (!pass || !(b[5] & BLOCK_PASSABLE_BY_BUFF));
}

// The cells a box covers, inclusive.
struct Box {
    int32_t lo[3], hi[3];
};
Box boxAt(const int64_t p[3], const float size[3]) {
    Box b;
    for (int i = 0; i < 3; i++) {
        const int64_t h = dots(size[i] * 0.5f);
        b.lo[i] = floorBlock(p[i] - h);
        b.hi[i] = floorBlock(p[i] + h);
    }
    return b;
}

// The shell scan of 0x2b77c0 (section 7.1): the first blocked block on the box's sides, floor or top.
bool shellBlocked(const std::function<Block(int x, int y, int z)> &block, const int64_t at[3], const float size[3], bool pass,
                  bool honour) {
    const Box b = boxAt(at, size);
    for (int x = b.lo[0]; x <= b.hi[0]; x++)
        for (int y = b.lo[1]; y <= b.hi[1]; y++) {
            const bool side = x == b.lo[0] || x == b.hi[0] || y == b.lo[1] || y == b.hi[1];
            for (int z = b.lo[2]; z <= b.hi[2]; z++)
                if ((side || z == b.lo[2] || z == b.hi[2]) && blocked(block ? block(x, y, z) : Block{}, honour, pass))
                    return true;
        }
    return false;
}

// Whether two boxes overlap on every axis, the way the game tests it (section 5.5 step 3.2):
// the far side touching counts on one end only.
bool overlaps(const int64_t a[3], const float aSize[3], const int64_t b[3], const float bSize[3]) {
    for (int i = 0; i < 3; i++) {
        const int64_t ha = dots(aSize[i] * 0.5f), hb = dots(bSize[i] * 0.5f);
        if (!(b[i] - hb <= ha + a[i] && a[i] - ha < hb + b[i]))
            return false;
    }
    return true;
}

// ---------------------------------------------------------------------------------------
// One physics step of one creature.

class Physics {
public:
    Physics(World &world, uint8_t *creature, float dt)
        : w(world), c(creature), dt(dt), dts(dt * 0.001f), before(get<uint32_t>(creature, CONTACT)) {}

    void run() {
        abilityStance();
        baseSpeed();
        abilitySpeeds();
        integrate();
        gravity();
        gliderChecks();
        leanIntoTurns();
        displacement();
        pushApart();
        if ((authoritative() || local()) && !(flags() & NO_UNSTICK) && !(state() & (NO_SIDE_COLLISION | HELD)) &&
            ability() != SLEEP)
            unstick();
        move();
        contactFlags();
        surfaces();
        climbOverTheTop();
        if (dashDust())
            return; // the game ends the step here (section 6.3)
        stride();
        stepOffset();
        turning();
    }

private:
    World &w;
    uint8_t *c;
    const float dt, dts;
    const uint32_t before; // the contact flags of the previous step

    float a[3] = {0, 0, 0}; // this step's acceleration
    float top = 0;          // the top speed, m
    bool noTurn = false;    // keeps the creature from turning toward its way (section 6.5)
    float d[3] = {0, 0, 0}; // one sub-step's move, blocks
    int64_t start[3] = {0, 0, 0};
    bool stepUpAllowed = true, onObject = false, wet = false;
    float heading[3] = {0, 0, 0};  // the wall probes' direction (section 6.1 step 12)
    float oldNormal[3] = {0, 0, 0};

    // --- Record access ---
    int ability() const { return c[ABILITY]; }
    void setAbility(int a) { c[ABILITY] = (uint8_t)a; }
    float t() const { return get<float>(c, ABILITY_TIME); }
    uint8_t hostility() const { return c[HOSTILITY]; }
    uint32_t flags() const { return get<uint32_t>(c, FLAGS); }
    uint16_t state() const { return get<uint16_t>(c, STATE); }
    void setState(uint16_t s) { put(c, STATE, s); }
    float f(size_t at) const { return get<float>(c, at); }
    void setF(size_t at, float v) { put(c, at, v); }
    float *vec(size_t at) { return reinterpret_cast<float *>(c + at); }
    int64_t *pos() { return reinterpret_cast<int64_t *>(c + POSITION); }
    int64_t id() const { return get<int64_t>(c, ID); }
    void sizes(float s[3]) const { std::memcpy(s, c + SIZE, 12); }
    float stun() const { return f(STUN); }
    float dodge() const { return f(DODGE); }

    bool local() const { return c == w.local; }
    bool authoritative() const { return !w.client; }
    bool owned() const { return local() || (authoritative() && hostility() != PLAYER); }

    // --- The world ---
    Block block(int x, int y, int z) const { return w.block ? w.block(x, y, z) : Block{}; }
    Block blockAt(int64_t x, int64_t y, int64_t z) const { return block(floorBlock(x), floorBlock(y), floorBlock(z)); }
    int rand() const { return w.rand ? w.rand() : 0; }
    uint8_t *byId(int64_t id) const { return w.byId ? w.byId(id) : nullptr; }

    const nativebuffs::Buff *findBuff(const uint8_t *who, int type) const {
        const std::vector<nativebuffs::Buff> *list = w.buffs ? w.buffs(who) : nullptr;
        if (list)
            for (const nativebuffs::Buff &b : *list)
                if (b[0] == type)
                    return &b;
        return nullptr;
    }
    bool hasBuff(int type) const { return findBuff(c, type) != nullptr; }
    nativerules::HasBuff hasBuffRule() const {
        return [this](int type) { return hasBuff(type); };
    }
    std::vector<nativerules::Buff> statBuffs() const {
        std::vector<nativerules::Buff> out;
        if (const std::vector<nativebuffs::Buff> *list = w.buffs ? w.buffs(c) : nullptr)
            for (const nativebuffs::Buff &b : *list)
                out.push_back({b[0], nativebuffs::strength(b)});
        return out;
    }
    float maxHealth() const {
        const std::vector<nativerules::Buff> b = statBuffs();
        return nativerules::maxHealth(c, b.data(), b.size());
    }
    void giveBuff(const nativebuffs::Buff &buff, bool refresh) {
        if (w.giveBuff)
            w.giveBuff(c, buff, refresh);
        w.buffNotes.push_back(nativebuffs::notice(id(), id(), buff, 0));
    }

    // The current ability's timing (abilityDuration and friends read +0x68 as it is now).
    float duration() const { return (float)nativerules::abilityDuration(c, hasBuffRule()); }
    int windUp() const { return nativerules::abilityEffectTime(c, -1, hasBuffRule()); }
    int middle() const { return nativerules::abilityMiddleTime(c, -1, hasBuffRule()); }

    // --- What the step tells the world ---
    void sound(int soundId, const int64_t at[3], float pitch, float volume, int64_t owner) {
        std::array<uint8_t, 0x30> s{};
        std::memcpy(&s[0], at, 24);
        put(s.data(), 0x18, (int32_t)soundId);
        put(s.data(), 0x1c, pitch);
        put(s.data(), 0x20, volume);
        put(s.data(), 0x28, owner);
        w.sounds.push_back(s);
    }
    void particles(const int64_t at[3], const float velocity[3], const float colour[4], float size, int count,
                   int style, float spread) {
        std::array<uint8_t, 0x48> p{};
        std::memcpy(&p[0], at, 24);
        std::memcpy(&p[0x18], velocity, 12);
        std::memcpy(&p[0x24], colour, 16);
        put(p.data(), 0x34, size);
        put(p.data(), 0x38, (int32_t)count);
        put(p.data(), 0x3c, (int32_t)style);
        put(p.data(), 0x40, spread);
        w.particles.push_back(p);
    }
    void damageNumber(float damage) {
        std::array<uint8_t, 0x48> n{};
        put(n.data(), 0x08, id());
        put(n.data(), 0x10, damage);
        std::memcpy(&n[0x20], pos(), 24);
        w.numbers.push_back(n);
    }

    // --- Section 3.1: the ability's stance (owned creatures only) ---
    static bool keepsOwnStance(int a) {
        switch (a) {
        case 0x5f: case 0x78: case 0x8a: case 0x8e: case 0x98: case 0x9f: case 0x0c: case 0x50: case 0x51:
        case 0x52: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: case 0x59: case 0x5a: case 0x76:
        case 0x79: case 0x7c: case 0x7d: case 0x80: case 0x81: case 0x92: case 0x9c: case 0x9d: case 0xa2:
        case 0xa3:
            return true;
        default:
            return false;
        }
    }
    static bool holdsStanceWhileCharging(int a) {
        switch (a) {
        case 0x5f: case 0x93: case 0x09: case 0x19: case 0x1a: case 0x1c: case 0x1d: case 0x25: case 0x3c:
        case 0x40: case 0x41:
            return true;
        default:
            return false;
        }
    }
    static bool turnsFreely(int a) {
        switch (a) {
        case 0x23: case 0x5f: case 0x60: case 0x61: case 0x62: case 0x65: case 0x75: case 0x85: case 0x87:
        case 0x88: case 0x89: case 0x8c: case 0x8f: case 0x90: case 0x95: case 0x9b:
            return true;
        default:
            return false;
        }
    }

    void abilityStance() {
        if (!owned())
            return;
        uint16_t s = state() & ~FACES_AIM;
        const int a = ability();
        if (a != 0 && !keepsOwnStance(a) && !(a == 0xa1 && hostility() == PLAYER) &&
            (t() < (float)(nativerules::abilityDuration(c, hasBuffRule()) + 200) || holdsStanceWhileCharging(a))) {
            if (!turnsFreely(a) && (s & NO_GRAVITY_STATES) == 0)
                s |= FACES_AIM;
            s &= ~SPRINT;
        }
        setState(s);
        if (a == STOPS_ON_INTENT && lengthSq3(vec(INTENT)) > 0)
            setAbility(0);
    }

    // --- Section 3.2: base speed, grip and lamp ---
    void baseSpeed() {
        float *intent = vec(INTENT);
        if (ability() == SAIL && (before & ON_GROUND))
            intent[0] = intent[1] = intent[2] = 0;
        for (int i = 0; i < 3; i++)
            a[i] = intent[i] * dts;
        if (stun() > 0)
            a[0] = a[1] = a[2] = 0;
        int ab = ability();
        if (!(state() & HELD) && ab != GLIDE && ab != CLIMB && (before & AT_SURFACE) && a[2] > 0)
            a[2] = 0; // at the water's surface it cannot swim up

        top = 6.0f;
        if (ab == RUN_DRAINING && f(STAMINA) > 0)
            top = 7.8f;
        if (ab == FAST_RUN && t() < duration())
            top = 9.0f;
        if (flags() & (FAST_40000 | FAST_200))
            top = 8.0f;
        if (ab == SLOW_WALK && t() < duration())
            top = 0.1f;

        if (owned())
            setGrip(ab);
        top = top * f(GRIP);

        if (local() && ab == RIDE) {
            if (before & IN_WATER) {
                setAbility(0);
            } else {
                const uint8_t *mount = byId(get<int64_t>(c, MOUNT));
                if (mount && get<float>(mount, HEALTH) > 0 && mountableRace(get<int32_t>(mount, RACE)))
                    top = top * 1.5f;
            }
        }

        if (ability() == GLIDE && !(before & ON_GROUND)) {
            // (The game also takes the glide factor of +0x1e0 here and drops it.)
            const float g = std::min(1.0f, std::max(0.0f, f(GLIDE_TIME) * 0.001f - 0.5f));
            top = top * (((1.0f - g) + (1.0f - g)) + g * 3.0f);
            // Pitch toward the way it flies, in double.
            const float *v = vec(VELOCITY);
            const double length = std::sqrt(((double)v[0] * v[0] + (double)v[1] * v[1]) + (double)v[2] * v[2]);
            const double q = 1.0 / length * v[2];
            const float target = (float)(nativecrt::asin(q) * DEGREES_PER_RADIAN);
            setF(PITCH, turn(f(PITCH), target, smooth(dt, 0.1f)));
        }
        if (ability() == SNEAK && (before & IN_WATER))
            setAbility(0);

        ab = ability();
        if ((state() & SPRINT) && ab != SNEAK && !(ab == CLIMB && (before & (ON_GROUND | ON_WALL))) &&
            !(ab == GLIDE && !(before & ON_GROUND)))
            top = ab == SWIM_SPRINT ? (float)((double)top * 1.5) : top + top;
        if (hasBuff(BUFF_DOUBLE_SPEED))
            top = top + top;
        if (hasBuff(BUFF_FIRE_TRAIL) || (ab == FIRE_RUN && f(CHARGE_LIMIT) > 0)) {
            top = top * 1.3f;
            fireTrail();
        }
    }

    // The ground grip (+0x194) and the lamp's radius, from the previous contact flags.
    void setGrip(int ab) {
        const bool fast = flags() & (FAST_200 | FAST_40000);
        float grip = 1.0f;
        if (ab == SAIL && (before & IN_WATER))
            grip = nativerules::skillValue(c, nativerules::Skill::Sailing);
        else if ((before & IN_WATER) && !fast)
            grip = nativerules::skillValue(c, nativerules::Skill::Swimming);
        else if (!fast && (before & ON_LAVA))
            grip = 0.35f;
        else if (!fast && (before & ON_SWAMP))
            grip = 0.25f;
        else if (!fast && (before & ON_SURFACE_10))
            grip = 0.98f;
        else if (before & ON_SURFACE_8)
            grip = 0.85f;
        if (f(STEP_OFFSET) > 0.1f)
            grip = grip * 0.8f;
        if (before & ON_SURFACE_13)
            grip = grip * 1.1f;
        setF(GRIP, grip);
        setF(LAMP_RADIUS, nativerules::skillValue(c, nativerules::Skill::LightRadius));
    }

    // Section 4.1: the fire trail leaves a burning area at the feet unless one of its own is near.
    void fireTrail() {
        if (!w.effects)
            return;
        const int64_t *p = pos();
        const float sz = f(SIZE + 8);
        for (const nativeeffects::Effect &e : *w.effects) {
            if (e[0x6c] != 5 || get<int64_t>(e.data(), 0) != id())
                continue;
            const float dx = blocks(get<int64_t>(e.data(), 0x08) - p[0]);
            const float dy = blocks(get<int64_t>(e.data(), 0x10) - p[1]);
            const float dz = blocks(get<int64_t>(e.data(), 0x18) - p[2] + dots(sz * 0.5f));
            if ((dx * dx + dy * dy) + dz * dz < 9.0f)
                return;
        }
        nativeeffects::Effect e{};
        put(e.data(), 0x00, id());
        put(e.data(), 0x08, p[0] + (int64_t)0.0f);
        put(e.data(), 0x10, p[1] + (int64_t)0.0f);
        put(e.data(), 0x18, p[2] + dots(0.01f - sz * 0.5f));
        const std::vector<nativerules::Buff> b = statBuffs();
        put(e.data(), 0x50,
            nativerules::abilityDamageFactor(ability()) * nativerules::attackPower(c, b.data(), b.size()));
        put(e.data(), 0x54, 5.0f);
        put(e.data(), 0x58, 1.0f);
        put(e.data(), 0x68, (int32_t)6);
        e[0x6c] = 5;
        put(e.data(), 0x74, (int32_t)5000);
        w.effects->push_back(e);
        w.sentEffects.push_back(e);
    }

    // --- Section 3.3: charges, dashes and fixed speeds ---
    static bool chargeStart(int a) {
        return (a >= 0x02 && a <= 0x08) || (a >= 0x0a && a <= 0x15) || a == 0x3b || a == 0x3f ||
               (a >= 0x42 && a <= 0x44);
    }
    static bool isCharging(int a) {
        switch (a) {
        case 0x09: case 0x19: case 0x1a: case 0x1c: case 0x3c: case 0x40: case 0x41: case STAMINA_CHARGE:
            return true;
        default:
            return false;
        }
    }
    static bool isLunge(int a) { return a == 0x45 || a == 0x46 || a == 0x67; }
    static bool isDash(int a) { return isLunge(a) || a == DASH_94; }

    void abilitySpeeds() {
        int ab = ability();
        if (ab == CHARGE_RUSH)
            top = 40.0f;
        if (ab == DIVE && t() < (float)(windUp() + middle()))
            top = 40.0f;
        if (ab == RUSH_END && t() < 500.0f)
            top = 40.0f;
        const bool charging = ab == CHARGE_RUSH || (chargeStart(ab) && t() < 200.0f);
        if (ab == TIMED_RUSH) {
            if (t() >= 500.0f)
                setAbility(0);
            else
                top = 40.0f;
        }
        if ((local() || authoritative()) && charging)
            contactOrPush();
        if (ability() == USE_ITEM) {
            if (t() >= (float)nativerules::useTime(c + ITEM_IN_USE))
                setAbility(0);
            else
                top = 0;
        }
        ab = ability();
        if (dodge() != 0 && ((ab != RUSH_END && ab != RUN_DRAINING && ab != 0x6c) || t() > duration())) {
            // A dodge cancels the ability (the class 4, specialisation 1 keeps three of them).
            if (c[CLASS] == 4 && c[SPECIALISATION] == 1) {
                if (ab != 0x06 && ab != 0x12 && ab != 0x15)
                    setAbility(0);
            } else {
                setAbility(0);
                setF(CHARGE, 0);
            }
        }

        noTurn = false;
        ab = ability();
        const float wind = (float)windUp();
        if (isLunge(ab) && t() <= wind)
            top = 0;
        if (isDash(ab) && wind < t() && t() < (float)(windUp() + (3 * middle()) / 4)) {
            const Basis b = basisOf(f(YAW));
            a[0] = (((b.right[0] * 0.0f) + b.forward[0]) + b.up[0] * 0.0f) * 20.0f;
            a[1] = (((b.right[1] * 0.0f) + b.forward[1]) + b.up[1] * 0.0f) * 20.0f;
            noTurn = true;
            top = 50.0f;
        }
        if (ab == 0x5f || (ab == FAST_RUN && t() < duration()))
            noTurn = true;
        if ((ab == 0x4e || ab == 0x4f || ab == 0x4d) && wind < t() && t() < (float)(windUp() + middle())) {
            // Side steps and a forward lunge.
            const Basis b = basisOf(f(YAW));
            for (int i = 0; i < 3; i++) {
                if (ab == 0x4e)
                    a[i] = (((b.forward[i] * 0.0f) - b.right[i] * 1.0f) + b.up[i] * 0.0f) * 20.0f;
                else if (ab == 0x4f)
                    a[i] = ((b.forward[i] * 0.0f + b.right[i]) + b.up[i] * 0.0f) * 20.0f;
                else
                    a[i] = ((b.right[i] * 0.0f + b.forward[i]) + b.up[i] * 0.0f) * 20.0f;
            }
            noTurn = true;
            top = 60.0f;
        }
        if (ab == 0x48 && t() < duration()) {
            top = 0;
            noTurn = true;
        }
        if (ab == SPIN && t() < duration()) {
            setF(YAW, dts * 720.0f + f(YAW));
            noTurn = true;
            top = 12.0f;
        }
        if (isDash(ab) && t() > wind)
            noTurn = noTurn || t() < duration();
        if (hostility() == FAST_HOSTILE)
            top = 30.0f;
        if ((ab == 0x3a || ab == 0x4b) && t() < duration() && t() >= wind) {
            noTurn = true;
            top = 0.1f;
        }
        if (ab == 0x25)
            setF(CHARGE, nativerules::attackSpeed(c, hasBuffRule()) * (dt * 0.0005f) + f(CHARGE));
        if (isCharging(ab) && t() >= wind)
            chargeUp(ab);
        if (f(CHARGE) > f(CHARGE_LIMIT))
            setF(CHARGE, f(CHARGE_LIMIT));

        if (f(SLOWED) != 0)
            top = top * 0.5f;
        if (hasBuff(BUFF_COLD))
            top = top * 0.5f;
        if (hasBuff(BUFF_SLOW))
            top = top * 0.1f;
        if (f(HASTED) != 0)
            top = top * 1.5f;
        backPedal();
        if (state() & HELD)
            top = 10.0f;
        if (ab == CLIMB) {
            top = top * 0.5f;
            float *k = vec(KNOCKBACK);
            k[0] = k[1] = k[2] = 0;
        }
        if (ab == GLIDE && !(before & ON_GROUND))
            setF(GLIDE_TIME, dt + f(GLIDE_TIME));
        else
            setF(GLIDE_TIME, 0);
    }

    // Section 3.3 step 18: a charging ability fills the charge (+0x158).
    void chargeUp(int ab) {
        float gain = nativerules::attackSpeed(c, hasBuffRule()) * (dt * 0.00075f);
        if (c[CLASS] == 2 && c[SPECIALISATION] == 0) {
            const float r = (float)get<int32_t>(c, COMBO) / (float)nativerules::comboThreshold(c);
            gain = gain * (std::min(r, 4.0f) + 1.0f);
        }
        if (hasBuff(BUFF_FRENZY))
            gain = gain * 10.0f;
        if (hasBuff(BUFF_FULL_CHARGE))
            gain = f(CHARGE_LIMIT);
        if (ab == STAMINA_CHARGE) {
            const float take = std::min(f(STAMINA), gain);
            setF(STAMINA, f(STAMINA) - take);
            float charge = take + f(CHARGE);
            if (charge > 1.0f)
                charge = 1.0f;
            setF(CHARGE, charge);
            if (charge > f(CHARGE_LIMIT))
                setF(CHARGE_LIMIT, charge);
        } else {
            setF(CHARGE, gain + f(CHARGE));
        }
    }

    // Section 3.3 step 21: walking away from the aim is slower.
    void backPedal() {
        if (!(state() & FACES_AIM) || !(dodge() == 0) || !(ability() != RUSH_END || t() > 1000.0f))
            return;
        float vx = f(VELOCITY), vy = f(VELOCITY + 4);
        float wx = f(AIM), wy = f(AIM + 4);
        if (wx * vx + wy * vy < 0) {
            const float qw = 1.0f / std::sqrt(wx * wx + wy * wy);
            wx = wx * qw, wy = wy * qw;
            const float qv = 1.0f / std::sqrt(vx * vx + vy * vy);
            vx = vx * qv, vy = vy * qv;
            top = top * ((vx * wx + vy * wy) * 0.5f + 1.0f);
        }
    }

    // Section 4.2: a charge ends on reaching an enemy (or after 500 ms); until then it pushes
    // the creature forward.
    void contactOrPush() {
        bool contact = false;
        const int64_t *p = pos();
        const float sx = f(SIZE);
        for (const uint8_t *o : w.creatures) {
            if (!(get<float>(o, HEALTH) > 0) || !nativerules::hostile(c, o))
                continue;
            const float reach = (sx + 1.0f) + get<float>(o, SIZE) * 0.7f;
            const int64_t dx = p[0] - get<int64_t>(o, POSITION), dy = p[1] - get<int64_t>(o, POSITION + 8);
            const int64_t dxx = (int64_t)((uint64_t)dx * (uint64_t)dx), dyy = (int64_t)((uint64_t)dy * (uint64_t)dy);
            if (dxx / 65536 + dyy / 65536 < (int64_t)(reach * reach * 65536.0f)) {
                // The game then tests a vertical gap it computes from a zero, which any
                // positive sizes pass: so near in the plane is contact.
                contact = true;
            }
        }
        if (contact || t() > 500.0f) {
            if (ability() == CHARGE_RUSH)
                endChargeRush();
            if (ability() == LEAP) { // never reached: a leap does not charge
                setF(RELEASED_CHARGE, 0);
                setAbility(DIVE);
            }
        }
        if (!contact) {
            const Basis b = basisOf(f(YAW));
            const float k = top + top;
            for (int i = 0; i < 3; i++)
                a[i] = ((a[i] + b.forward[i] * k) + b.right[i] * 0.0f) + b.up[i] * 0.0f;
        }
    }

    void endChargeRush() {
        setF(RELEASED_CHARGE, f(CHARGE_LIMIT));
        setF(CHARGE_LIMIT, 0);
        setAbility(0);
        setF(ABILITY_TIME, 0);
        c[CHARGE_ENDED] = 0;
        if (c[WEAPON] != 3)
            return;
        const int32_t subtype = get<int32_t>(c, WEAPON + 4);
        setAbility(subtype == 5 ? 0x06 : subtype == 3 ? 0x12 : 0x15);
        setF(ABILITY_TIME, (float)(windUp() + 1));
        if (hasBuff(BUFF_CHARGE_END)) {
            giveBuff(nativebuffs::make(BUFF_CHARGE_END, 0, 0, 0), false);
            c[CHARGE_ENDED] = 1;
        }
    }

    // --- Section 5.1: integrate and cap ---
    bool climbingWallBefore() const { return ability() == CLIMB && (before & ON_WALL); }

    void integrate() {
        float *v = vec(VELOCITY);
        for (int i = 0; i < 3; i++)
            v[i] = v[i] + a[i];
        if (climbingWallBefore()) {
            const float l = lengthSq3(v);
            if (l > top * top) {
                const float q = 1.0f / std::sqrt(l);
                for (int i = 0; i < 3; i++)
                    v[i] = (v[i] * q) * top;
            }
        } else {
            const float l = lengthSq2(v[0], v[1]);
            if (l > top * top) {
                const float q = 1.0f / std::sqrt(l);
                v[0] = v[0] * q * top;
                v[1] = v[1] * q * top;
            }
        }
    }

    // --- Section 5.2: gravity and the slow falls ---
    void gravity() {
        float *v = vec(VELOCITY);
        if (climbingWallBefore()) {
            const float k = smooth(dt, 0.0025f);
            v[0] = v[0] + k * (0.0f - v[0]);
            v[1] = k * (0.0f - v[1]) + v[1];
            v[2] = k * (0.0f - v[2]) + v[2];
            if (v[2] > -10.0f)
                setF(FALL_START, blocks(pos()[2]));
            return;
        }
        const int ab = ability();
        if ((state() & NO_GRAVITY_STATES) || ab == SLEEP || (state() & HELD) || ab == SPECIAL_9C || ab == SPECIAL_9D)
            return;
        const uint8_t *linked = byId(get<int64_t>(c, LINKED));
        const bool unstunned = stun() <= 0;
        float k;
        if (ab == FOLLOW_OWNER && linked && (get<uint32_t>(linked, FLAGS) & SLOW_FALL) && unstunned)
            k = dts * 0.5f;
        else if (((flags() & SLOW_FALL) && unstunned) || ab == SWIM_SPRINT || ((ab == 0x86 || ab == 0x87) && t() < duration()))
            k = dts * 0.1f;
        else if (ab == FAST_RUN && v[2] < 0 && t() < duration())
            k = dts * 0.5f;
        else if (ab == HOVER && unstunned)
            k = dts * 0.035f;
        else if (ab == GLIDE)
            k = (1.0f - std::min(1.0f, std::max(0.0f, f(GLIDE_TIME) * 0.003f - 0.5f)) * 0.75f) * dts;
        else {
            if (v[2] >= 0)
                setF(FALL_START, blocks(pos()[2]));
            v[2] = v[2] - dts * GRAVITY;
            return;
        }
        v[2] = v[2] - k * GRAVITY;
        setF(FALL_START, blocks(pos()[2]));
    }

    // --- Section 5.3: the local player's glider ---
    void gliderChecks() {
        if (local() && ability() == GLIDE) {
            float *v = vec(VELOCITY);
            if (before & IN_WATER) {
                setAbility(0);
            } else if (!(before & ON_GROUND)) {
                const float *n = vec(WALL_NORMAL);
                if (f(GLIDE_TIME) > 1000.0f && (before & ON_WALL) && lengthSq3(v) > 64.0f &&
                    (n[1] * v[1] + n[0] * v[0]) + n[2] * v[2] < -4.0f)
                    crash();
                const std::array<float, 3> wind = w.wind ? w.wind(pos()[0], pos()[1]) : windAt(pos()[0], pos()[1]);
                for (int i = 0; i < 3; i++)
                    v[i] = wind[i] * dts + v[i];
            } else if (std::fabs((double)f(LEAN)) > 40.0) {
                crash(); // landed banked too far
            }
        }
        if (stun() > 0)
            setF(CHARGE, 0);
    }

    void crash() {
        setF(STUN, (float)nativerules::stunDuration(c));
        setAbility(0);
        sound(SOUND_CRASH, pos(), 1.0f, 1.0f, 0);
    }

    // Whether it glides (0x5ee70), by the contact flags in the record now.
    bool gliding() const {
        return ability() == GLIDE && !(get<uint32_t>(c, CONTACT) & ON_GROUND) && stun() <= 0 && dodge() <= 0;
    }

    // --- Section 5.4: leaning into turns ---
    void leanIntoTurns() {
        const float ix = f(INTENT), iy = f(INTENT + 4);
        const float vx = f(VELOCITY), vy = f(VELOCITY + 4);
        float target = 0;
        const float intentSq = ix * ix + iy * iy, speedSq = vx * vx + vy * vy;
        if ((double)intentSq > 0.01 && speedSq > 0.01f) {
            const float qi = 1.0f / std::sqrt(intentSq), qv = 1.0f / std::sqrt(speedSq);
            const float dx = ix * qi, dy = iy * qi;
            const float ux = vx * qv, uy = vy * qv;
            float s = uy * dx - ux * dy;
            s = std::min(std::max(s, -1.0f), 1.0f);
            target = (float)((double)nativecrt::asinf(s) / PI * 180.0 * 0.5);
        }
        const float sx = f(SIZE);
        if (sx > 2.0f)
            target = target * (2.0f / sx);
        if (state() & FACES_AIM)
            target = 0;
        float rate = 0.005f;
        if (gliding()) {
            target = std::max(-90.0f, std::min(90.0f, target * 40.0f));
            rate = 0.002f;
        }
        setF(LEAN, smooth(dt, rate) * (target - f(LEAN)) + f(LEAN));
    }

    // --- Section 5.5: displacement and pushing apart ---
    float D[3] = {0, 0, 0}; // this step's whole move, blocks

    void displacement() {
        const float *v = vec(VELOCITY), *k = vec(KNOCKBACK);
        for (int i = 0; i < 3; i++)
            D[i] = dts * (v[i] + k[i]);
        if (climbingWallBefore()) {
            const float *n = vec(WALL_NORMAL);
            for (int i = 0; i < 3; i++)
                D[i] = D[i] - n[i] * (dts * 0.01f);
        }
    }

    bool pushedApartFrom(const uint8_t *o) const {
        const int ab = ability();
        const uint8_t oh = o[HOSTILITY], ch = hostility();
        const int32_t race = get<int32_t>(c, RACE), oRace = get<int32_t>(o, RACE);
        if ((ab == FOLLOW_OWNER || ab == CARRY) && get<int64_t>(c, LINKED) == get<int64_t>(o, ID))
            return false;
        if (o[ABILITY] == CARRIED && get<int64_t>(o, LINKED) == id())
            return false;
        if (o == c || (get<uint32_t>(o, FLAGS) & NOT_PUSHED) || !(get<float>(o, HEALTH) > 0))
            return false;
        const bool summon = oh == ORPHANED || oh == SUMMONED;
        if ((ch == SUMMONED || (ch == ORPHANED && race != WATER_BUBBLE)) && !summon)
            return false;
        if (ch == PLAYER && (oh == PLAYER || oh == HOSTILITY_3 || oh == PET))
            return false;
        if (o[ABILITY] == 0x77 || o[ABILITY] == CIRCLE || (get<uint16_t>(o, STATE) & HELD) ||
            (get<uint32_t>(o, FLAGS) & NO_PUSHING))
            return false;
        if (oRace == WATER_BUBBLE && race != WATER_BUBBLE)
            return false;
        return true;
    }

    void pushApart() {
        const int ab = ability();
        if ((flags() & NO_PUSHING) || (ab >= 0x57 && ab <= 0x59) || ab == 0x77 || ab == 0x82 || ab == 0x91 ||
            (state() & HELD) || ((ab == CIRCLE || ab == DASH_94) && t() < duration()))
            return;
        float size[3];
        sizes(size);
        const int64_t *p = pos();
        for (const uint8_t *o : w.creatures) {
            if (!pushedApartFrom(o))
                continue;
            float other[3];
            std::memcpy(other, o + SIZE, 12);
            if (o[HOSTILITY] != ORPHANED && o[HOSTILITY] != SUMMONED)
                other[0] = other[0] + 1.0f, other[1] = other[1] + 1.0f, other[2] = other[2] + 0.0f;
            const int64_t *op = reinterpret_cast<const int64_t *>(o + POSITION);
            if (!overlaps(p, size, op, other))
                continue;
            float ex = blocks(p[0] - op[0]), ey = blocks(p[1] - op[1]);
            const float ez = blocks(p[2] - op[2]);
            if ((ex * ex + ey * ey) + ez * ez < 0.0001f) {
                // Exactly on top of each other: a random way out.
                const float angle = (float)((double)rand() * (2.0 * PI) / 32767.0);
                ex = nativecrt::cosf(angle), ey = nativecrt::sinf(angle);
            }
            if ((ex * ex + ey * ey) + 0.0f * 0.0f > 0) {
                const float q = 1.0f / std::sqrt((ex * ex + ey * ey) + 0.0f * 0.0f);
                ex = ex * q, ey = ey * q;
                if (stun() > 0)
                    ex = ex * 0.1f, ey = ey * 0.1f;
                const float k = get<float>(o, SIZE) / size[0];
                const float target[3] = {D[0] + k * ex, D[1] + k * ey, D[2] + k * 0.0f};
                ease3(D, target, smooth(dt, 0.001f));
            }
        }
    }

    // --- Section 7.1: freeing a creature stuck in blocks (0x2da1e0) ---
    bool stuckAt(const int64_t at[3], const float size[3], bool pass, bool honour) const {
        return shellBlocked(w.block, at, size, pass, honour);
    }

    void unstick() {
        if (!(f(HEALTH) >= 0) || (state() & NO_STEP_STATES))
            return;
        float size[3];
        sizes(size);
        if (ability() == NARROW_57 || ability() == NARROW_58)
            size[0] = size[1] = 0.8f;
        int64_t *p = pos();
        const Box b = boxAt(p, size);
        if (w.loaded)
            for (int x = b.lo[0]; x <= b.hi[0]; x++)
                for (int y = b.lo[1]; y <= b.hi[1]; y++)
                    if (!w.loaded(x, y))
                        return;
        const bool pass = hasBuff(BUFF_PASS);
        const bool honour = !(before & IN_PASSAGE) && !(state() & NO_STEP_STATES);
        if (!stuckAt(p, size, pass, honour))
            return;
        static const int tries[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {2, 0}, {-2, 0}, {0, 2}, {0, -2}};
        for (const auto &o : tries) {
            const int64_t at[3] = {p[0] + o[0] * BLOCK, p[1] + o[1] * BLOCK, p[2]};
            if (!stuckAt(at, size, pass, honour)) {
                p[0] = at[0], p[1] = at[1];
                return;
            }
        }
        p[2] += BLOCK;
        setF(STEP_OFFSET, 0);
    }

    // --- Section 5.6: the move in sub-steps ---
    void move() {
        const int n = (int)(std::sqrt(lengthSq3(D)) * 1.5f + 1.0f);
        const float share = 1.0f / (float)n;
        for (int i = 0; i < 3; i++)
            d[i] = D[i] * share;
        const int ab = ability();
        stepUpAllowed = !(ab == NARROW_57 || ab == NARROW_58 || ab == CLIMB || (state() & NO_STEP_STATES));

        float size[3];
        sizes(size);
        int64_t *p = pos();
        // Objects it already overlaps never block it this step.
        std::vector<bool> ignored(w.objects.size());
        for (size_t i = 0; i < w.objects.size(); i++) {
            float os[3];
            std::memcpy(os, w.objects[i] + SIZE, 12);
            ignored[i] = overlaps(p, size, reinterpret_cast<const int64_t *>(w.objects[i] + POSITION), os);
        }
        std::copy(p, p + 3, start);
        const bool pass = hasBuff(BUFF_PASS);
        const bool honour = !(before & IN_PASSAGE);

        for (int step = 0; step < n; step++)
            for (int axis = 0; axis < 3; axis++) {
                int64_t pre[3];
                std::copy(p, p + 3, pre);
                p[axis] += dots(d[axis]);
                if (state() & HELD)
                    continue;
                float foot[3];
                sizes(foot);
                if (ability() == NARROW_57 && (state() & STATE_2000))
                    foot[0] = foot[1] = 0.8f;
                bool hit = stuckAt(p, foot, pass, honour);
                if (!hit || axis == 2)
                    if (hitsObject(axis, pre, foot, ignored, pass)) {
                        hit = true;
                        onObject = true;
                        stepUpAllowed = false;
                    }
                if (!hit)
                    continue;
                if (axis == 2)
                    land();
                else
                    sideHit(axis, foot, pass);
            }
    }

    bool hitsObject(int axis, const int64_t pre[3], const float foot[3], const std::vector<bool> &ignored,
                    bool pass) const {
        const int64_t *p = reinterpret_cast<const int64_t *>(c + POSITION);
        for (size_t i = 0; i < w.objects.size(); i++) {
            const uint8_t *o = w.objects[i];
            const int32_t type = get<int32_t>(o, RACE);
            if (type == OBJECT_PASSABLE && (pass || (flags() & SEES_THROUGH)))
                continue;
            float os[3];
            std::memcpy(os, o + SIZE, 12);
            int64_t op[3];
            std::memcpy(op, o + POSITION, 24);
            if (type == OBJECT_LADDER) {
                const float rise = get<float>(o, RISE);
                os[2] = os[2] + rise;
                op[2] = op[2] + dots(rise * 0.5f);
            }
            // Only what it moves toward.
            if (!((int64_t)((float)(pre[axis] - op[axis]) * d[axis]) < 0))
                continue;
            bool overlap = true;
            for (int k = 0; k < 3 && overlap; k++) {
                const int64_t lo = std::min(pre[k], p[k]), hi = std::max(pre[k], p[k]);
                const int64_t ho = dots(os[k] * 0.5f), hc = dots(foot[k] * 0.5f);
                overlap = op[k] - ho <= hc + hi && lo - hc < ho + op[k];
            }
            if (overlap && !ignored[i])
                return true;
        }
        return false;
    }

    // Landing (or hitting a ceiling) on the z axis.
    void land() {
        int64_t *p = pos();
        float *v = vec(VELOCITY);
        if ((authoritative() || local()) && v[2] < -2.0f) {
            const float pitch = randUnit(rand(), 0.2f) + 1.0f;
            sound(SOUND_STEP, p, pitch, 0.5f, hostility() == PLAYER ? id() : 0);
        }
        if (!(before & IN_WATER) && v[2] < -5.0f && local() && dots(f(FALL_START)) - p[2] > FALL_DAMAGE_HEIGHT)
            fallDamage();
        p[2] -= dots(d[2]);
        v[2] = 0;
        setF(KNOCKBACK + 8, 0);
        setF(FALL_START, blocks(p[2]));
    }

    void fallDamage() {
        const int64_t fall = dots(f(FALL_START)) - pos()[2];
        const int64_t q = (int64_t)((float)fall / 20.0f) - BLOCK;
        const float damage = blocks((int64_t)((float)(int64_t)((float)q * maxHealth()) * 0.5f));
        float health = f(HEALTH) - damage;
        setF(STUN, std::max(stun(), (float)nativerules::stunDuration(c)));
        sound(SOUND_CRASH, pos(), 1.0f, 1.0f, 0);
        if (health < 0)
            health = 0;
        setF(HEALTH, health);
        damageNumber(damage);
    }

    // Meeting a wall on x or y: slide along it, stay, or step up onto it.
    void sideHit(int axis, const float foot[3], bool pass) {
        if (state() & NO_SIDE_COLLISION)
            return;
        int64_t *p = pos();
        if (!stepUpAllowed) {
            p[axis] -= dots(d[axis]);
            return;
        }
        const int64_t raised[3] = {p[0], p[1], p[2] + STEP_UP_HEIGHT};
        const Box b = boxAt(raised, foot);
        for (int x = b.lo[0]; x <= b.hi[0]; x++)
            for (int y = b.lo[1]; y <= b.hi[1]; y++)
                for (int z = b.lo[2]; z <= b.hi[2]; z++)
                    if (blocked(block(x, y, z), true, pass)) {
                        p[axis] -= dots(d[axis]);
                        return;
                    }
        const int64_t hz = dots(foot[2] * 0.5f);
        const int64_t oldZ = p[2];
        p[2] = hz + (int64_t)floorBlock(p[2] - hz) * BLOCK + STEP_UP_HEIGHT;
        if (!(ability() == DASH_94 && t() <= duration())) {
            float offset = blocks(p[2] - (oldZ - dots(f(STEP_OFFSET))));
            if (offset > 1.0f)
                offset = 1.0f;
            setF(STEP_OFFSET, offset);
        }
    }

    // --- Section 6.1: the contact flags ---
    uint32_t now = 0; // the new contact flags

    void contactFlags() {
        std::memcpy(oldNormal, c + WALL_NORMAL, 12);
        now = 0;
        int64_t *p = pos();
        float *v = vec(VELOCITY);
        const float *intent = vec(INTENT);
        if (stun() <= 0 && lengthSq2(v[0], v[1]) > 16.0f && lengthSq3(intent) > 16.0f) {
            const float mx = blocks(start[0] - p[0]), my = blocks(start[1] - p[1]), mz = blocks(start[2] - p[2]);
            if ((mx * mx + my * my) + mz * mz < lengthSq3(d) * 0.01f)
                now |= STUCK;
        }
        float size[3];
        sizes(size);
        const Box b = boxAt(p, size);
        const int64_t hz = dots(size[2] * 0.5f);
        const bool pass = hasBuff(BUFF_PASS);

        int32_t waterTop = b.lo[2];
        for (int x = b.lo[0]; x <= b.hi[0]; x++)
            for (int y = b.lo[1]; y <= b.hi[1]; y++)
                for (int z = b.lo[2]; z <= b.hi[2]; z++) {
                    const Block k = block(x, y, z);
                    if (k[4] == WATER) {
                        now |= IN_WATER;
                        waterTop = std::max(waterTop, z + 1);
                    }
                    if (k[5] & BLOCK_PASSAGE)
                        now |= IN_PASSAGE;
                }
        if (now & IN_WATER) {
            if (!(flags() & NO_UNSTICK) && !sinks(get<int32_t>(c, RACE)))
                buoyancy(waterTop, size[2]);
            atSurface(b);
        } else {
            findGround(b, p[2] - hz - GROUND_PROBE, pass);
        }
        surfaceUnderFeet(b, floorBlock(p[2] - hz - BLOCK));
        if (ability() == SWIM_SPRINT)
            now &= ~(ON_LAVA | ON_SWAMP);
        if ((wet || (now & IN_WATER)) && hostility() == PET)
            wetPet();
        stepDown(b, hz);
        const float rate = lengthSq3(intent) > 0 ? 0.0025f : 0.005f;
        boatDrift(rate);
        friction(rate);
        wallProbes(b, pass);
        ladders();
        put(c, CONTACT, now);
    }

    // Buoyancy: the deeper in water, the stronger the lift.
    void buoyancy(int32_t waterTop, float sz) {
        float *v = vec(VELOCITY);
        const float depth = blocks((int64_t)waterTop * BLOCK - pos()[2]) / (sz * 0.5f);
        float g;
        if (depth <= 0)
            g = 0;
        else if (depth < 1.0f)
            g = depth + 1.0f;
        else
            g = 1.0f;
        const uint8_t h = hostility();
        if (ability() == SAIL || ((h == ORPHANED || h == SUMMONED) && (flags() & BUOYANT_EXTRA) == 0))
            g = depth * 1.5f + 1.0f;
        v[2] = (dts * GRAVITY) * g + v[2];
    }

    void atSurface(const Box &b) {
        for (int x = b.lo[0]; x <= b.hi[0]; x++)
            for (int y = b.lo[1]; y <= b.hi[1]; y++)
                for (int z = b.lo[2]; z <= b.hi[2]; z++)
                    if (block(x, y, z)[4] != WATER) {
                        now |= AT_SURFACE;
                        return;
                    }
    }

    void findGround(const Box &b, int64_t probe, bool pass) {
        const int32_t g = floorBlock(probe);
        for (int x = b.lo[0]; x <= b.hi[0]; x++) {
            for (int y = b.lo[1]; y <= b.hi[1]; y++) {
                const Block k = block(x, y, g);
                if (blocked(k, true, pass))
                    now |= ON_GROUND;
                if (k[4] == WET)
                    wet = true;
            }
            if (now & ON_GROUND)
                break;
        }
    }

    void surfaceUnderFeet(const Box &b, int32_t s) {
        for (int x = b.lo[0]; x <= b.hi[0]; x++)
            for (int y = b.lo[1]; y <= b.hi[1]; y++) {
                const uint8_t type = block(x, y, s)[4];
                if (type == LAVA)
                    now |= ON_LAVA;
                else if (type == SWAMP)
                    now |= ON_SWAMP;
                else if (type == 10 || type == 11)
                    now |= ON_SURFACE_10;
                else if (type == 8 || type == 12)
                    now |= ON_SURFACE_8;
                else if (type == 13)
                    now |= ON_SURFACE_13;
                else if (type != AIR) {
                    now &= ~SURFACES_CLEARED;
                    return;
                }
            }
    }

    void wetPet() {
        uint8_t *owner = byId(get<int64_t>(c, OWNER));
        if (!owner)
            return;
        const float wetness = get<float>(owner, WETNESS);
        if (wetness < 1.0f)
            put(owner, WETNESS, std::min(dts * 0.05f + wetness, 1.0f));
    }

    // Walking off a ledge of up to a block keeps it on the ground.
    void stepDown(const Box &b, int64_t hz) {
        const int ab = ability();
        if (onObject || !(before & ON_GROUND) || (ab == DASH_94 && t() <= duration()) || !stepUpAllowed ||
            !(lengthSq2(d[0], d[1]) > 0) || (now & (ON_GROUND | IN_WATER)) || d[2] > 0)
            return;
        int64_t *p = pos();
        const int32_t level = floorBlock(p[2] - hz - STEP_DOWN_PROBE);
        for (int x = b.lo[0]; x <= b.hi[0]; x++)
            for (int y = b.lo[1]; y <= b.hi[1]; y++)
                if ((block(x, y, level)[4] & 0xfd) != 0) {
                    now |= ON_GROUND;
                    const int64_t oldZ = p[2];
                    p[2] = hz + STEP_UP_HEIGHT + (int64_t)level * BLOCK;
                    setF(STEP_OFFSET, blocks(p[2] - (oldZ - dots(f(STEP_OFFSET)))));
                    return;
                }
    }

    void boatDrift(float rate) {
        if (ability() != SAIL || !(now & IN_WATER) || c[ITEM_IN_USE] != 0x18 || get<int32_t>(c, ITEM_IN_USE + 4) != 2)
            return;
        float *v = vec(VELOCITY);
        const float slow = smooth(dt, rate * 0.1f);
        ease(v[0], 0, slow);
        ease(v[1], 0, slow);
        ease(v[2], 0, smooth(dt, rate));
        float target[3];
        std::memcpy(target, c + INTENT, 12);
        const float l = lengthSq3(target);
        if (l > 144.0f) {
            const float q = 1.0f / std::sqrt(l);
            for (float &x : target)
                x = x * q * 12.0f;
        }
        const float factor = nativerules::glideFactor(get<int32_t>(c, GLIDE_LEVEL));
        for (float &x : target)
            x = x * factor;
        ease3(vec(KNOCKBACK), target, smooth(dt, 0.005f));
    }

    static bool keepsMomentum(int ab, float t, int wind, int middle) {
        if (ab == CHARGE_RUSH || ab == TIMED_RUSH || (ab == DIVE && t <= (float)(wind + middle)))
            return true;
        const bool swing = ab == 0x07 || ab == 0x08 || ab == 0x0b || (ab >= 0x12 && ab <= 0x15);
        return swing && t < (float)wind;
    }

    void friction(float rate) {
        float *k = vec(KNOCKBACK);
        const int ab = ability();
        if (f(FRICTION_HOLD) == 0) {
            if (now & ON_GROUND) {
                if (!keepsMomentum(ab, t(), windUp(), middle()))
                    easeToZero3(vec(VELOCITY), smooth(dt, rate));
                easeToZero3(k, smooth(dt, 0.005f));
            } else if (ab == GLIDE || ab == FOLLOW_OWNER) {
                easeToZero3(k, smooth(dt, 0.005f));
            }
        }
        if ((now & IN_WATER) && ab != SAIL)
            easeToZero3(k, smooth(dt, 0.005f));
        easeToZero3(k, smooth(dt, 0.00001f));
    }

    // Probes one block beyond each side for walls; the wall it faces most becomes the normal.
    void wallProbes(const Box &b, bool pass) {
        const float ix = f(INTENT), iy = f(INTENT + 4);
        heading[0] = heading[1] = heading[2] = 0;
        const float l = ix * ix + iy * iy;
        if (l > 0) {
            const float q = 1.0f / std::sqrt(l);
            heading[0] = ix * q, heading[1] = iy * q, heading[2] = q * 0.0f;
        }
        const float *h = heading;
        float normal[3] = {0, 0, 0};
        float best = -2.0f;
        float size[3];
        sizes(size);
        const int64_t *p = pos();
        struct Side {
            int axis, sign;
            float score, normal[3];
        };
        const Side sides[4] = {
            {1, -1, (h[0] * -0.0f - h[1]) + h[2] * -0.0f, {0, 1, 0}},
            {1, 1, (h[0] * -0.0f + h[1]) + h[2] * -0.0f, {0, -1, 0}},
            {0, -1, (h[1] * -0.0f - h[0]) + h[2] * -0.0f, {1, 0, 0}},
            {0, 1, (h[1] * -0.0f + h[0]) + h[2] * -0.0f, {-1, 0, 0}},
        };
        for (const Side &s : sides) {
            int64_t reach;
            if (hostility() == PLAYER)
                reach = dots(size[s.axis] * 0.5f) + 16384;
            else
                reach = (int64_t)(int32_t)(size[s.axis] * 0.5f + 0.5f) * BLOCK + HALF_BLOCK;
            const int32_t line = floorBlock(p[s.axis] + s.sign * reach);
            const int other = 1 - s.axis;
            bool found = false;
            for (int u = b.lo[other]; u <= b.hi[other]; u++)
                for (int z = b.lo[2]; z <= b.hi[2]; z++) {
                    const Block k = s.axis == 0 ? block(line, u, z) : block(u, line, z);
                    if (!blocked(k, true, pass))
                        continue;
                    found = true;
                    now |= ON_WALL;
                    if (z > b.lo[2])
                        now |= WALL_ABOVE_FEET;
                    if (k[4] == WALL_16)
                        now |= WALL_TYPE_16;
                    if (k[5] & BLOCK_FLAG_4)
                        now |= WALL_FLAG_4;
                }
            if (found && s.score > best) {
                best = s.score;
                std::copy(s.normal, s.normal + 3, normal);
            }
        }
        std::memcpy(c + WALL_NORMAL, normal, 12);
    }

    void ladders() {
        const int64_t *p = pos();
        const float sx = f(SIZE);
        float *normal = vec(WALL_NORMAL);
        for (const uint8_t *o : w.objects) {
            const float rise = get<float>(o, RISE);
            if (get<int32_t>(o, RACE) != OBJECT_LADDER || !(rise > 1.0f))
                continue;
            const float r = (get<float>(o, SIZE) * 0.5f + sx * 0.5f) + 0.1f;
            const int64_t *op = reinterpret_cast<const int64_t *>(o + POSITION);
            const float ex = blocks(p[0] - op[0]), ey = blocks(p[1] - op[1]);
            if (std::max(std::fabs(ey), std::fabs(ex)) < r && op[2] <= p[2] && p[2] < op[2] + dots(rise)) {
                now |= LADDER;
                const float q = 1.0f / std::sqrt(ey * ey + ex * ex);
                normal[0] = ex * q, normal[1] = ey * q;
            }
        }
    }

    // --- Section 6.2: water, lava, swamp and pitch ---
    void surfaces() {
        const uint8_t h = hostility();
        const bool keepsLevel = h == ORPHANED || h == SUMMONED || (flags() & STAYS_LEVEL);
        float *v = vec(VELOCITY);
        if (now & IN_WATER) {
            if (ability() == SAIL) {
                sailPitch();
            } else {
                easeToZero3(v, smooth(dt, 0.005f));
                if (!keepsLevel)
                    setF(PITCH, f(PITCH) - smooth(dt, 0.01f) * (f(PITCH) + 60.0f));
            }
            if (!(before & IN_WATER) && v[2] < -3.0f) {
                const float up[3] = {0, 0, 10.0f}, colour[4] = {0.2f, 0.7f, 1.0f, 1.0f};
                particles(pos(), up, colour, 0.4f, 15, STYLE_WATER, 3.0f);
                if (owned())
                    sound(SOUND_SPLASH, pos(), randUnit(rand(), 1.0f) + 0.9f, 1.0f, id());
            }
            freezeInWater();
        } else if (now & ON_LAVA) {
            if (!keepsLevel)
                setF(PITCH, f(PITCH) - smooth(dt, 0.01f) * (f(PITCH) + 20.0f));
            if (!(before & ON_LAVA) && v[2] < -3.0f) {
                const float up[3] = {0, 0, 10.0f}, colour[4] = {1.0f, 0.3f, 0.0f, 1.0f};
                particles(pos(), up, colour, 0.4f, 15, STYLE_LAVA, 3.0f);
                sound(SOUND_LAVA, pos(), randUnit(rand(), 1.0f) + 0.9f, 1.0f, 0);
            }
            if (local() && f(HEALTH) > 0 && !hasBuff(BUFF_LAVA_PROOF) &&
                (int32_t)(w.time - dt) / 200 != (int32_t)w.time / 200)
                burn();
        } else if (now & ON_SWAMP) {
            if (!keepsLevel)
                setF(PITCH, f(PITCH) - smooth(dt, 0.01f) * (f(PITCH) + 20.0f));
            if (!(before & ON_SWAMP) && v[2] < -3.0f) {
                const float up[3] = {0, 0, 10.0f}, colour[4] = {0.5f, 0.4f, 0.3f, 1.0f};
                particles(pos(), up, colour, 0.4f, 15, STYLE_SWAMP, 3.0f);
                sound(SOUND_SPLASH, pos(), randUnit(rand(), 1.0f) + 0.6f, 1.0f, 0);
            }
            if (local() && !hasBuff(BUFF_SWAMP_PROOF) && !hasBuff(BUFF_POISON))
                giveBuff(nativebuffs::make(BUFF_POISON, maxHealth() * 0.01f, 3000.0f, 0), true);
        } else if (!gliding() && dodge() == 0) {
            float pitch = f(PITCH);
            ease(pitch, 0, smooth(dt, 0.01f));
            setF(PITCH, pitch);
        }
    }

    // A boat pitches with the water ahead of and behind it.
    void sailPitch() {
        const int64_t *p = pos();
        const float beta = (float)((double)f(YAW) * PI / 180.0 + PI / 2.0);
        const int64_t ox = dots(nativecrt::cosf(beta) * 3.0f), oy = dots(nativecrt::sinf(beta) * 3.0f);
        auto type = [&](int64_t x, int64_t y, int64_t z) { return blockAt(x, y, z)[4]; };
        int score = 0;
        score += type(p[0] + ox, p[1] + oy, p[2]) == WATER;
        score -= type(p[0] + ox, p[1] + oy, p[2] - BLOCK) == AIR;
        score -= type(p[0] - ox, p[1] - oy, p[2]) == WATER;
        score += type(p[0] - ox, p[1] - oy, p[2] - BLOCK) == AIR;
        const float k = smooth(dt, 0.003f);
        float pitch = f(PITCH);
        if (score < 0)
            pitch = pitch - k * (pitch + 20.0f);
        else if (score == 0)
            ease(pitch, 0, k);
        else
            ease(pitch, 20.0f, k);
        setF(PITCH, pitch);
    }

    void freezeInWater() {
        if (!local() || hasBuff(BUFF_COLD_PROOF))
            return;
        const nativebuffs::Buff *cold = findBuff(c, BUFF_COLD);
        if (cold && nativebuffs::strength(*cold) > 9800.0f)
            return;
        if (!w.warmth)
            return; // (no world generator)
        const int64_t *p = pos();
        if (w.warmth((int)(p[0] / BLOCK), (int)(p[1] / BLOCK)) < 0.2f)
            giveBuff(nativebuffs::make(BUFF_COLD, 0.75f, 10000.0f, 0), true);
    }

    void burn() {
        const float damage = (float)((double)maxHealth() * 0.1);
        float health = f(HEALTH) - damage;
        sound(SOUND_LAVA, pos(), 1.0f, 1.0f, 0);
        if (health < 0)
            health = 0;
        setF(HEALTH, health);
        damageNumber(damage);
    }

    // --- Section 6.3: climbing over the top, and the dashes' dust ---
    void climbOverTheTop() {
        // The game pushes the climber back off the wall's normal as it tops it.
        const float ix = f(INTENT), iy = f(INTENT + 4);
        if (local() && ability() == CLIMB && (before & ON_WALL) && !(now & ON_WALL) && ix * ix + iy * iy > 0 &&
            (oldNormal[1] * heading[1] + oldNormal[0] * heading[0]) + oldNormal[2] * heading[2] <= 0) {
            float *v = vec(VELOCITY);
            for (int i = 0; i < 3; i++)
                v[i] = oldNormal[i] * -2.0f;
        }
    }

    // Returns true when the step ends here.
    bool dashDust() {
        const int ab = ability();
        if (ab != DASH_94 && ab != 0x46)
            return false;
        const int wind = windUp();
        if (!((float)wind <= t() && t() < (float)(wind + middle())))
            return false;
        if (ab == DASH_94) {
            setF(STRIDE_STRENGTH, 0);
            setF(STRIDE_PHASE, 0);
        }
        const int64_t *p = pos();
        const float sz = f(SIZE + 8);
        const Block under = blockAt(p[0], p[1], p[2] - dots(sz * 0.5f) - HALF_BLOCK);
        if (under[4] != AIR && (int32_t)t() / 50 != (int32_t)(dt + t()) / 50) {
            const int r1 = rand(), r2 = rand();
            const float *v = vec(VELOCITY);
            float u[3] = {v[0], v[1], v[2]};
            normalise3(u);
            const float k = sz * 0.5f + 2.0f;
            const int64_t at[3] = {p[0] + dots(u[0] * k), p[1] + dots(u[1] * k), p[2] + dots(0.5f - sz * 0.5f)};
            const float velocity[3] = {(randUnit(r2, 4.0f) - 2.0f) + v[0] * 0.5f,
                                       (randUnit(r1, 4.0f) - 2.0f) + v[1] * 0.5f, v[2] * 0.5f + 2.0f};
            float colour[4] = {0.2f, 0.7f, 1.0f, 1.0f};
            int style = STYLE_WATER;
            if (under[4] != WATER) {
                for (int i = 0; i < 3; i++)
                    colour[i] = (float)under[i] * (1.0f / 128.0f);
                style = STYLE_BLOCK_DUST;
            }
            particles(at, velocity, colour, 0.3f, 8, style, 3.0f);
            if (owned())
                sound(SOUND_DUST, p, randUnit(rand(), 0.2f) + 0.8f, 1.0f, id());
        }
        return true;
    }

    // --- Section 6.4: stride and footsteps ---
    bool climbingWall() const { return ability() == CLIMB && (now & ON_WALL); }

    // Water thrown up behind a boat or a swimmer.
    void splash(int64_t at[3]) {
        const float *v = vec(VELOCITY);
        const float up = randUnit(rand(), 3.0f) + 2.0f;
        const int count = (rand() & 3) + 5;
        const float velocity[3] = {v[0] * -0.25f, v[1] * -0.25f, up};
        const float colour[4] = {0.2f, 0.7f, 1.0f, 1.0f};
        particles(at, velocity, colour, 0.25f, count, STYLE_WATER, 3.0f);
        const int soundId = SOUND_WET_STEP + rand() % 3;
        sound(soundId, at, randUnit(rand(), 1.5f) + 0.6f, 0.75f, 0);
    }
    void splashBehind() {
        const int64_t *p = pos();
        const float *v = vec(VELOCITY);
        int64_t at[3] = {p[0] - dots(v[0] * 0.1f), p[1] - dots(v[1] * 0.1f), p[2]};
        splash(at);
    }

    void stride() {
        const int ab = ability();
        const float sx = f(SIZE);
        const float k0 = std::sqrt(0.8f / sx);
        const float *v = vec(VELOCITY);
        const float speedSq = lengthSq3(v);
        if (ab == SAIL && (now & AT_SURFACE) && speedSq > 16.0f && (int32_t)(t() - dt) / 100 != (int32_t)t() / 100)
            splashBehind(); // the wake
        const bool water = now & IN_WATER, ground = now & ON_GROUND;
        float phase = f(STRIDE_PHASE), strength = f(STRIDE_STRENGTH);
        const float old = phase;
        bool idle = false;
        if (ab == SWIM_SPRINT || ab == TIMED_RUSH || (ab == 0x7d && t() < 5000.0f) ||
            ((ab == 0x61 || ab == 0x86 || ab == 0x87 || ab == 0x90) && t() < duration())) {
            // no stride
        } else if (ab == SLEEP && t() > 2000.0f) {
            phase = dts * 10.0f + phase;
            strength = dts + strength;
        } else if (ground || (flags() & SLOW_FALL) || climbingWall() || water) {
            const float planar = std::sqrt(lengthSq2(v[0], v[1]));
            if (speedSq <= 0.5f || ab == SAIL || ab == SPECIAL_9C || ab == SPECIAL_9D) {
                idle = true;
            } else if (climbingWall()) {
                phase = (std::sqrt(speedSq) * dts * 6.0f) * k0 + phase;
                strength = dts * 0.01f + strength;
            } else if (!ground) {
                phase = k0 * dt * 0.002f * planar * 1.5f + old;
                strength = strength + dts;
                setF(STRIDE_PHASE, phase), setF(STRIDE_STRENGTH, strength);
                if (authoritative() && (now & AT_SURFACE) && crossedHalfTurn(old, phase) && (rand() & 3) != 0)
                    splashBehind();
            } else {
                const bool lunging = ab == CHARGE_RUSH || (ab == DIVE && t() < duration()) ||
                                     ((ab == 0x07 || ab == 0x08 || ab == 0x0b || (ab >= 0x12 && ab <= 0x15)) &&
                                      t() < (float)windUp());
                float inc;
                if (lunging) {
                    inc = k0 * dt * 0.0001f * planar * 1.5f;
                } else {
                    inc = k0 * dts * 1.5f * planar; // (the order the recordings show)
                    if (ab == SNEAK)
                        inc = inc + inc;
                }
                phase = phase + inc;
                strength = strength + dts;
                setF(STRIDE_PHASE, phase), setF(STRIDE_STRENGTH, strength);
                if (owned() && !(dodge() > 0) && !water && planar > 5.0f && crossedHalfTurn(old, phase))
                    footstep();
            }
        } else {
            idle = true;
        }
        if (idle && !ground && !water && !climbingWall()) {
            phase = 0;
            strength = k0 * dts + strength;
        }
        setF(STRIDE_PHASE, phase);
        setF(STRIDE_STRENGTH, strength);

        if (!ground && !water && !climbingWall())
            setF(AIR_TIME, dt + f(AIR_TIME));
        else
            setF(AIR_TIME, 0);
        strength = f(STRIDE_STRENGTH);
        if (strength > 1.0f)
            strength = 1.0f;
        ease(strength, 0, smooth(dt, 0.005f));
        setF(STRIDE_STRENGTH, strength);
        if (flags() & FIXED_STRIDE) {
            const uint32_t x = xorshift((uint32_t)id());
            setF(STRIDE_PHASE, (float)(x % 10000));
            setF(STRIDE_STRENGTH, 0.1f);
        }
    }

    static bool crossedHalfTurn(float old, float phase) {
        return (int32_t)((double)phase / PI) > (int32_t)((double)old / PI);
    }

    void footstep() {
        const int64_t *p = pos();
        const Block under = blockAt(p[0], p[1], p[2] - dots(f(SIZE + 8) * 0.5f) - HALF_BLOCK);
        int soundId;
        float pitch;
        if (under[4] == WET) {
            soundId = SOUND_WET_STEP + rand() % 3;
            pitch = 1.0f;
        } else {
            soundId = SOUND_STEP;
            pitch = randUnit(rand(), 0.2f) + 1.0f;
        }
        sound(soundId, p, pitch, 1.0f, hostility() == PLAYER ? id() : 0);
    }

    // Eases the step-smoothing offset toward its rest: raised over lava and swamp.
    void stepOffset() {
        const float target = (now & (ON_LAVA | ON_SWAMP)) ? 0.75f : 0.0f;
        float offset = f(STEP_OFFSET);
        if (ability() == DASH_94 && t() < duration())
            offset = target;
        else
            ease(offset, target, smooth(dt, offset < 0 ? 0.01f : 0.02f));
        setF(STEP_OFFSET, offset);
    }

    // --- Section 6.5: turning ---
    void turnYawToward(float target, float rate) {
        float yaw = f(YAW);
        ease(yaw, turn(yaw, target, 1.0f), smooth(dt, rate));
        setF(YAW, yaw);
    }

    void turning() {
        float tilt = f(TILT);
        if (climbingWall()) {
            ease(tilt, 45.0f, smooth(dt, 0.005f));
            setF(TILT, tilt);
            const float *n = vec(WALL_NORMAL);
            float into[3] = {-n[0], -n[1], -n[2]};
            normalise3(into);
            turnYawToward(headingOf(into[0], into[1]), 0.005f);
            return;
        }
        tilt = nativecrt::expf(dt * -0.01f) * tilt;
        setF(TILT, tilt);
        if ((state() & FACES_AIM) && stun() <= 0) {
            float wx = f(FACING), wy = f(FACING + 4);
            const float l = wx * wx + wy * wy;
            if (l > 0 && !noTurn) {
                const float q = 1.0f / std::sqrt(l);
                wx = wx * q, wy = wy * q;
                turnYawToward(headingOf(wx, wy), 0.1f);
            }
            float aim[3];
            std::memcpy(aim, c + FACING, 12);
            if (lengthSq3(aim) > 0) {
                normalise3(aim);
                const float target = degrees(nativecrt::asinf(aim[2]));
                ease(tilt, turn(tilt, target, 1.0f), smooth(dt, 0.005f));
                setF(TILT, tilt);
            }
            return;
        }
        const float *v = vec(VELOCITY), *intent = vec(INTENT);
        if (dodge() == 0 && lengthSq2(v[0], v[1]) > 0.02f &&
            (intent[1] * v[1] + intent[0] * v[0]) + intent[2] * v[2] > 0 && !noTurn) {
            const float q = 1.0f / std::sqrt(lengthSq2(v[0], v[1]));
            turnYawToward(headingOf(v[0] * q, v[1] * q), 0.1f);
        }
    }
};

} // namespace

void update(World &world, uint8_t *creature, float dt) { Physics(world, creature, dt).run(); }

bool boxBlocked(const World &world, const int64_t pos[3], const float size[3], bool pass, bool honour) {
    return shellBlocked(world.block, pos, size, pass, honour);
}

// Section 7.7: three simplex noises of the position, each on its own offset of the plane.
std::array<float, 3> windAt(int64_t x, int64_t y) {
    constexpr double DOT = 1.0 / 65536.0;
    const int64_t mx = (int64_t)((double)x * -0.02), my = (int64_t)((double)y * -0.02);
    const int64_t px = (int64_t)((double)x * 0.02), py = (int64_t)((double)y * 0.02);
    std::array<float, 3> wind;
    wind[2] = 2.0f * (worldgen::simplex((double)(0x108a0000 - mx) * DOT, (double)(0x14e10000 - my) * DOT) + 0.5f);
    wind[1] = worldgen::simplex((double)(0xd7f0000 - mx) * DOT, (double)(0x20f60000 - my) * DOT) * 8.0f;
    wind[0] = worldgen::simplex((double)px * DOT, (double)py * DOT) * 8.0f;
    return wind;
}

} // namespace nativeupdate
