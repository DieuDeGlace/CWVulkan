// The ability step (0x2c5250), written from docs/specs/creature-abilities.md (part 1: the
#include "GameAngles.h"
// switch, the common tail and the melee moves) and creature-abilities-ranged.md (part 2:
// every other branch). Section numbers in the comments refer to those two documents; the
// effect objects the branches create are laid out as effect-objects.md §3 describes.
//
// Arithmetic is single precision in the order the specs give (it decides the last bit of
// damage numbers and hit strengths); the few double-precision steps are marked.
#include "NativeAbility.h"

#include "GameMath.h"
#include "NativeBuffs.h"
#include "worldgen/Gear.h"
#include "worldgen/Looks.h"
#include "worldgen/Residents.h"
#include "NativeCrt.h"
#include "NativeInteract.h"
#include "NativeRules.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <limits>

namespace nativeability {
namespace {

using nativeai::Creature;
using nativeeffects::Effect;
using Pos = std::array<int64_t, 3>;    // a world position in 1/65536 block ("dots")
using Vec3 = std::array<float, 3>;
using Colour = std::array<float, 4>;
using Number = std::array<uint8_t, 0x48>;

// --- The creature record's fields (creature-abilities.md §1.1) -----------------------------
constexpr size_t ID = 0x08, POSITION = 0x10, YAW = 0x30, VELOCITY = 0x34, INTENT = 0x40, KNOCKBACK = 0x4c,
                 HOSTILITY = 0x60, RACE = 0x64, ABILITY = 0x68, TIME = 0x6c, COMBO = 0x70, SINCE_HIT = 0x74,
                 APPEARANCE = 0x78, FLAGS = 0x80, WIDTH = 0x84, DEPTH = 0x88, HEIGHT = 0x8c, BOOMERANG_SIZE = 0xb4,
                 STATE = 0x128, DODGE = 0x12c, STUN = 0x130, LEVEL = 0x144, CLASS = 0x14c, SPEC = 0x14d,
                 CHARGE = 0x158, AIM = 0x174, HEALTH = 0x180, MANA = 0x188, STEALTH = 0x18c, RISING = 0x19c,
                 OWNER = 0x1a0, TARGET = 0x1a8, FLIGHT_POINT = 0x1b0, PATH_START = 0x1b8, HOME_CELL = 0x1d0,
                 ITEM_IN_USE = 0x1d8, EQUIPMENT = 0x278, WEAPON_B = 0x6d8, ELEMENT = 0x970, STAMINA = 0x9a0,
                 MOUNT = 0x9d8, LAST_FOE = 0x9e0, SWING_COMBO = 0xad8, DESTINATION = 0xae0, DRAWN = 0xb10,
                 RELEASED = 0xba8, DEALT = 0xbac, FALL_START = 0xbb0, SURE_CRIT = 0xbb8, AI_FLAGS = 0xbcc,
                 SUMMON_MODE = 0xbe8;
constexpr size_t ITEM_SIZE = 0xa0, EQUIPMENT_SIZE = 11 * ITEM_SIZE, APPEARANCE_SIZE = 0xb0;
constexpr size_t ITEM_SUBTYPE = 4; // inside an item: type byte +0, subtype i32 +4

constexpr uint32_t FLAG_SUMMONED = 0x800;   // +0x80: a summon or a clone
constexpr uint32_t FLAG_CLEARED_ON_SUMMON = 0x200; // +0x80: a flag a summon never keeps (its meaning is not established)
constexpr uint16_t STATE_STRIKES_FROM_AIM = 0x4; // +0x128

// --- The effect record (effect-objects.md §0.1) --------------------------------------------
constexpr size_t E_OWNER = 0x00, E_POSITION = 0x08, E_TARGET = 0x20, E_VELOCITY = 0x44, E_DAMAGE = 0x50,
                 E_RADIUS = 0x54, E_SIZE = 0x58, E_CHARGE = 0x5c, E_EXTRA = 0x60, E_NO_CONTACT = 0x64, E_KIND = 0x68,
                 E_HIT_TYPE = 0x6c, E_AGE = 0x70, E_LIFETIME = 0x74, E_FREEZE = 0x78;

// --- A zone object (zone-object-interaction.md §0.2) ---------------------------------------
constexpr size_t O_TYPE = 0x00, O_POSITION = 0x08, O_STATE = 0x28;

// --- Numbers --------------------------------------------------------------------------------
constexpr float BLOCK = 1.0f / 65536.0f;       // dots to blocks (0x42e510)
constexpr float RAND_SCALE = 32767.0f;         // rand() divisor (0x42e5f0)
constexpr float DEG_TO_RAD = 0.0174532924f;    // 0x42e520
constexpr float COMBO_TIMEOUT = 4000.0f;       // ms without a landed hit (0x4725a4)
constexpr float SIGHT_LIMIT = 200.0f;          // blocks (0x42e5c0)
constexpr double PI = 3.141592653589793;
constexpr int64_t FIRST_MADE_ID = std::numeric_limits<int64_t>::min() + 1000; // ids of made creatures count down from here
constexpr int HUM_SOUND = 0x27;                // the charging and casting hum, every 200 ms
constexpr Colour UNSET_COLOUR = {0.0f, 0.0f, 0.0f, 0.0f}; // a colour the game leaves unwritten (§6, open question 2)

template <class T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <class T> void put(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }

bool oneOf(int a, std::initializer_list<int> ids) { return std::find(ids.begin(), ids.end(), a) != ids.end(); }

// dots(f) and blocks(i) (§0).
int64_t dots(float f) { return (int64_t)(f * 65536.0f); }
float blocks(int64_t i) { return (float)i * BLOCK; }
// A dot coordinate's block, toward zero (trunc-block) or toward minus infinity (floor-block).
int truncBlock(int64_t p) { return (int)((p < 0 ? p + 0xffff : p) >> 16); }
int floorBlock(int64_t p) { return (int)(p >> 16); }
int floorDiv(int v, int n) { return v >= 0 ? v / n : -((-v + n - 1) / n); }

Pos positionOf(const uint8_t *r) { return {get<int64_t>(r, POSITION), get<int64_t>(r, POSITION + 8), get<int64_t>(r, POSITION + 16)}; }
void setPosition(uint8_t *r, const Pos &p) {
    for (int i = 0; i < 3; i++)
        put<int64_t>(r, POSITION + 8 * i, p[i]);
}
Vec3 vecAt(const uint8_t *r, size_t at) { return {get<float>(r, at), get<float>(r, at + 4), get<float>(r, at + 8)}; }
void putVec(uint8_t *r, size_t at, const Vec3 &v) {
    for (int i = 0; i < 3; i++)
        put<float>(r, at + 4 * i, v[i]);
}
void putPos(uint8_t *r, size_t at, const Pos &p) {
    for (int i = 0; i < 3; i++)
        put<int64_t>(r, at + 8 * i, p[i]);
}
int64_t idOf(const uint8_t *r) { return get<int64_t>(r, ID); }

// A position moved by a vector in blocks, each component converted to dots.
Pos moved(const Pos &p, const Vec3 &v) { return {p[0] + dots(v[0]), p[1] + dots(v[1]), p[2] + dots(v[2])}; }
Vec3 scaled(const Vec3 &v, float k) { return {v[0] * k, v[1] * k, v[2] * k}; }

// (x² + y²) + z² (0x2b650).
float lengthSquared(const Vec3 &v) { return (v[0] * v[0] + v[1] * v[1]) + v[2] * v[2]; }
// One reciprocal, then three products (0x127f0, 0x179b0).
Vec3 normalised(const Vec3 &v) {
    const float q = 1.0f / std::sqrt(lengthSquared(v));
    return {v[0] * q, v[1] * q, v[2] * q};
}
// The squared distance in blocks of a − b, each axis converted before squaring.
float squaredBlocks(const Pos &a, const Pos &b) {
    const float x = blocks(a[0] - b[0]), y = blocks(a[1] - b[1]), z = blocks(a[2] - b[2]);
    return (x * x + y * y) + z * z;
}
// The flat distance test of the strikes (§0): the 64-bit squares divided by 65536, against
// the radius squared in dots. The products wrap as the game's 64-bit multiplies do.
bool withinFlat(const Pos &a, const Pos &b, float radius) {
    auto square = [](int64_t d) { return (int64_t)((uint64_t)d * (uint64_t)d) / 65536; };
    return square(a[0] - b[0]) + square(a[1] - b[1]) < (int64_t)((radius * radius) * 65536.0f);
}

// The yaw basis (0x13050, creature-physics.md §7.2): rows right, forward and up.
struct Basis {
    Vec3 right, forward, up;
};
Basis yawBasis(float yaw) {
    const float angle = yaw * DEG_TO_RAD;
    const float c = nativecrt::cosf(angle), s = nativecrt::sinf(angle);
    return {{c, s, 0.0f}, {-s, c, 0.0f}, {0.0f, 0.0f, 1.0f}};
}
// The basis applied to an offset (0xa5960), in the game's order of additions.
Vec3 rotated(const Basis &b, const Vec3 &o) {
    return {(o[1] * b.forward[0] + o[0] * b.right[0]) + o[2] * b.up[0],
            (o[0] * b.right[1] + o[1] * b.forward[1]) + o[2] * b.up[1],
            (o[0] * b.right[2] + o[1] * b.forward[2]) + o[2] * b.up[2]};
}

// The turn 0x17840 (creature-physics.md §7.3): angle `to` re-expressed nearest `from`, scaled.
float turn(float from, float to, float factor) {
    return gameangles::turn(from, to, factor);
}

// The saturating curve of a skill level (stat-rules.md §10.1).
float saturation(int level) { return 1.0f - 1.0f / (((float)level - 1.0f) / 50.0f + 1.0f); }

// The push of a melee move (0x5f180, §4.20), S being the attack-speed factor.
float pushFactor(const uint8_t *r, int a, float speed) {
    if (r[HOSTILITY] == 6 || r[HOSTILITY] == 3)
        return 0.0f;
    if (oneOf(a, {2, 3, 0x0a, 0x0e, 0x0f, 0x10, 0x3d, 0x3e}))
        return 6.0f / speed;
    if (oneOf(a, {4, 5, 7, 8, 0x13, 0x14, 0x3f}))
        return 3.0f / speed;
    if (a == 6)
        return 4.0f;
    if (a == 0x0c || a == 0x60)
        return 10.0f;
    if (a == 0x12 || a == 0x15)
        return 6.0f;
    if (a == 0x16 || a == 0x61)
        return 30.0f;
    if (a >= 0x1f && a <= 0x22)
        return 0.0f;
    if (a == 0x37)
        return 5.0f;
    if (a == 0x42 || a == 0x43)
        return 5.0f / speed;
    return 2.0f / speed;
}

// --- Which branch serves each ability (§2) -------------------------------------------------
enum class Branch {
    None,
    Melee,           // §4 (0x2c5cf0)
    Spell,           // part 2 §1 (0x2cf5fa)
    Arrow,           // §2 (0x2ceb05)
    Barrage,         // §3 (0x2cf1ea)
    CastArea,        // §4 (0x2c9ffc)
    Charging,        // §5 (0x2cad1d)
    AllyHeal,        // §6 (0x2cae0d)
    Ray,             // §7 (0x2cb299)
    Beam,            // §8 (0x2c97c1)
    UseItem,         // §9 (0x2c5337)
    ManaHeal,        // §10.1 (0x2c8ed0)
    HealPulses,      // §10.2 (0x2c9290)
    Focus,           // §11 (0x2c8b16)
    SelfSkill,       // §12 (0x2c9ea6, 0x2c9dfb, 0x2c9f51)
    RangerArea,      // §12 (0x2c8d3a)
    QueueInteraction, // §12 (0x2cab73, 0x2cac73, 0x2cabe3)
    Dash,            // §13.1 (0x2c5a1b)
    Blink,           // §13.2 (0x2ca321)
    Stealth,         // §13.3 (0x2cf174)
    ReturnHome,      // §13.4 (0x2c5886)
    Travel,          // §13.5 (0x2cd9bb)
    EagleFlight,     // §13.6 (0x2cdfd9)
    EagleRise,       // §13.7 (0x2cde09)
    Summon,          // §14.1 (0x2ca803)
    Tame,            // §14.2 (0x2ce833)
    PetWithOwner,    // §14.3 (0x2cca01)
    Throw,           // §14.4 (0x2ce6a7)
    CallUp,          // §14.5 (0x2ccb1a)
    SelfKill,        // §14.6 (0x2c59b9)
    ChargeSound,     // §15 ability 9 (0x2c5b29)
    Growl,           // §15 ability 0x4a (0x2cc920)
    RandomHum,       // §15 ability 0x53 (0x2cb17b)
    SpecialItem,     // §16 (0x2cce4c)
};

constexpr int LAST_ABILITY = 0xa0; // abilities 0, 1 and above 0xa0 go straight to the tail

const std::array<Branch, LAST_ABILITY + 1> &branchTable() {
    static const std::array<Branch, LAST_ABILITY + 1> table = [] {
        std::array<Branch, LAST_ABILITY + 1> t{};
        auto set = [&](Branch b, std::initializer_list<std::pair<int, int>> ranges) {
            for (const auto &[first, last] : ranges)
                for (int a = first; a <= last; a++)
                    t[a] = b;
        };
        set(Branch::Melee, {{2, 8}, {0x0a, 0x16}, {0x1f, 0x22}, {0x35, 0x35}, {0x37, 0x37}, {0x3a, 0x3b}, {0x3d, 0x3f},
                            {0x42, 0x47}, {0x49, 0x49}, {0x4b, 0x4f}, {0x5f, 0x61}, {0x64, 0x64}, {0x67, 0x67},
                            {0x74, 0x74}, {0x8b, 0x8b}, {0x94, 0x94}, {0x9e, 0x9f}});
        set(Branch::Spell, {{0x26, 0x30}, {0x7b, 0x7b}});
        set(Branch::Arrow, {{0x17, 0x18}, {0x1b, 0x1b}, {0x6c, 0x6c}, {0xa0, 0xa0}});
        set(Branch::Barrage, {{0x38, 0x38}});
        set(Branch::CastArea, {{0x62, 0x63}});
        set(Branch::Charging, {{0x19, 0x1a}, {0x1c, 0x1c}, {0x25, 0x25}, {0x3c, 0x3c}, {0x40, 0x41}, {0x93, 0x93}});
        set(Branch::AllyHeal, {{0x1d, 0x1d}});
        set(Branch::Ray, {{0x68, 0x6b}});
        set(Branch::Beam, {{0x88, 0x88}});
        set(Branch::UseItem, {{0x54, 0x55}, {0x82, 0x82}});
        set(Branch::ManaHeal, {{0x23, 0x23}});
        set(Branch::HealPulses, {{0x86, 0x87}});
        set(Branch::Focus, {{0x8c, 0x8c}});
        set(Branch::SelfSkill, {{0x70, 0x71}, {0x73, 0x73}});
        set(Branch::RangerArea, {{0x8e, 0x8e}});
        set(Branch::QueueInteraction, {{0x8f, 0x90}, {0x95, 0x95}});
        set(Branch::Dash, {{0x31, 0x32}, {0x52, 0x52}});
        set(Branch::Blink, {{0x33, 0x33}});
        set(Branch::Stealth, {{0x34, 0x34}});
        set(Branch::ReturnHome, {{0x75, 0x75}});
        set(Branch::Travel, {{0x98, 0x98}});
        set(Branch::EagleFlight, {{0x9c, 0x9c}});
        set(Branch::EagleRise, {{0x9d, 0x9d}});
        set(Branch::Summon, {{0x65, 0x66}});
        set(Branch::Tame, {{0x7e, 0x7e}});
        set(Branch::PetWithOwner, {{0x80, 0x80}});
        set(Branch::Throw, {{0x81, 0x81}});
        set(Branch::CallUp, {{0x9a, 0x9a}});
        set(Branch::SelfKill, {{0x85, 0x85}});
        set(Branch::ChargeSound, {{9, 9}});
        set(Branch::Growl, {{0x4a, 0x4a}});
        set(Branch::RandomHum, {{0x53, 0x53}});
        set(Branch::SpecialItem, {{0x89, 0x89}});
        return t;
    }();
    return table;
}

// One call of the step for one creature.
class Step {
  public:
    Step(World &world, Creature *creature, float dt)
        : w(world), c(creature), r(creature->record), dt(dt), authoritative(!world.client),
          local(world.local != nullptr && world.local == creature->record),
          deciding((authoritative && r[HOSTILITY] != 0) || local) {}

    void run() {
        const int a = r[ABILITY];
        if (a >= 2 && a <= LAST_ABILITY)
            runBranch(branchTable()[a]);
        tail();
    }

  private:
    World &w;
    Creature *c;
    uint8_t *r;
    const float dt;
    const bool authoritative; // world+0x358 is 0
    const bool local;         // C is the world's local player
    const bool deciding;      // the world that decides what C does (§0.1)
    nativeai::World sight;    // the blocks as sight and the block ray read them
    bool sightReady = false;

    // --- Time (§0.1) ----------------------------------------------------------------------
    float t() const { return get<float>(r, TIME); }
    float later() const { return dt + t(); }
    int ability() const { return r[ABILITY]; }
    bool passes(float x) const { return t() <= x && x < later(); }
    bool reaches(float x) const { return t() < x && x <= later(); }
    // Whether trunc32(from) / n and trunc32(from + dt) / n differ. A zero period, which
    // would fault in the game (open questions), never crosses.
    bool crosses(float from, int n) const { return n > 0 && (int)from / n != (int)(from + dt) / n; }
    bool boundary(int n) const { return n > 0 && (int)t() / n != (int)later() / n; }

    nativerules::HasBuff buffTest() const {
        return [this](int type) { return nativebuffs::find(*c, (uint8_t)type) != nullptr; };
    }
    bool hasBuff(int type) const { return nativebuffs::find(*c, (uint8_t)type) != nullptr; }
    int effectTime() const { return nativerules::abilityEffectTime(r, -1, buffTest()); }
    int middleTime() const { return nativerules::abilityMiddleTime(r, -1, buffTest()); }
    int duration() const { return nativerules::abilityDuration(r, buffTest()); }

    // --- Stats (stat-rules.md) -------------------------------------------------------------
    float damageFactor(int a) const { return nativerules::abilityDamageFactor(a); }
    float attackPower() const {
        const auto b = nativebuffs::forStats(*c);
        return nativerules::attackPower(r, b.data(), b.size());
    }
    float spellPower() const {
        const auto b = nativebuffs::forStats(*c);
        return nativerules::spellPower(r, b.data(), b.size());
    }
    float critChance(const Creature *target, bool magic) const {
        const auto b = nativebuffs::forStats(*target);
        return nativerules::critChance(r, buffTest(), target->record, b.data(), b.size(), magic);
    }
    float manaCost(int a) const { return nativerules::manaCost(a, buffTest()); }
    float staminaCost(int a) const { return nativerules::staminaCost(a, buffTest()); }
    float attackSpeed() const { return nativerules::attackSpeed(r, buffTest()); }
    float manaFactor() const {
        const auto b = nativebuffs::forStats(*c);
        return nativerules::manaFactor(b.data(), b.size());
    }
    static float maxHealthOf(const Creature *x) {
        const auto b = nativebuffs::forStats(*x);
        return nativerules::maxHealth(x->record, b.data(), b.size());
    }
    bool proc(float chance) { return nativerules::procHappens(r, chance, rand()); }

    float mana() const { return get<float>(r, MANA); }
    void setMana(float v) { put<float>(r, MANA, v); }
    void spendStamina(int a) {
        float stamina = get<float>(r, STAMINA) - staminaCost(a);
        if (0.0f > stamina)
            stamina = 0.0f;
        put<float>(r, STAMINA, stamina);
    }
    Vec3 aim() const { return vecAt(r, AIM); }
    Pos here() const { return positionOf(r); }
    int64_t myId() const { return idOf(r); }
    Creature *byId(int64_t id) const { return w.byId ? w.byId(id) : nullptr; }

    // --- rand() (§0) -------------------------------------------------------------------------
    int rand() { return w.rand ? w.rand() : 0; }
    float randScaled(float k, float plus) { return (float)rand() * k / RAND_SCALE + plus; }
    float variation() { return 1.25f - (float)rand() * 0.5f / RAND_SCALE; }

    // --- What the step appends to the world (§0.2) ------------------------------------------
    void sound(const Pos &p, int32_t id, float pitch, int64_t owner, float volume = 1.0f) {
        std::array<uint8_t, 0x30> s{};
        putPos(s.data(), 0, p);
        put<int32_t>(s.data(), 0x18, id);
        put<float>(s.data(), 0x1c, pitch);
        put<float>(s.data(), 0x20, volume);
        put<int64_t>(s.data(), 0x28, owner);
        w.sounds.push_back(s);
    }
    void hum(int64_t owner) { sound(here(), HUM_SOUND, randScaled(0.25f, 1.0f), owner); }
    void burst(const Pos &p, const Vec3 &velocity, const Colour &colour, float size, int32_t count, int32_t style,
               float spread) {
        std::array<uint8_t, 0x48> b{};
        putPos(b.data(), 0, p);
        putVec(b.data(), 0x18, velocity);
        for (int i = 0; i < 4; i++)
            put<float>(b.data(), 0x24 + 4 * i, colour[i]);
        put<float>(b.data(), 0x34, size);
        put<int32_t>(b.data(), 0x38, count);
        put<int32_t>(b.data(), 0x3c, style);
        put<float>(b.data(), 0x40, spread);
        w.particles.push_back(b);
    }
    static Number number(int64_t from, int64_t to, float amount, const Pos &where, bool critical = false,
                         uint8_t kind = 0) {
        Number n{};
        put<int64_t>(n.data(), 0x00, from);
        put<int64_t>(n.data(), 0x08, to);
        put<float>(n.data(), 0x10, amount);
        n[0x14] = critical ? 1 : 0;
        putPos(n.data(), 0x20, where);
        n[0x45] = kind;
        return n;
    }
    void notice(Creature *x, const nativebuffs::Buff &b, uint8_t flag) {
        const int64_t id = idOf(x->record);
        w.buffNotes.push_back(nativebuffs::notice(id, id, b, flag));
    }
    // Give-and-tell (0x29d2f0): the buff, then its notice with the refresh flag.
    void giveAndTell(Creature *x, const nativebuffs::Buff &b, bool refresh) {
        nativebuffs::give(*x, b, refresh);
        notice(x, b, refresh ? 1 : 0);
    }
    // Consuming a buff: an emptied buff of that type replaces C's, with its notice.
    void consumeBuff(uint8_t type) {
        if (!hasBuff(type))
            return;
        const nativebuffs::Buff b = nativebuffs::make(type, 0.0f, 0.0f, 0);
        nativebuffs::give(*c, b, false);
        notice(c, b, 0);
    }
    // A hit request (0x29eb60): kept in the world's list, then applied if the world can.
    bool strike(const Creature *target, float damage, bool critical, bool spirit, float stunChance, const Pos &where,
                const Vec3 &push, int32_t type, int32_t ability) {
        nativeupkeep::Hit h;
        h.target = idOf(target->record);
        h.attacker = myId();
        h.damage = damage;
        h.critical = critical ? 1 : 0;
        h.flag = spirit ? 1 : 0;
        h.charge = stunChance;
        std::copy(where.begin(), where.end(), h.where);
        std::copy(push.begin(), push.end(), h.push);
        h.type = type;
        h.ability = ability;
        h.extra = 0;
        h.last = 1; // blockable
        w.hits.push_back(h);
        return w.hit ? w.hit(h) : false;
    }
    void applyHeal(const Number &n) {
        if (w.heal)
            w.heal(n);
    }
    // An effect goes to the world's list (+0x60) and to the copies it sends (+0xd0).
    void launch(const Effect &e) {
        if (w.effects)
            w.effects->push_back(e);
    }
    void send(const Effect &e) {
        if (w.sentEffects)
            w.sentEffects->push_back(e);
    }
    void launchAndSend(const Effect &e) {
        launch(e);
        send(e);
    }

    // --- The world's blocks --------------------------------------------------------------------
    nativeai::World &sightWorld() {
        if (!sightReady) {
            sight.block = w.block;
            sight.zone = w.zone;
            sightReady = true;
        }
        return sight;
    }
    nativeai::Block blockAt(const Pos &p) const {
        if (w.blockAt)
            return w.blockAt(p[0], p[1], p[2]);
        return {0xc8, 0xc8, 0xc8, 0x00, 0x01, 0x00}; // the default block of an unloaded zone
    }
    // Breaking one block, if it is one strikes break (§4.4 step 2): debris, the removal and
    // the change record. Says whether it broke.
    bool breakBlock(const Pos &p) {
        const nativeai::Block b = blockAt(p);
        if (!(b[5] & 1))
            return false;
        const float share = 1.0f / 128.0f;
        burst(p, {0.0f, 0.0f, 10.0f}, {(float)b[0] * share, (float)b[1] * share, (float)b[2] * share, 1.0f}, 0.5f, 3,
              0, 3.0f);
        const int x = floorBlock(p[0]), y = floorBlock(p[1]), z = floorBlock(p[2]);
        if (w.removeBlock)
            w.removeBlock(x, y, z);
        std::array<uint8_t, 0x20> change{};
        put<int32_t>(change.data(), 0, x);
        put<int32_t>(change.data(), 4, y);
        put<int32_t>(change.data(), 8, z);
        std::memcpy(change.data() + 0x0c, b.data(), 6);
        const uint8_t empty[6] = {0xff, 0xff, 0xff, 0x00, 0x00, 0x00};
        std::memcpy(change.data() + 0x12, empty, 6);
        put<uint32_t>(change.data(), 0x18, w.changeCount);
        w.blockChanges.push_back(change);
        return true;
    }
    // After blocks broke: what they held up falls (0x2e0710) over a square around `centre`.
    void dropLoose(const Pos &centre, int64_t half) {
        if (w.rebuild)
            w.rebuild(truncBlock(centre[0] - half), truncBlock(centre[1] - half), truncBlock(centre[0] + half),
                      truncBlock(centre[1] + half));
    }

    // --- Shared pieces of several branches -------------------------------------------------------
    // Dropping the combo (§3.1): the ability keeps its share of its duration as its speed changes.
    void dropCombo() {
        const float before = attackSpeed();
        put<int32_t>(r, COMBO, 0);
        const float after = attackSpeed();
        put<float>(r, TIME, (before / after) * t());
    }
    void raiseCombo() {
        put<int32_t>(r, COMBO, get<int32_t>(r, COMBO) + 1);
        put<float>(r, SINCE_HIT, 0.0f);
    }
    // The evade reward (0x4ee50, creature-upkeep.md §4.10) of a dodging rogue.
    void rewardEvade(Creature *v) {
        uint8_t *vr = v->record;
        if (get<float>(vr, DODGE) == 0.0f || vr[CLASS] != 4 || vr[SPEC] != 1)
            return;
        float m = get<float>(vr, MANA) + 0.25f;
        if (m > 1.0f)
            m = 1.0f;
        put<float>(vr, MANA, m);
        const nativebuffs::Buff b = nativebuffs::make(0x0b, 0.0f, 30000.0f, 0);
        nativebuffs::give(*v, b, false);
        notice(v, b, 0);
    }
    // Crediting those who dodged this swing (§4.11 step 2): set B but not set A.
    void creditDodgers() {
        for (int64_t id : c->struck) {
            if (c->counted.count(id))
                continue;
            Creature *v = byId(id);
            if (!v)
                continue;
            rewardEvade(v);
            w.numbers.push_back(number(myId(), id, 0.0f, positionOf(v->record), false, 4));
        }
    }
    void forgetSwing() {
        c->counted.clear(); // set A
        c->struck.clear();  // set B
    }
    // The launch position of a shot: C's position moved by the shot's own velocity over the
    // frame (effect-objects.md §3.0 says "velocity"; the recordings show it is the shot's).
    Pos launchPosition(const Effect &e) const {
        return moved(here(), scaled(vecAt(e.data(), E_VELOCITY), dt * 0.001f));
    }
    // A healing area (kind 6, hit type 2) left by a fire mage's ray (effect-objects.md §3.6).
    void burningGround(const Pos &at) {
        Effect e = nativeeffects::made();
        put<int32_t>(e.data(), E_KIND, 6);
        e[E_NO_CONTACT] = 0;
        put<float>(e.data(), E_CHARGE, 0.0f);
        put<float>(e.data(), E_EXTRA, 0.0f);
        put<float>(e.data(), E_DAMAGE, spellPower() * 0.1f);
        put<float>(e.data(), E_RADIUS, 4.0f);
        e[E_HIT_TYPE] = 2;
        put<int64_t>(e.data(), E_OWNER, myId());
        putPos(e.data(), E_POSITION, at);
        put<float>(e.data(), E_SIZE, 1.0f);
        put<float>(e.data(), E_AGE, 0.0f);
        putVec(e.data(), E_VELOCITY, {0.0f, 0.0f, 0.0f});
        put<int32_t>(e.data(), E_LIFETIME, 5000);
        launchAndSend(e);
    }
    bool fireMage() const { return r[CLASS] == 3 && r[SPEC] == 1; }
    // C, or both C and X, are local: a heal applies where the world decides, or on the
    // local player healing itself.
    bool applies(const Creature *x) const { return authoritative || (local && w.local == x->record); }

    void runBranch(Branch b) {
        switch (b) {
        case Branch::None: break;
        case Branch::Melee: melee(); break;
        case Branch::Spell: spell(); break;
        case Branch::Arrow: arrow(); break;
        case Branch::Barrage: barrage(); break;
        case Branch::CastArea: castArea(); break;
        case Branch::Charging: charging(); break;
        case Branch::AllyHeal: allyHeal(); break;
        case Branch::Ray: ray(); break;
        case Branch::Beam: beam(); break;
        case Branch::UseItem: useItem(); break;
        case Branch::ManaHeal: manaHeal(); break;
        case Branch::HealPulses: healPulses(); break;
        case Branch::Focus: focus(); break;
        case Branch::SelfSkill: selfSkill(); break;
        case Branch::RangerArea: rangerArea(); break;
        case Branch::QueueInteraction: queueInteraction(); break;
        case Branch::Dash: dash(); break;
        case Branch::Blink: blink(); break;
        case Branch::Stealth: stealth(); break;
        case Branch::ReturnHome: returnHome(); break;
        case Branch::Travel: travel(); break;
        case Branch::EagleFlight: eagleFlight(); break;
        case Branch::EagleRise: eagleRise(); break;
        case Branch::Summon: summon(); break;
        case Branch::Tame: tame(); break;
        case Branch::PetWithOwner: petWithOwner(); break;
        case Branch::Throw: throwCreature(); break;
        case Branch::CallUp: callUp(); break;
        case Branch::SelfKill: selfKill(); break;
        case Branch::ChargeSound: chargeSound(); break;
        case Branch::Growl: growl(); break;
        case Branch::RandomHum: randomHum(); break;
        case Branch::SpecialItem: specialItem(); break;
        }
    }

    // --- The common tail (§3) ------------------------------------------------------------------
    void tail() {
        if (authoritative || local) {
            const float since = dt + get<float>(r, SINCE_HIT);
            put<float>(r, SINCE_HIT, since);
            if (since > COMBO_TIMEOUT)
                dropCombo();
            // (Each cooldown would be reset here if 0x368230 said so; it never does.)
        }
        for (auto &[skill, left] : c->cooldowns) {
            left = left - dt;
            if (0.0f > left)
                left = 0.0f;
        }
    }

    // --- The melee branch (§4) ------------------------------------------------------------------
    void melee();
    // Reach and the strike centre (§4.1).
    void meleeReach(int a, Pos &centre, float &reach) const;

    // --- Part 2 ---------------------------------------------------------------------------------
    void spell();
    void arrow();
    void barrage();
    void castArea();
    void charging();
    void allyHeal();
    void ray();
    void beam();
    void useItem();
    void manaHeal();
    void healPulses();
    void focus();
    void selfSkill();
    void rangerArea();
    void queueInteraction();
    void dash();
    void blink();
    void stealth();
    void returnHome();
    void travel();
    void eagleFlight();
    void eagleRise();
    void summon();
    void tame();
    void petWithOwner();
    void throwCreature();
    void callUp();
    void selfKill();
    void chargeSound();
    void growl();
    void randomHum();
    void specialItem();

    // The nine zones around C with their objects, for the special items (§16).
    template <class Visit> void nearZones(Visit &&visit) {
        if (!w.objects)
            return;
        const Pos me = here();
        const int zx = floorDiv(floorBlock(me[0]), 64), zy = floorDiv(floorBlock(me[1]), 64);
        for (int ox = -1; ox <= 1; ox++)
            for (int oy = -1; oy <= 1; oy++)
                if (std::vector<nativeai::Object> *objects = w.objects(zx + ox, zy + oy))
                    visit(*objects, zx + ox, zy + oy);
    }
};

// =============================================================================================
// Part 1 §4: the melee branch
// =============================================================================================

void Step::meleeReach(int a, Pos &centre, float &reach) const {
    const float width = get<float>(r, WIDTH);
    float ahead = 1.5f;
    reach = width * 1.5f;
    if (oneOf(a, {2, 3, 4, 5, 7, 8, 0x0a, 0x0e, 0x0f, 0x13, 0x14})) {
        ahead = 1.0f;
        reach = width + width;
    } else if (a == 0x15 || a == 0x16) {
        reach = reach * 2.25f;
    } else if (a == 0x5f || a == 0x9f) {
        ahead = 0.0f;
        reach = reach * 1.75f;
    } else if (a == 0x45 || a == 0x47) {
        ahead = 0.0f;
    } else if (a == 0x94 || a == 0x74) {
        ahead = 0.0f;
        reach = reach + reach;
    } else if (a == 0x60 || a == 0x61) {
        ahead = 0.0f;
        reach = 8.0f;
    } else if (a == 0x1f || a == 0x21) {
        ahead = 0.0f;
        reach = 3.0f;
    } else if (a == 0x20 || a == 0x22) {
        ahead = 0.0f;
        reach = 5.0f;
    } else if (a == 0x64 || a == 0x4b) {
        ahead = 0.0f;
        reach = 20.0f;
    } else if (oneOf(a, {0x8b, 0x49, 0x4d, 0x4e, 0x4f})) {
        ahead = 0.0f;
        reach = reach * 1.2f;
    }
    ahead = ahead * width;

    // The point `ahead` blocks in front of C, through its yaw basis.
    const Basis y = yawBasis(get<float>(r, YAW));
    const float ox = (y.forward[0] * ahead + y.right[0] * 0.0f) + y.up[0] * 0.0f;
    const float oy = (ahead * y.forward[1] + y.right[1] * 0.0f) + y.up[1] * 0.0f;
    const float oz = (y.forward[2] * ahead + y.right[2] * 0.0f) + y.up[2] * 0.0f;
    const Pos me = here();
    centre = moved(me, {ox, oy, oz});
    if (a >= 0x1f && a <= 0x22) // the close spells strike where C aims
        centre = moved(me, aim());
    if (a == 0x4c || a == 0x46)
        centre[2] = centre[2] - dots((get<float>(r, HEIGHT) - width) * 0.5f);
    if (oneOf(a, {0x0c, 0x37, 0x3e, 0x9e})) { // the spins strike all around
        centre = me;
        reach = reach * 3.0f;
    }
}

void Step::melee() {
    const int a = ability();
    const bool closeSpell = a >= 0x1f && a <= 0x22;
    Pos centre;
    float reach;
    meleeReach(a, centre, reach);

    // §4.2 The strike window, from the effect part L and the middle part L2.
    const int L = effectTime(), L2 = middleTime();
    float start = (float)((L2 / 2 - 100) + L), end = (float)(L2 / 2 + L);
    if ((int)start < L)
        start = (float)L;
    if (L + L2 <= (int)end)
        end = (float)(L - 1 + L2);
    if (a == 0x45 || a == 0x46)
        start = (float)(L2 / 3 + L);
    if (a == 0x67) {
        start = (float)(L2 / 5 + L);
        end = (float)(L + L2);
    }
    if (a == 0x94) {
        start = (float)L;
        end = (float)(L + L2);
    }

    // §4.3 Moves that strike again and again, a window every period.
    bool newSwing = false;
    if (oneOf(a, {6, 0x0c, 0x1f, 0x20, 0x21, 0x22, 0x49, 0x5f, 0x64, 0x8b, 0x9e, 0x9f})) {
        const float from = (float)L;
        if (!(t() >= from) || !(t() < (float)(L + L2))) {
            start = end = from;
        } else {
            int period = a == 0x64 ? 1000 : 250;
            if (oneOf(a, {0x0c, 0x9e, 6}))
                period = L2 / 3;
            else if (a == 0x1f || a == 0x21)
                period = L2 / 6;
            else if (a == 0x20 || a == 0x22)
                period = L2 / 12;
            if (period <= 0) // the game would divide by zero here (open question 1)
                period = 1;
            const int i0 = (int)(t() - from);
            start = (float)(L + (i0 - i0 % period));
            end = (float)((int)start + 100);
            const int i1 = (int)((dt + t()) - from);
            const float next = (float)(L + (i1 - i1 % period));
            if (next != start && (int)next < L + L2) {
                put<int32_t>(r, SWING_COMBO, get<int32_t>(r, COMBO));
                newSwing = true;
                forgetSwing();
            }
            if (end < later())
                newSwing = true;
            if (deciding && a != 0x8b && a != 6) {
                const float into = t() - from;
                const bool samePeriod = from <= t() - dt && (int)into / period == (int)(into + dt) / period;
                if (!samePeriod) {
                    Pos at = here();
                    if (get<uint16_t>(r, STATE) & STATE_STRIKES_FROM_AIM)
                        at = moved(at, aim());
                    sound(at, 0x0f, randScaled(0.25f, 1.0f), myId());
                    if (a == 0x64)
                        sound(here(), 0x55, randScaled(0.1f, 0.5f), myId());
                }
            }
        }
    }

    // §4.4 Breaking the blocks within reach of the centre.
    if (start < t() && t() <= end && authoritative && !oneOf(a, {0x4b, 0x60, 0x64, 0x8b})) {
        int radius = (int)reach;
        if (radius >= 9)
            radius = 8;
        else if (radius < 4)
            radius = 4;
        bool broke = false;
        for (int dx = -radius; dx <= radius; dx++)
            for (int dy = -radius; dy <= radius; dy++)
                for (int dz = -radius; dz <= radius; dz++) {
                    const Pos p = {centre[0] + (int64_t)dx * 65536, centre[1] + (int64_t)dy * 65536,
                                   centre[2] + (int64_t)dz * 65536};
                    if (squaredBlocks(p, centre) < (float)(radius * radius) && breakBlock(p))
                        broke = true;
                }
        if (broke) {
            sound(here(), 0x1a, randScaled(0.4f, 0.5f), 0);
            dropLoose(centre, (int64_t)(reach * 65536.0f));
        }
    }

    // §4.5 Another player's swing on a server is that player's to resolve.
    if (authoritative && r[HOSTILITY] == 0 && !local)
        return;

    // §4.6 The rogue's dodge on a charged strike, and the spins turning.
    if (r[CLASS] == 4 && r[SPEC] == 1 && oneOf(a, {6, 0x12, 0x15}) && t() == 0.0f)
        put<float>(r, DODGE, (mana() * 600.0f) * mana());
    if ((a == 0x5f || a == 0x9f) && t() < (float)duration()) {
        const float yaw = get<float>(r, YAW);
        put<float>(r, YAW, turn(yaw, ((dt * 0.00025f) * 360.0f) * 8.0f + yaw, 1.0f));
    }

    // §4.7 Charging up, casting, or lifting a two-handed weapon.
    const float T = (float)effectTime();
    if (a == 0x60 || a == 0x61) {
        if (t() < T && boundary(200))
            hum(0);
        if (t() < T && T <= later()) {
            sound(here(), 0x54, 1.0f, 0);
            burst(here(), {0.0f, 0.0f, 2.0f}, UNSET_COLOUR, 0.25f, 0x50, 1, 20.0f);
        }
    } else if (closeSpell) {
        if (t() < T && boundary(200))
            hum(0);
        if (t() < T && T <= later()) {
            const bool healer = r[SPEC] == 1;
            sound(here(), healer ? 0x2c : 0x29, 1.0f, 0);
            burst(moved(here(), aim()), {0.0f, 0.0f, 2.0f}, healer ? Colour{0.0f, 0.2f, 1.0f, 1.0f} : UNSET_COLOUR,
                  0.25f, 0x14, healer ? 2 : 1, 8.0f);
        }
    } else if (t() == 0.0f && nativerules::holdsTwoHanded(r) && a != 0x64) {
        sound(here(), 0x11, randScaled(0.1f, 1.0f), 0);
    }

    // §4.8 Ability 0x9e leaps.
    if (a == 0x9e && t() <= 100.0f && 100.0f < later())
        put<float>(r, VELOCITY + 8, get<float>(r, VELOCITY + 8) + 8.0f);

    // §4.9 The whoosh as the swing starts.
    if (deciding && t() <= (float)effectTime() && (float)effectTime() < later() && a != 0x64) {
        float pitch = randScaled(0.2f, 0.9f);
        int32_t id = 0x0f;
        if (a == 0x0b) {
            id = 0x10;
        } else if (a == 0x0c) {
            id = 0x33;
        } else if (a == 0x9e || a == 0x9f) {
            id = 0x33;
            pitch = pitch * 2.0f;
        } else if (a == 0x37) {
            id = 0x0e;
        } else {
            pitch = pitch * 1.5f;
            if (nativerules::holdsTwoHanded(r) || oneOf(a, {0x45, 0x46, 0x94, 0x67}))
                pitch = pitch * 0.5f;
        }
        sound(here(), id, pitch, myId());
    }

    // §4.10 A grunt half-way through some heavy swings.
    if (deciding && oneOf(a, {0x3a, 0x3b, 0x3d, 0x4b})) {
        const float mid = (float)(middleTime() / 2 + effectTime());
        if (t() < mid && mid <= later())
            sound(here(), 0x0c, randScaled(0.25f, 1.0f), myId());
    }

    // §4.11 Winding up: a fresh swing.
    if (t() <= (float)effectTime()) {
        put<int32_t>(r, SWING_COMBO, get<int32_t>(r, COMBO));
        creditDodgers();
        forgetSwing();
        r[DEALT] = 0;
    }

    // §4.12 Ability 0x4b roars.
    if (a == 0x4b) {
        const float mid = (float)(middleTime() / 2 + effectTime());
        if (t() < mid && mid <= later())
            sound(here(), 0x55, randScaled(0.1f, 0.5f), 0);
    }

    // §4.13 The end of the window.
    if (end <= t()) {
        creditDodgers();
        c->struck.clear();
    }

    // §4.14 A swing that dealt with no one loses the combo.
    if (r[DEALT] == 0 && (float)(effectTime() + middleTime()) <= later() && t() < (float)duration() &&
        get<float>(r, DODGE) == 0.0f)
        dropCombo();

    // §4.15 Stamina, as the swing starts.
    if (t() == 0.0f)
        spendStamina(a);

    // §4.16 Mana and the charge released into the swing.
    if (a == 0x20 || a == 0x22) {
        if (t() <= (float)effectTime() && (float)effectTime() < later()) {
            put<float>(r, RELEASED, 0.1f);
            float m = mana() - manaCost(a);
            if (0.0f > m)
                m = 0.0f;
            setMana(m);
        }
    } else if (t() == 0.0f) {
        r[SURE_CRIT] = 0;
        put<float>(r, RELEASED, 0.0f);
        if (manaCost(a) <= 0.0f) {
            if (oneOf(a, {6, 0x12, 0x15})) { // the whole mana goes into a strong strike
                put<float>(r, RELEASED, mana());
                setMana(0.0f);
                if (hasBuff(0x0b)) {
                    consumeBuff(0x0b);
                    r[SURE_CRIT] = 1;
                }
            } else if (oneOf(a, {0x37, 0x94, 0x16, 0x60, 0x61})) {
                put<float>(r, RELEASED, 1.0f);
            }
        } else {
            put<float>(r, RELEASED, manaCost(a));
            setMana(mana() - get<float>(r, RELEASED));
        }
    }
    const float charge = get<float>(r, CHARGE);
    if (charge > 0.0f) {
        put<float>(r, RELEASED, charge);
        setMana(mana() - charge);
        r[DEALT] = 0;
        if (a == 0x0c || a == 6)
            put<float>(r, RELEASED, get<float>(r, RELEASED) * 0.5f);
    }
    put<float>(r, CHARGE, 0.0f);
    if (mana() <= 0.0f && mana() != 0.0f)
        setMana(0.0f);
    const bool charged = get<float>(r, RELEASED) > 0.0f || oneOf(a, {0x0c, 0x5f, 0x9f});

    // §4.17 The hits, while the window is open.
    if (!(start < t() && t() <= end))
        return;
    const float bonus = oneOf(a, {0x0c, 0x3d, 0x3f}) ? 0.0f : 1.0f;
    bool landedAny = false;
    if (a == 0x20 && (float)effectTime() <= t() && hasBuff(9))
        consumeBuff(9);
    for (size_t i = 0; i < w.creatures.size(); i++) {
        Creature *x = w.creatures[i];
        uint8_t *xr = x->record;
        const int64_t xid = idOf(xr);
        if (!(0.0f < get<float>(xr, HEALTH)) || c->counted.count(xid))
            continue;
        const float rx = get<float>(xr, WIDTH) * 0.7f + reach;
        Pos eye = here();
        if (closeSpell)
            eye = {centre[0], centre[1], centre[2] + dots(reach)};
        const Pos xp = positionOf(xr);
        if (!withinFlat(centre, xp, rx))
            continue;
        if (!(std::fabs(blocks(centre[2] - xp[2])) < (get<float>(xr, HEIGHT) * 0.5f + get<float>(r, HEIGHT)) + reach))
            continue;
        if (!nativeai::canSee(sightWorld(), eye.data(), xp.data(), true, SIGHT_LIMIT))
            continue;

        if (nativerules::hostile(r, xr)) {
            if (get<float>(xr, DODGE) != 0.0f) { // it dodged: credited when the swing ends
                c->struck.insert(xid);
                continue;
            }
            const Pos me = here();
            Vec3 dir = {blocks(xp[0] - me[0]), blocks(xp[1] - me[1]), 0.0f};
            if ((dir[0] * dir[0] + dir[1] * dir[1]) + 0.0f > 0.01f)
                dir = normalised(dir);
            const int32_t hitType = (a == 0x1f || a == 0x20) ? 1 : (a == 0x21 || a == 0x22) ? 2 : 0;
            bool critical = r[SURE_CRIT] != 0;
            if (!critical) {
                const float roll = (float)rand() / RAND_SCALE;
                critical = roll < critChance(x, hitType != 0);
            }
            float k = pushFactor(r, a, attackSpeed());
            float damage;
            if (a == 0x60 || a == 0x61 || closeSpell) {
                const float m = damageFactor(a), p = spellPower();
                damage = variation() * (m * p);
            } else {
                const float m = damageFactor(a), wp = attackPower();
                const float released = get<float>(r, RELEASED);
                const float roll = (float)rand();
                damage = ((1.25f - roll * 0.5f / RAND_SCALE) + 0.0f) * ((((released * released) * 5.0f) + bonus) * (m * wp));
            }
            if (a == 0x16) { // the warrior's earth shatter, by its fixed skill level
                k = saturation(nativerules::skillLevel()) * 15.0f + 15.0f;
                damage = damage * nativerules::cloneAuraStrength(0x16);
            }
            if (critical)
                damage = damage + damage;
            float s = std::sqrt(get<float>(r, WIDTH) / get<float>(xr, WIDTH));
            s = s < 1.75f ? s : 1.75f;
            const Vec3 push = {dir[0] * s * k, dir[1] * s * k, (s * 0.25f) * k};
            const bool comboCounts = c->counted.empty() && 0.0f >= get<float>(r, STUN);
            const bool landed = strike(x, damage, critical, charged, get<float>(r, RELEASED), here(), push, hitType, a);
            if (landed) {
                if (a == 0x1f && proc(0.25f) && r[SPEC] == 0) { // the water mage's free cast
                    const nativebuffs::Buff b = nativebuffs::make(9, 0.0f, 30000.0f, 0);
                    nativebuffs::give(*c, b, false);
                    notice(c, b, 0);
                    sound(here(), 0x32, 1.0f, 0);
                }
                if (r[CLASS] == 1) { // a warrior's frenzy, likelier on longer swings
                    float q = (float)duration() * 0.01f;
                    if (!(1.0f > q))
                        q = 1.0f;
                    if (proc(q * 0.1f)) {
                        const nativebuffs::Buff b = nativebuffs::make(2, 0.0f, 8000.0f, 0);
                        nativebuffs::give(*c, b, false);
                        notice(c, b, 0);
                        sound(here(), 0x60, 1.0f, 0);
                    }
                }
                if (comboCounts)
                    raiseCombo();
                if (charged && r[CLASS] == 4 && r[SPEC] == 0) {
                    float sneak = get<float>(r, STEALTH) + get<float>(r, RELEASED);
                    if (1.0f < sneak)
                        sneak = 1.0f;
                    put<float>(r, STEALTH, sneak);
                }
                if ((authoritative || local) && !landedAny && !charged) { // mana from the first plain hit
                    const float roll = (float)rand();
                    float g = (1.0f - (roll + roll) / RAND_SCALE) * 0.05f + 0.1f;
                    if (a == 0x3b)
                        g = g + g;
                    g = g * ((float)duration() / 500.0f);
                    if (rand() % 8 == 0)
                        g = g * 1.5f;
                    float m = manaFactor() * g + mana();
                    if (1.0f < m)
                        m = 1.0f;
                    setMana(m);
                }
                landedAny = true;
            }
            c->counted.insert(xid);
            r[DEALT] = 1;
        }

        // A healer's close spell heals the friends it reaches, C too.
        if ((a == 0x21 || a == 0x22) && newSwing && !nativerules::hostile(r, xr) && xr[HOSTILITY] != 8 &&
            r[SPEC] == 1) {
            const float roll = (float)rand();
            const float amount = (spellPower() * (roll * 0.5f / RAND_SCALE - 1.25f)) * 0.1f;
            const bool critical = (float)rand() / RAND_SCALE < critChance(x, true);
            const Number n = number(myId(), xid, amount, positionOf(xr), critical);
            w.numbers.push_back(n);
            sound(here(), 0x2d, 1.0f, 0);
            if (applies(x))
                applyHeal(n);
        }
    }
    // (The game then asks for the duration once more and drops the answer.)
}

// =============================================================================================
// Part 2: the other branches
// =============================================================================================

// §1 Staff and wand shots, volleys and the meteor (effect-objects.md §3.3).
void Step::spell() {
    if (!deciding)
        return;
    const int a = ability();
    const int T = effectTime();
    if (t() <= (float)(T - 200) && boundary(200))
        hum(myId());

    // When the shot leaves: at T, or for the volleys at the start of each third of the middle.
    int shotAt = T;
    if (a >= 0x2e && a <= 0x30) {
        const int third = middleTime() / 3;
        if (third > 0) {
            const int e = (int)((dt + t()) - (float)T);
            shotAt = T + (e - e % third);
            if (shotAt < T)
                shotAt = T;
        }
    }
    const bool aiming = lengthSquared(aim()) > 0.0f;
    if (passes((float)T) && aiming)
        setMana(mana() - manaCost(a));
    if (!(t() < (float)(T + middleTime()) && passes((float)shotAt) && aiming))
        return;

    Effect e = nativeeffects::made();
    const Vec3 n = normalised(aim());
    const float speed = get<int32_t>(r, WEAPON_B + ITEM_SUBTYPE) == 12 ? 100.0f : 50.0f;
    putVec(e.data(), E_VELOCITY, scaled(n, speed));
    put<float>(e.data(), E_AGE, 0.0f);
    const float charge = get<float>(r, CHARGE);
    put<float>(e.data(), E_RADIUS, charge * 4.0f + 0.5f);
    Pos at = here();
    put<float>(e.data(), E_CHARGE, charge);
    put<float>(e.data(), E_SIZE, charge + 0.5f);
    if (get<int32_t>(r, RACE) == 0x65) {
        at[1] += dots(get<float>(r, DEPTH));
        at[2] += dots(get<float>(r, HEIGHT) * 0.5f);
    }
    putPos(e.data(), E_POSITION, at);
    put<int64_t>(e.data(), E_OWNER, myId());
    if (a == 0x7b) { // the meteor
        put<int32_t>(e.data(), E_KIND, 7);
        put<float>(e.data(), E_DAMAGE, (variation() * (damageFactor(a) * attackPower())) * 5.0f);
        put<float>(e.data(), E_SIZE, 4.0f);
        put<float>(e.data(), E_CHARGE, 1.0f);
        putVec(e.data(), E_VELOCITY, scaled(n, 100.0f));
    } else if (a == 0x2f) { // a shot that falls on the aimed point
        put<int32_t>(e.data(), E_KIND, 4);
        put<float>(e.data(), E_DAMAGE, (variation() * (damageFactor(a) * attackPower())) * 5.0f);
        put<float>(e.data(), E_SIZE, 3.0f);
        put<float>(e.data(), E_CHARGE, 0.5f);
        put<float>(e.data(), E_RADIUS, 5.0f);
        putVec(e.data(), E_VELOCITY, {0.0f, 0.0f, 50.0f});
        putPos(e.data(), E_TARGET, moved(here(), aim()));
    } else {
        put<int32_t>(e.data(), E_KIND, 3);
        put<float>(e.data(), E_DAMAGE, variation() * (damageFactor(a) * spellPower()));
        if (a == 0x2e || a == 0x30) {
            put<float>(e.data(), E_SIZE, get<float>(e.data(), E_SIZE) + 0.5f);
            put<float>(e.data(), E_CHARGE, 0.25f);
        } else if (a == 0x26 || a == 0x2c) {
            put<float>(e.data(), E_SIZE, get<float>(e.data(), E_SIZE) + 1.0f);
            put<float>(e.data(), E_CHARGE, 1.0f);
        }
    }
    if (r[CLASS] == 3 || r[CLASS] == 0x98)
        e[E_HIT_TYPE] = r[SPEC] == 1 ? 2 : 1;
    e[E_NO_CONTACT] = oneOf(a, {0x26, 0x2c, 0x2e, 0x30}) ? 1 : 0;

    launch(e);
    if (charge > 0.0f) {
        float m = mana() - charge;
        if (m < 0.0f)
            m = 0.0f;
        setMana(m);
        put<float>(r, CHARGE, 0.0f);
    }
    if (e[E_NO_CONTACT])
        consumeBuff(9);
    send(e);

    int32_t id;
    float pitch;
    if (e[E_HIT_TYPE] == 2) {
        id = 0x2c;
        pitch = oneOf(a, {0x2a, 0x2b, 0x2d}) ? 1.25f : 1.0f;
    } else {
        id = 0x29;
        pitch = (a >= 0x27 && a <= 0x29) ? 2.0f : 1.0f;
    }
    sound(here(), id, pitch, 0, 0.9f);
}

// §2 Arrows, the boomerang and the homing bolt (effect-objects.md §3.1).
void Step::arrow() {
    if (!deciding)
        return;
    const int a = ability();
    const int T = effectTime();
    if (t() == 0.0f)
        spendStamina(a);
    if (passes((float)T)) { // the charge is released into the shot
        if (manaCost(a) > 0.0f)
            setMana(mana() - manaCost(a));
        else
            setMana(mana() - get<float>(r, CHARGE));
        if (mana() < 0.0f)
            setMana(0.0f);
        put<float>(r, RELEASED, get<float>(r, CHARGE));
        put<float>(r, CHARGE, 0.0f);
    }
    const float released = get<float>(r, RELEASED);
    const bool returnThrow = a == 0x1b && released > 0.0f && passes((float)(T + 300));
    if (!passes((float)T) && !returnThrow)
        return;

    Effect e = nativeeffects::made();
    const Vec3 n = normalised(aim());
    putVec(e.data(), E_VELOCITY, scaled(n, 100.0f));
    put<float>(e.data(), E_AGE, 0.0f);
    put<float>(e.data(), E_RADIUS, 0.5f);
    float q = released;
    if (a == 0x1b) {
        q = released * 0.2f;
        put<float>(e.data(), E_RADIUS, (released + released) + 2.0f);
    }
    put<float>(e.data(), E_CHARGE, q);
    if (manaCost(a) > 0.0f) {
        q = manaCost(a);
        put<float>(e.data(), E_CHARGE, q);
    }
    put<float>(e.data(), E_EXTRA, q);
    e[E_NO_CONTACT] = (released > 0.0f || a == 0x18) ? 1 : 0;
    putPos(e.data(), E_POSITION, launchPosition(e));
    put<int64_t>(e.data(), E_OWNER, myId());
    if (e[E_NO_CONTACT])
        consumeBuff(0x0a);
    if (a == 0x6c) {
        put<int32_t>(e.data(), E_KIND, 2);
    } else if (a == 0xa0) { // a poison bolt that homes on the aimed point
        put<int32_t>(e.data(), E_KIND, 10);
        e[E_HIT_TYPE] = 6;
        put<float>(e.data(), E_SIZE, 1.0f);
        put<float>(e.data(), E_CHARGE, 0.0f);
        put<float>(e.data(), E_RADIUS, 1.0f);
        e[E_NO_CONTACT] = 1;
        putVec(e.data(), E_VELOCITY, {0.0f, 0.0f, 50.0f});
        putPos(e.data(), E_TARGET, moved(here(), aim()));
    } else if (a == 0x1b) {
        put<int32_t>(e.data(), E_KIND, 5);
        put<float>(e.data(), E_SIZE, get<float>(r, BOOMERANG_SIZE));
    } else {
        put<int32_t>(e.data(), E_KIND, r[CLASS] == 0x95 ? 1 : 0);
    }
    const float kk = a == 0x1b ? 1.0f : 5.0f;
    float base = 1.0f;
    if (get<float>(e.data(), E_CHARGE) > 0.0f)
        base = 0.1f;
    if (a == 0x18)
        base = 0.0f;
    const float mw = damageFactor(a) * attackPower();
    put<float>(e.data(), E_DAMAGE, (variation() + 0.0f) * (((released * released) * kk + base) * mw));
    launchAndSend(e);

    const int roll = rand();
    if (a == 0x17 || a == 0x18) {
        const float x = (float)roll * 0.5f / RAND_SCALE;
        if (e[E_NO_CONTACT])
            sound(here(), 0x16, x + 1.0f, myId());
        else
            sound(here(), 0x0f, x + 2.0f, myId());
    } else if (a == 0x1b || a == 0xa0) {
        sound(here(), 0x0f, (float)(((double)roll * 0.51) / 32767.0 + (double)0.7f), myId());
    } else {
        sound(here(), 0x16, (float)roll * 0.5f / RAND_SCALE + 1.0f, myId());
    }
}

// §3 The arrow barrage (effect-objects.md §3.2): an arrow every quarter of the middle part.
void Step::barrage() {
    if (!deciding)
        return;
    const int a = ability();
    const int T = effectTime(), M = middleTime();
    if (passes((float)T)) {
        put<float>(r, RELEASED, get<float>(r, CHARGE));
        setMana(mana() - get<float>(r, CHARGE));
        put<float>(r, CHARGE, 0.0f);
        consumeBuff(0x0a);
    }
    if (!((float)T <= t() && t() < (float)(T + M) && boundary(M / 4) && lengthSquared(aim()) > 0.0f))
        return;
    sound(here(), 0x16, randScaled(0.5f, 1.0f), myId());
    const float released = get<float>(r, RELEASED);
    Effect e = nativeeffects::made();
    putVec(e.data(), E_VELOCITY, scaled(normalised(aim()), 150.0f));
    put<float>(e.data(), E_AGE, 0.0f);
    put<float>(e.data(), E_RADIUS, 0.5f);
    put<float>(e.data(), E_CHARGE, released * 0.25f);
    put<float>(e.data(), E_EXTRA, 0.0f);
    e[E_NO_CONTACT] = 1;
    putPos(e.data(), E_POSITION, launchPosition(e));
    put<int64_t>(e.data(), E_OWNER, myId());
    put<int32_t>(e.data(), E_KIND, r[CLASS] == 0x95 ? 1 : 0);
    const float mw = damageFactor(a) * attackPower();
    put<float>(e.data(), E_DAMAGE, (variation() + 0.0f) * (((released * released) * 5.0f + 0.1f) * mw));
    launchAndSend(e);
}

// §4 The area casts (effect-objects.md §3.4). Every creature hums; only the deciding world casts.
void Step::castArea() {
    const int a = ability();
    const int T = effectTime();
    if (t() < (float)T && boundary(200))
        hum(0);
    if (!deciding || !passes((float)T))
        return;
    sound(here(), 0x29, randScaled(0.25f, 1.0f), 0);
    const float mp = damageFactor(a) * spellPower();
    Effect e = nativeeffects::made();
    put<int64_t>(e.data(), E_OWNER, myId());
    putPos(e.data(), E_POSITION, here());
    putVec(e.data(), E_VELOCITY, {0.0f, 0.0f, 0.0f});
    put<float>(e.data(), E_DAMAGE, mp + mp);
    put<float>(e.data(), E_RADIUS, 8.0f);
    put<float>(e.data(), E_SIZE, 1.0f);
    put<float>(e.data(), E_CHARGE, 0.0f);
    put<float>(e.data(), E_EXTRA, 0.0f);
    e[E_NO_CONTACT] = 0;
    put<int32_t>(e.data(), E_KIND, 6);
    e[E_HIT_TYPE] = a == 0x62 ? 1 : 2; // 0x63 is the healing area
    put<float>(e.data(), E_AGE, 0.0f);
    put<int32_t>(e.data(), E_LIFETIME, 5000);
    put<int32_t>(e.data(), E_FREEZE, 0);
    launchAndSend(e);
}

// §5 Charging moves hum higher as the charge grows (the charge is built elsewhere).
void Step::charging() {
    if (deciding && boundary(200))
        sound(here(), 0x3a, get<float>(r, CHARGE) * 0.5f + 1.0f, myId());
}

// §6 Healing an ally with a stream, a tenth of its health every 200 ms.
void Step::allyHeal() {
    if (!deciding)
        return;
    float charge = dt * 0.0002f + get<float>(r, CHARGE);
    if (1.0f < charge)
        charge = 1.0f;
    put<float>(r, CHARGE, charge);
    if (!boundary(200))
        return;
    Colour colour = {0.2f, 0.6f, 1.0f, 0.0f}; // the alpha is left unset by the game
    if (r[ELEMENT] == 4)
        colour = {1.0f, 0.4f, 0.3f, 0.0f};
    else if (r[ELEMENT] == 5)
        colour = {0.2f, 1.0f, 0.4f, 0.0f};
    burst(moved(here(), aim()), {0.0f, 0.0f, 10.0f}, colour, 0.15f, 6, 2, 3.0f);
    Creature *x = byId(get<int64_t>(r, TARGET));
    if (!x || !(0.0f < get<float>(x->record, HEALTH)))
        return;
    const Number n = number(myId(), idOf(x->record), maxHealthOf(x) * -0.1f, positionOf(x->record));
    w.numbers.push_back(n);
    if (applies(x)) { // the game appends the number a second time where it applies it
        w.numbers.push_back(n);
        applyHeal(n);
    }
}

// §7 The ray spells: a line from the shot point that breaks blocks and strikes creatures.
void Step::ray() {
    if (!deciding)
        return;
    const int a = ability();
    const bool held = a == 0x69 || a == 0x6a;
    const int T = effectTime();
    if (passes((float)T) && held) {
        setMana(mana() - manaCost(a));
        consumeBuff(9);
    }
    if (mana() <= 0.0f && mana() != 0.0f) {
        setMana(0.0f);
        return;
    }
    if (!((float)T <= later() && t() < (float)(T + middleTime())))
        return;

    if ((a == 0x68 || a == 0x6b) && passes((float)T))
        sound(here(), 0x2b, randScaled(0.1f, 1.0f), 0);
    else if (held && boundary(200))
        hum(0);

    bool fires = passes((float)T);
    if (!fires && held)
        fires = crosses(t() - (float)T, 200) && (float)T < later();
    if (!fires)
        return;

    // The strike.
    Vec3 n = aim();
    if (lengthSquared(n) > 0.0f)
        n = normalised(n);
    Pos origin;
    nativeai::shotPoint(r, origin.data());
    const Vec3 d = normalised(aim());
    const float length = nativeai::ray(sightWorld(), origin.data(), d.data(), 200.0f, true);
    forgetSwing();

    if (length < 200.0f) { // the blocks around the ray's end break
        const Pos endAt = moved(origin, scaled(normalised(aim()), length));
        bool broke = false;
        for (int ox = -4; ox <= 4; ox++)
            for (int oy = -4; oy <= 4; oy++)
                for (int oz = -4; oz <= 4; oz++) {
                    const Pos q = {endAt[0] + (int64_t)ox * 65536, endAt[1] + (int64_t)oy * 65536,
                                   endAt[2] + (int64_t)oz * 65536};
                    if (squaredBlocks(q, endAt) < 16.0f && breakBlock(q))
                        broke = true;
                }
        if (broke) {
            sound(endAt, 0x1a, randScaled(0.4f, 0.5f), 0);
            dropLoose(endAt, 0x40000);
        }
    }
    if (fireMage())
        burningGround(moved(origin, scaled(normalised(aim()), length)));

    if (length > 0.0f) {
        const Pos step = {dots(n[0]), dots(n[1]), dots(n[2])};
        Pos p = origin;
        bool struckAny = false;
        int32_t hitType = (a == 0x68 || a == 0x69) ? 1 : 2;
        if (r[SPEC] == 2)
            hitType = 7;
        int i = 0;
        do {
            for (size_t k = 0; k < w.creatures.size(); k++) {
                Creature *x = w.creatures[k];
                uint8_t *xr = x->record;
                const int64_t xid = idOf(xr);
                if (x == c || !(0.0f < get<float>(xr, HEALTH)) || c->counted.count(xid) || get<float>(xr, DODGE) != 0.0f)
                    continue;
                const Pos xp = positionOf(xr);
                if (!withinFlat(p, xp, get<float>(xr, WIDTH) * 0.5f + 1.0f))
                    continue;
                if (!(std::fabs(blocks(p[2] - xp[2])) < get<float>(xr, HEIGHT) * 0.5f + 1.0f))
                    continue;
                c->counted.insert(xid);
                const float roll = (float)rand();
                const bool critical = roll / RAND_SCALE < critChance(x, true);
                const float mp = damageFactor(a) * spellPower();
                float damage = (((float)rand() * 0.05f / RAND_SCALE) + 1.0f) * mp;
                if (critical)
                    damage = damage + damage;
                if (nativerules::hostile(r, xr)) {
                    const bool landed = strike(x, damage, critical, held, held ? 0.1f : 0.0f, p, {0.0f, 0.0f, 0.0f},
                                               hitType, 0);
                    if (landed && a == 0x68 && proc(0.25f)) {
                        const nativebuffs::Buff b = nativebuffs::make(9, 0.0f, 30000.0f, 0);
                        nativebuffs::give(*c, b, false);
                        notice(c, b, 0);
                        sound(here(), 0x32, 1.0f, 0);
                    }
                    if (fireMage() && passes((float)T))
                        burningGround(p);
                    struckAny = true;
                }
            }
            for (int axis = 0; axis < 3; axis++)
                p[axis] += step[axis];
            i++;
        } while ((float)i < length);
        if (struckAny)
            raiseCombo();
    }

    if (c->counted.empty()) { // touched no one: a puff where the ray ended
        burst(moved(here(), scaled(normalised(aim()), length)), {n[0] * -1.0f, n[1] * -1.0f, n[2] * -1.0f},
              {1.0f, 0.2f, 0.5f, 0.0f}, 0.1f, 5, 1, 3.0f);
        dropCombo();
    }
}

// §8 The channelled beam: pulses that strike enemies along a short ray, for mana.
void Step::beam() {
    const int a = ability();
    const float cost = manaCost(0x88);
    const float whole = (float)(effectTime() + middleTime());
    if (mana() < cost && t() < whole)
        put<float>(r, TIME, whole); // out of mana: the beam ends now
    if (!deciding || !(t() <= (float)(effectTime() + middleTime())))
        return;
    if (boundary(200))
        hum(0);
    const int period = (int)((float)(effectTime() + middleTime()) / 20.0f);
    if (!boundary(period) || !(mana() >= cost))
        return;
    setMana(mana() - cost);
    c->counted.clear();
    const float length = nativeai::ray(sightWorld(), here().data(), normalised(aim()).data(), 5.0f, true);
    for (int i = 0; i <= 5; i++) {
        const Vec3 d = normalised(aim());
        const float f = (length * (float)i) / 5.0f;
        const Pos p = moved(here(), scaled(d, f));
        const float radius = ((float)i / 5.0f) * 5.0f + 1.0f;
        for (size_t k = 0; k < w.creatures.size(); k++) {
            Creature *x = w.creatures[k];
            uint8_t *xr = x->record;
            if (!nativerules::hostile(r, xr) || xr[HOSTILITY] == 8 || c->counted.count(idOf(xr)) ||
                !(radius * radius > squaredBlocks(positionOf(xr), p)))
                continue;
            sound(here(), 0x2a, 1.0f, 0);
            const Vec3 push = scaled(normalised(aim()), 8.0f);
            const float roll = (float)rand();
            const float base = ((damageFactor(a) * spellPower()) * t()) / (float)(effectTime() + middleTime());
            const bool critical = roll / RAND_SCALE < critChance(x, true);
            const float damage = variation() * base; // not doubled when critical
            strike(x, damage, critical, true, 0.05f, p, push, 1, 0);
        }
    }
}

// §9 Using an item (a potion, food) and the healing skill 0x82.
void Step::useItem() {
    if (!deciding)
        return;
    const int a = ability();
    const uint8_t kind = r[ITEM_IN_USE];
    if (kind != 1 && a != 0x82) {
        if (t() != 0.0f)
            return;
        sound(here(), 0x2f, 1.0f, 0);
        if (kind != 0x1c)
            return;
        // A potion: its buff for ten minutes.
        uint8_t type = 0x16;
        float strength = 1.2f;
        switch (get<int32_t>(r, ITEM_IN_USE + ITEM_SUBTYPE)) {
        case 1: type = 0x17; break;
        case 2: type = 0x18; break;
        case 3: type = 0x19; break;
        case 4: type = 0x1a, strength = 0.0f; break;
        case 5: type = 0x1b, strength = 0.0f; break;
        case 6: type = 0x1c, strength = 0.0f; break;
        default: break;
        }
        if (get<int32_t>(r, ITEM_IN_USE + ITEM_SUBTYPE) == 5) { // 0x2d2b40: buff 0x1d ends now
            for (nativebuffs::Buff &b : c->buffs)
                if (b[0] == 0x1d) {
                    put<float>(b.data(), 8, 0.0f);
                    notice(c, b, 0);
                    break;
                }
        }
        const nativebuffs::Buff b = nativebuffs::make(type, strength, 600000.0f, 0);
        nativebuffs::give(*c, b, true);
        notice(c, b, 0);
        return;
    }

    // Food, or the healing skill: health back every 200 ms over the use time.
    int use = nativerules::useTime(r + ITEM_IN_USE);
    if (a == 0x82) {
        use = duration();
        if (reaches(50.0f)) {
            burst(here(), {0.0f, 0.0f, 5.0f}, {0.6f, 0.5f, 0.3f, 1.0f}, 0.4f, 0x0f, 0, 3.0f);
            sound(here(), 0x30, randScaled(0.1f, 0.8f), 0);
        }
    }
    if (!(t() < (float)use && boundary(200)))
        return;
    float share = 200.0f / (float)use;
    if (r[HOSTILITY] != 0)
        share = share * 0.5f;
    const float heal = maxHealthOf(c) * share;
    if (heal > 0.0f) {
        w.numbers.push_back(number(-1, myId(), -heal, here()));
        float health = heal + get<float>(r, HEALTH);
        if (health > maxHealthOf(c))
            health = maxHealthOf(c);
        put<float>(r, HEALTH, health);
    }
    if (boundary(400))
        sound(here(), 0x2f, 1.0f, 0);
}

// §10.1 Healing the target (or C) from mana, eight times over the middle part.
void Step::manaHeal() {
    const int T = effectTime(), M = middleTime();
    if (t() < (float)T && boundary(200))
        hum(0);
    if (!deciding || !((float)T < t() && t() <= (float)(T + M)))
        return;
    const int period = (int)((float)M * 0.125f);
    if (period <= 0 || (int)((dt + t()) - (float)T) / period == (int)(t() - (float)T) / period)
        return;
    const float cost = manaCost(0x23);
    if (!(mana() >= cost))
        return;
    setMana(mana() - cost);
    Creature *x = byId(get<int64_t>(r, TARGET));
    if (!x)
        x = c;
    const float roll = (float)rand();
    const float amount = (spellPower() * -2.0f) * (1.25f - roll * 0.5f / RAND_SCALE);
    const Number n = number(myId(), idOf(x->record), amount, positionOf(x->record));
    w.numbers.push_back(n);
    sound(here(), 0x2c, 1.0f, 0);
    if (applies(x))
        applyHeal(n);
}

// §10.2 Healing pulses on the friends around C (0x87) or around where it aims (0x86).
void Step::healPulses() {
    const int a = ability();
    const float cost = manaCost(a);
    const float whole = (float)(effectTime() + middleTime());
    if (cost > mana() && t() < whole)
        put<float>(r, TIME, whole);
    if (!deciding || !(t() <= (float)(effectTime() + middleTime())))
        return;
    if (boundary(200))
        hum(0);
    if (!boundary((int)((float)(effectTime() + middleTime()) * 0.0625f)) || !(mana() >= cost))
        return;
    setMana(mana() - cost);
    Pos centre = here();
    if (a == 0x86)
        centre = moved(centre, aim());
    for (size_t k = 0; k < w.creatures.size(); k++) {
        Creature *x = w.creatures[k];
        uint8_t *xr = x->record;
        if (nativerules::hostile(r, xr) || xr[HOSTILITY] == 8 || !(25.0f > squaredBlocks(positionOf(xr), centre)))
            continue;
        const float roll = (float)rand();
        const float amount = (spellPower() * -0.5f) * (1.25f - roll * 0.5f / RAND_SCALE);
        const Number n = number(myId(), idOf(xr), amount, positionOf(xr));
        w.numbers.push_back(n);
        sound(here(), 0x2c, 1.0f, 0);
        if (applies(x))
            applyHeal(n);
    }
}

// §11 Repeated strikes on the focused target, sixteen over the middle part.
void Step::focus() {
    const int a = ability();
    const int T = effectTime(), M = middleTime();
    if (!deciding || !((float)T < t() && t() <= (float)(T + M)))
        return;
    const int period = (int)((float)M * 0.0625f);
    if (period <= 0 || (int)((dt + t()) - (float)T) / period == (int)(t() - (float)T) / period)
        return;
    if (!(mana() >= manaCost(a)))
        return;
    Creature *target = byId(get<int64_t>(r, TARGET));
    if (!target)
        return;
    const float mw = damageFactor(a) * attackPower();
    float damage = variation() * mw;
    const bool critical = (float)rand() / RAND_SCALE < critChance(target, false);
    if (critical)
        damage = damage + damage;
    if (strike(target, damage, critical, true, 0.0f, here(), {0.0f, 0.0f, 0.0f}, 0, 0x8c))
        raiseCombo();
}

// §12 The self-skills 0x70, 0x71 and 0x73: their start sound (their buffs come with the press).
void Step::selfSkill() {
    if (!deciding || t() != 0.0f)
        return;
    const int a = ability();
    const int32_t key = a == 0x70 ? 0x0c : a == 0x71 ? 1 : 6;
    // 0x29d3c0 with a key it gives no buff for: only the key's cooldown is set, and the
    // cooldown table (0x50bd0) gives 0 for every id below 0x16.
    c->cooldowns[key] = 0.0f;
    sound(here(), a == 0x70 ? 0x32 : a == 0x71 ? 0x60 : 0x5f, 1.0f, 0);
}

// §12 The ranger's area (effect-objects.md §3.5): enemies in it are slowed.
void Step::rangerArea() {
    if (!deciding || t() != 0.0f)
        return;
    const float j = saturation(nativerules::skillLevel());
    Effect e = nativeeffects::made();
    put<int64_t>(e.data(), E_OWNER, myId());
    putPos(e.data(), E_POSITION, here());
    putVec(e.data(), E_VELOCITY, {0.0f, 0.0f, 0.0f});
    put<float>(e.data(), E_DAMAGE, damageFactor(ability()) * attackPower());
    put<float>(e.data(), E_RADIUS, j * 5.0f + 5.0f);
    put<float>(e.data(), E_SIZE, 1.0f);
    put<float>(e.data(), E_CHARGE, 0.0f);
    put<float>(e.data(), E_EXTRA, 0.0f);
    e[E_NO_CONTACT] = 0;
    put<int32_t>(e.data(), E_KIND, 6);
    e[E_HIT_TYPE] = 4;
    put<float>(e.data(), E_AGE, 0.0f);
    put<int32_t>(e.data(), E_LIFETIME, (int)(j * 10000.0f + 10000.0f));
    put<int32_t>(e.data(), E_FREEZE, 0);
    launchAndSend(e);
}

// §12 The shadow clone (0x8f), water bubbles (0x90) and heroic shout (0x95) are interactions.
void Step::queueInteraction() {
    if (!deciding)
        return;
    const int a = ability();
    uint8_t kind = 0;
    if (a == 0x8f && !(get<uint32_t>(r, FLAGS) & FLAG_SUMMONED) && t() == 0.0f)
        kind = nativeinteract::SHADOW_CLONE;
    else if (a == 0x90 && t() < (float)effectTime() && boundary(effectTime() / 8))
        kind = nativeinteract::WATER_BUBBLE;
    else if (a == 0x95 && passes((float)effectTime()))
        kind = nativeinteract::HEROIC_SHOUT;
    if (kind)
        c->interactions.push_back(nativeinteract::fresh(kind));
}

// §13.1 Dashes and rolls.
void Step::dash() {
    if (!deciding)
        return;
    if (t() == 0.0f)
        spendStamina(ability());
    if (t() <= 0.0f && 0.0f < later())
        sound(here(), 0x0e, randScaled(0.25f, 1.0f), myId());
}

// §13.2 The blink: to the free spot nearest where the aim meets a block, then down to the ground.
void Step::blink() {
    if (!deciding || !passes((float)effectTime()) || !w.overlaps)
        return;
    float m = mana() - manaCost(0x33);
    if (0.0f > m)
        m = 0.0f;
    setMana(m);
    const Vec3 v = aim();
    if (!(lengthSquared(v) > 0.0f))
        return;
    const float reachLength = std::sqrt(lengthSquared(v));
    const Vec3 d = normalised(v);
    const float length = nativeai::ray(sightWorld(), here().data(), d.data(), reachLength, true);
    const Pos me = here();
    const Pos endAt = {me[0] + (int64_t)((length * d[0]) * 65536.0f), me[1] + (int64_t)((length * d[1]) * 65536.0f),
                       me[2] + (int64_t)((length * d[2]) * 65536.0f)};
    const bool pass = hasBuff(0x13);
    const float size[3] = {get<float>(r, WIDTH), get<float>(r, DEPTH), get<float>(r, HEIGHT)};
    float best = 400.0f;
    Pos to = endAt;
    for (int ox = -4; ox <= 4; ox++)
        for (int oy = -4; oy <= 4; oy++)
            for (int oz = -2; oz <= 6; oz++) {
                const Pos q = {endAt[0] + (int64_t)ox * 65536, endAt[1] + (int64_t)oy * 65536,
                               endAt[2] + (int64_t)oz * 65536};
                if (w.overlaps(q.data(), size, pass))
                    continue;
                const float d2 = squaredBlocks(q, endAt);
                if (best > d2) {
                    best = d2;
                    to = q;
                }
            }
    if (!(400.0f > best))
        return;
    setPosition(r, to);
    // Down until it stands on something or in water. (The game has no limit; a world with
    // no ground at all would keep it falling, so this one stops after the world's height.)
    for (int fallen = 0; fallen < 0x10000; fallen++) {
        Pos p = here();
        const Pos below = {p[0], p[1], p[2] - 65536};
        if (w.overlaps(below.data(), size, pass))
            break;
        const nativeai::Block b = blockAt({p[0], p[1], p[2] - dots(get<float>(r, HEIGHT) * 0.5f) - 65536});
        if (b[4] == 2)
            break;
        p[2] -= 65536;
        setPosition(r, p);
    }
    put<float>(r, FALL_START, blocks(here()[2]));
}

// §13.3 Full stealth.
void Step::stealth() {
    if (!deciding || t() != 0.0f)
        return;
    put<float>(r, STEALTH, 1.0f);
    spendStamina(ability());
}

// §13.4 Returning to the home cell (the local player). Needs the cell height (0x2d87f0).
void Step::returnHome() {
    if (!local || !passes((float)effectTime()) || !w.cellHeight)
        return;
    const int32_t hx = get<int32_t>(r, HOME_CELL), hy = get<int32_t>(r, HOME_CELL + 4);
    // Cell-to-block arithmetic wraps to signed32 before conversion to fixed point.
    auto coordinate = [](int32_t cell, uint32_t offset) { return (int64_t)std::bit_cast<int32_t>((uint32_t)cell * 16u + offset) * 65536; };
    Pos p = {coordinate(hx, 6), coordinate(hy, 8), 0};
    p[2] = (int64_t)w.cellHeight(hx, hy) * 65536;
    setPosition(r, p);
    if (Creature *mount = byId(get<int64_t>(r, MOUNT)))
        setPosition(mount->record, p);
}

// §13.5 Travelling along a curve to a destination (the local player).
void Step::travel() {
    if (!local || !(t() > 2000.0f))
        return;
    const Pos start = {get<int64_t>(r, PATH_START), get<int64_t>(r, PATH_START + 8), get<int64_t>(r, PATH_START + 16)};
    const Pos dest = {get<int64_t>(r, DESTINATION), get<int64_t>(r, DESTINATION + 8), get<int64_t>(r, DESTINATION + 16)};
    if (64.0f > squaredBlocks(here(), dest)) {
        r[ABILITY] = 0;
        return;
    }
    const float u = (t() - 2000.0f) / 20000.0f;
    // A cubic Bezier curve through control points 20 blocks above the destination's height.
    const Vec3 p0 = {0.0f, 0.0f, 0.0f};
    const Vec3 p1 = {0.0f, 0.0f, blocks(0 + ((dest[2] + 20 * 65536) - start[2]))};
    const Vec3 p3 = {blocks(dest[0] - start[0]), blocks(dest[1] - start[1]), blocks(dest[2] - start[2])};
    const Vec3 p2 = {p3[0], p3[1], p3[2] + 20.0f};
    auto lerp = [u](const Vec3 &a, const Vec3 &b) {
        return Vec3{((b[0] - a[0]) * u) + a[0], ((b[1] - a[1]) * u) + a[1], ((b[2] - a[2]) * u) + a[2]};
    };
    const Vec3 a1 = lerp(p0, p1), b1 = lerp(p1, p2), c1 = lerp(p2, p3);
    const Vec3 d1 = lerp(a1, b1), e1 = lerp(b1, c1);
    const Vec3 tail = {(e1[0] - d1[0]) * u, (e1[1] - d1[1]) * u, (e1[2] - d1[2]) * u};
    const Pos goal = moved(moved(start, d1), tail);
    const float f = gamemath::blendFactor((double)dt, 0.01f);
    Pos p = here();
    for (int i = 0; i < 3; i++)
        p[i] = p[i] + dots(blocks(goal[i] - p[i]) * f);
    setPosition(r, p);
    putPos(r, DRAWN, goal);
    for (size_t at = VELOCITY; at < KNOCKBACK + 12; at += 4)
        put<float>(r, at, 0.0f);
    put<float>(r, FALL_START, blocks(p[2]));
}

// §13.6 The eagle's flight from where it started (+0x1b8) to the flight point (+0x1b0).
void Step::eagleFlight() {
    if (!local)
        return;
    auto land = [&](int64_t x, int64_t y) { return w.elevation ? w.elevation(truncBlock(x), truncBlock(y)) : 0.0f; };
    auto terrain = [&](int64_t x, int64_t y) { return w.ground ? w.ground(truncBlock(x), truncBlock(y)) : 0.0f; };
    auto overZero = [](float h) { return h > 0.0f ? h : 0.0f; }; // maxss: NaN gives 0
    Pos start = {get<int64_t>(r, PATH_START), get<int64_t>(r, PATH_START + 8), 0};
    Pos dest = {(int64_t)get<int32_t>(r, FLIGHT_POINT) * 65536, (int64_t)get<int32_t>(r, FLIGHT_POINT + 4) * 65536, 0};
    start[2] = dots(overZero(land(start[0], start[1])) + 200.0f);
    dest[2] = dots(overZero(land(dest[0], dest[1])) + 200.0f);
    const int N = flightTime(truncBlock(start[0]), truncBlock(start[1]), truncBlock(dest[0]), truncBlock(dest[1]));

    if (t() >= (float)N) { // arrival: the hang glider takes over
        r[ABILITY] = 0;
        Pos p = here();
        p[0] = dest[0], p[1] = dest[1];
        setPosition(r, p);
        bool found = false;
        if (c->inventory.size() > 1)
            for (const auto &slot : c->inventory[1])
                if (get<int32_t>(slot.data(), 0) > 0 && slot[4] == 0x18 && get<int32_t>(slot.data(), 4 + ITEM_SUBTYPE) == 1) {
                    std::memcpy(r + ITEM_IN_USE, slot.data() + 4, ITEM_SIZE);
                    found = true;
                    break;
                }
        r[ABILITY] = 0x78;
        if (!found) {
            r[ITEM_IN_USE] = 0x18;
            put<int32_t>(r, ITEM_IN_USE + ITEM_SUBTYPE, 1);
        }
        put<float>(r, TIME, 0.0f);
        // The landing marker (effect-objects.md §3.7).
        Effect e = nativeeffects::made();
        put<int32_t>(e.data(), E_KIND, 8);
        put<float>(e.data(), E_CHARGE, 0.0f);
        const Vec3 heading = normalised({blocks(dest[0] - start[0]), blocks(dest[1] - start[1]), 0.0f});
        putVec(e.data(), E_VELOCITY, scaled(heading, 20.0f));
        put<float>(e.data(), E_AGE, 0.0f);
        put<float>(e.data(), E_RADIUS, 0.0f);
        putPos(e.data(), E_POSITION, here());
        put<float>(e.data(), E_SIZE, get<float>(r, WIDTH) * 1.5f);
        launchAndSend(e);
        sound(here(), 0x79, 1.0f, 0);
    } else { // under way, eased in and out, high over the land
        double q = (double)t() / (double)(N - 1);
        q = q * q;
        const double g = 1.0 - (1.0 - q) * (1.0 - q);
        auto scale = [](int64_t v, int64_t f) { return (int64_t)((uint64_t)v * (uint64_t)f) / 65536; };
        Pos k;
        for (int i = 0; i < 3; i++)
            k[i] = start[i] + scale(dest[i] - start[i], (int64_t)(g * 65536.0));
        if (g > 0.5)
            for (int i = 0; i < 3; i++)
                k[i] = dest[i] + scale(start[i] - dest[i], (int64_t)((1.0 - g) * 65536.0));
        Pos p = here();
        const float h1 = land(k[0], k[1]) + 200.0f, h2 = terrain(p[0], p[1]) + 50.0f;
        float h = h1 > h2 ? h1 : h2;
        h = h > 200.0f ? h : 200.0f;
        k[2] = dots(h);
        // 0x2b8050: on a long flight, far from the destination, it flies higher.
        auto flat = [](int64_t dx, int64_t dy) {
            const double x = (double)dx / 65536.0, y = (double)dy / 65536.0;
            return std::sqrt(x * x + y * y);
        };
        if (flat(p[0] - dest[0], p[1] - dest[1]) > 1000.0 && flat(start[0] - dest[0], start[1] - dest[1]) > 16384.0)
            k[2] = (int64_t)500 * 65536;
        p[0] = k[0], p[1] = k[1];
        const int64_t rise = k[2] - p[2];
        p[2] = p[2] + (int64_t)((float)rise * gamemath::blendFactor((double)dt, 0.005f));
        if (g > (double)0.9f) {
            const int64_t fall = (int64_t)((double)(dest[2] - p[2]) * (g - (double)0.9f));
            p[2] = p[2] + (int64_t)((float)fall / 0.1f);
        }
        setPosition(r, p);
        putPos(r, DRAWN, p);
        putVec(r, KNOCKBACK, {0.0f, 0.0f, 0.0f});
        put<float>(r, FALL_START, blocks(p[2]));
    }

    // Always: the heading's speed, with a slow weave.
    double vx = (double)(dest[0] - start[0]) / 65536.0, vy = (double)(dest[1] - start[1]) / 65536.0, vz = 0.0;
    const double inv = 1.0 / std::sqrt(vx * vx + vy * vy + vz * vz);
    vx = vx * inv * 10.0, vy = vy * inv * 10.0, vz = vz * inv * 10.0;
    const Vec3 velocity = {(float)vx, (float)vy, (float)vz};
    putVec(r, VELOCITY, velocity);
    const float angle = t() * 0.001f;
    putVec(r, INTENT, {5.0f * nativecrt::cosf(angle) + velocity[0], 5.0f * nativecrt::sinf(angle) + velocity[1],
                       5.0f * 0.0f + velocity[2]});
}

// §13.7 Rising on the eagle before the flight.
void Step::eagleRise() {
    if (!local)
        return;
    Pos p = here();
    const float h1 = w.elevation ? w.elevation(truncBlock(p[0]), truncBlock(p[1])) : 0.0f;
    const float h2 = (w.ground ? w.ground(truncBlock(p[0]), truncBlock(p[1])) : 0.0f) + 50.0f;
    float h = (h1 > h2 ? h1 : h2) + 150.0f;
    const float f = gamemath::blendFactor((double)dt, 0.001f);
    h = h > 150.0f ? h : 150.0f;
    p[2] = p[2] + (int64_t)((float)(dots(h) - p[2]) * f);
    setPosition(r, p);
    putVec(r, VELOCITY, {10.0f, 5.0f, 0.0f});
    putVec(r, KNOCKBACK, {0.0f, 0.0f, 0.0f});
    put<float>(r, FALL_START, blocks(p[2]));
    const float angle = t() * 0.001f;
    putVec(r, INTENT, {5.0f * nativecrt::cosf(angle) + 10.0f, 5.0f * nativecrt::sinf(angle) + 5.0f, 5.0f * 0.0f + 0.0f});
}

// §14.1 Summoning a creature that fights for C (at most three at once).
void Step::summon() {
    if (!deciding || !passes((float)effectTime()) || !w.construct)
        return;
    const int a = ability();
    const int64_t me = myId();
    int owned = 0;
    int64_t smallest = FIRST_MADE_ID;
    for (Creature *x : w.creatures) {
        if (get<int64_t>(x->record, OWNER) == me && (get<uint32_t>(x->record, FLAGS) & FLAG_SUMMONED))
            owned++;
        smallest = std::min(smallest, idOf(x->record));
    }
    if (!w.creatures.empty() && owned > 2)
        return;
    Creature *made = w.construct(smallest - 1);
    if (!made)
        return;
    uint8_t *m = made->record;
    m[HOSTILITY] = (r[HOSTILITY] == 1 || r[HOSTILITY] == 7) ? 1 : 4;
    put<int32_t>(m, LEVEL, get<int32_t>(r, LEVEL));
    int race = 0x25;
    if (!c->summonRaces.empty())
        race = c->summonRaces[(size_t)((uint64_t)(int64_t)rand() % c->summonRaces.size())];
    const int32_t mode = get<int32_t>(r, SUMMON_MODE);
    if (mode == 1 || mode == 2) { // a copy of C, or of the creature it fought last
        const uint8_t *from = r;
        if (mode == 2)
            if (Creature *foe = byId(get<int64_t>(r, LAST_FOE)))
                from = foe->record;
        put<int32_t>(m, RACE, get<int32_t>(from, RACE));
        std::memcpy(m + EQUIPMENT, from + EQUIPMENT, EQUIPMENT_SIZE);
        std::memcpy(m + APPEARANCE, from + APPEARANCE, APPEARANCE_SIZE);
        m[CLASS] = from[CLASS];
    } else { // one of its races, with that race's looks and gear (0x53e80, 0x511e0)
        put<int32_t>(m, RACE, race);
        auto next = [this] { return rand(); };
        const worldgen::Draw draw(next);
        worldgen::Appearance looks; // (keeping the flags it has)
        std::memcpy(looks.bytes.data(), m + APPEARANCE, worldgen::Appearance::SIZE);
        worldgen::makeLooks(worldgen::sharedCreatureTables(), race, false, draw, looks);
        std::memcpy(m + APPEARANCE, looks.bytes.data(), worldgen::Appearance::SIZE);
        worldgen::Gear gear;
        worldgen::makeGear({m[HOSTILITY], race, get<int32_t>(m, LEVEL), get<uint32_t>(m, FLAGS), m[ELEMENT]}, draw, gear);
        worldgen::wearGear(m, gear);
        made->skills.assign(gear.abilities.begin(), gear.abilities.end());
        made->summonRaces.assign(gear.abilities2.begin(), gear.abilities2.end());
        made->inventory = worldgen::inventoryRecords(gear.inventory);
    }
    put<uint32_t>(m, FLAGS, (get<uint32_t>(m, FLAGS) & ~FLAG_CLEARED_ON_SUMMON) | FLAG_SUMMONED);
    put<int32_t>(m, LEVEL, get<int32_t>(r, LEVEL));
    put<float>(m, HEALTH, maxHealthOf(made));
    setPosition(m, here());
    putPos(m, PATH_START, here());
    m[ELEMENT] = r[ELEMENT];
    put<uint32_t>(m, AI_FLAGS, a == 0x65 ? 0xb0u : 0x1000u);
    put<int64_t>(m, OWNER, me);
    if (w.added)
        w.added(made);
}

// §14.2 Taming: each feeding (the tamer's item 0x15 of C's race) adds a fifth; at full, C
// becomes the tamer's pet under a new id.
void Step::tame() {
    if (!authoritative || !reaches((float)effectTime()))
        return;
    Creature *x = byId(get<int64_t>(r, LAST_FOE));
    if (!x || x->record[ITEM_IN_USE] != 0x15 || get<int32_t>(x->record, ITEM_IN_USE + ITEM_SUBTYPE) != get<int32_t>(r, RACE))
        return;
    sound(here(), 0x2f, randScaled(0.2f, 0.9f), 0);
    float s = 0.0f;
    for (const nativebuffs::Buff &b : c->buffs) // the last buff of type 7
        if (b[0] == 7)
            s = nativebuffs::strength(b);
    raiseCombo();
    s = s + 0.2f;
    if (s >= 1.0f) {
        s = 1.0f;
        const int64_t oldId = myId();
        if (w.markDead)
            w.markDead(oldId);
        int64_t smallest = FIRST_MADE_ID;
        for (Creature *y : w.creatures)
            smallest = std::min(smallest, idOf(y->record));
        put<int64_t>(r, ID, smallest - 1);
        if (w.renumbered)
            w.renumbered(oldId, c);
        r[HOSTILITY] = 6;
        // The appearance goes back to its constructor's defaults (0x4d1f0) before it is
        // made a pet.
        std::memcpy(r + worldgen::creature::APPEARANCE, worldgen::Appearance::constructed().bytes.data(), worldgen::Appearance::SIZE);
        if (w.adopt)
            w.adopt(idOf(x->record), c);
    }
    w.numbers.push_back(number(myId(), idOf(x->record), s, here(), false, 6));
    giveAndTell(c, nativebuffs::make(7, s, 5000.0f, get<int64_t>(r, OWNER)), true);
}

// §14.3 A pet with its owner: the owner's healing skill (0x82) is ended.
void Step::petWithOwner() {
    if (get<int64_t>(r, OWNER) == myId()) {
        put<int64_t>(r, OWNER, 0);
        r[ABILITY] = 0;
    }
    if (!reaches(50.0f))
        return;
    if (authoritative)
        if (Creature *y = byId(get<int64_t>(r, TARGET)))
            if (y->record[ABILITY] == 0x82)
                y->record[ABILITY] = 0;
    if (deciding)
        sound(here(), 0x30, randScaled(0.1f, 1.0f), myId());
}

// §14.4 Throwing the creature C holds.
void Step::throwCreature() {
    if (get<int64_t>(r, TARGET) == myId()) {
        put<int64_t>(r, TARGET, 0);
        r[ABILITY] = 0;
    }
    if (t() > 2000.0f)
        r[ABILITY] = 0;
    if (!reaches(50.0f))
        return;
    if (deciding)
        sound(here(), 0x0f, randScaled(0.1f, 2.0f), myId());
    if (!authoritative)
        return;
    Creature *x = byId(get<int64_t>(r, TARGET));
    if (!x)
        return;
    const float angle = (float)(((double)get<float>(r, YAW) * PI) / 180.0 + PI / 2.0);
    const float s = nativecrt::sinf(angle), co = nativecrt::cosf(angle);
    const Vec3 v = vecAt(r, VELOCITY);
    putVec(x->record, KNOCKBACK, {10.0f * co + v[0], 10.0f * s + v[1], 5.0f + v[2]});
}

// §14.5 Calling up the owner, a creature of race 0x12e, which then rises.
void Step::callUp() {
    if (!authoritative)
        return;
    if (200.0f < t() && t() < (float)duration() && boundary(400)) {
        const Basis y = yawBasis(get<float>(r, YAW));
        const Vec3 o = rotated(y, {0.0f, get<float>(r, DEPTH) * 0.6f, 0.1f - get<float>(r, HEIGHT) * 0.5f});
        burst(moved(here(), o), {0.0f, 0.0f, 5.0f}, {0.6f, 0.5f, 0.3f, 1.0f}, 0.2f, 5, 0, 3.0f);
        sound(here(), 0x30, randScaled(0.1f, 0.5f), 0);
    }
    if (t() > 1800.0f) {
        Creature *o = byId(get<int64_t>(r, OWNER));
        if (o && get<int32_t>(o->record, RACE) == 0x12e && o->record[RISING] == 0) {
            o->record[RISING] = 1;
            o->record[ABILITY] = 0x99;
            put<float>(o->record, TIME, 0.0f);
        }
    }
}

// §14.6 The local player's self-kill.
void Step::selfKill() {
    if (local && passes((float)effectTime()))
        put<float>(r, HEALTH, 0.0f);
}

// §15 Ability 9: a sound as the effect part ends, then a rising hum.
void Step::chargeSound() {
    if (!deciding)
        return;
    if (passes((float)effectTime()))
        sound(here(), 0x10, randScaled(0.25f, 1.0f), myId());
    if (boundary(200))
        sound(here(), 0x3a, get<float>(r, CHARGE) * 0.5f + 1.0f, myId());
}

// §15 Ability 0x4a: a growl.
void Step::growl() {
    if (authoritative && reaches((float)effectTime()))
        sound(here(), 0x2e, randScaled(0.25f, 0.6f), 0);
}

// §15 Ability 0x53: a hum of random pitch and volume every half second.
void Step::randomHum() {
    if (!deciding || !boundary(500))
        return;
    const float pitch = randScaled(0.25f, 0.5f);
    const float volume = randScaled(0.25f, 0.5f);
    sound(here(), HUM_SOUND, pitch, 0, volume);
}

// §16 Using a special item (item type 0x18): the local player's, over 5 s.
void Step::specialItem() {
    if (!local)
        return;
    const uint8_t kind = r[ITEM_IN_USE];
    const int32_t sub = get<int32_t>(r, ITEM_IN_USE + ITEM_SUBTYPE);
    const bool special = kind == 0x18;
    const bool useMoment = reaches(5000.0f);
    const Pos me = here();
    auto objectAt = [](const nativeai::Object &o) {
        return Pos{get<int64_t>(o.data(), O_POSITION), get<int64_t>(o.data(), O_POSITION + 8),
                   get<int64_t>(o.data(), O_POSITION + 16)};
    };
    auto cellOf = [](int64_t x) { return floorDiv(floorBlock(x), 16); };

    if (special && sub == 7 && useMoment) { // finds the way to the nearest cell object 0x3c
        nearZones([&](std::vector<nativeai::Object> &objects, int, int) {
            for (const nativeai::Object &o : objects) {
                if (get<int32_t>(o.data(), O_TYPE) != 1 || !(squaredBlocks(objectAt(o), me) < 64.0f))
                    continue;
                putPos(r, DESTINATION, {0, 0, 0});
                const Pos op = objectAt(o);
                const int ocx = cellOf(op[0]), ocy = cellOf(op[1]);
                for (int cx = ocx - 10; cx <= ocx + 10; cx++)
                    for (int cy = ocy - 10; cy <= ocy + 10; cy++)
                        if (w.cellObject && w.cellHeight && w.cellObject(cx, cy) == 0x3c) {
                            putPos(r, DESTINATION, {((int64_t)cx * 16 + 24) * 65536, ((int64_t)cy * 16 + 24) * 65536,
                                                    (int64_t)w.cellHeight(cx, cy) * 65536});
                            r[ABILITY] = 0x98;
                            put<float>(r, TIME, 0.0f);
                            putPos(r, PATH_START, me);
                        }
                sound(me, 0x73, 1.0f, 0);
                break; // the zone's other objects are skipped
            }
        });
        return;
    }
    if (special && sub == 4 && useMoment) { // marks the cells of the objects of kind 0 nearby
        nearZones([&](std::vector<nativeai::Object> &objects, int, int) {
            for (const nativeai::Object &o : objects) {
                if (get<int32_t>(o.data(), O_TYPE) != 0 || !(squaredBlocks(objectAt(o), me) < 64.0f))
                    continue;
                const Pos op = objectAt(o);
                if (!w.discover || !w.discover(cellOf(op[0]), cellOf(op[1])))
                    continue;
                for (auto &tab : c->inventory)
                    for (auto &slot : tab)
                        if (get<int32_t>(slot.data(), 0) != 0 && slot[4] == 0x18 &&
                            get<int32_t>(slot.data(), 4 + ITEM_SUBTYPE) == 4)
                            put<int32_t>(slot.data(), 0xc, get<int32_t>(slot.data(), 0xc) + 1);
                sound(me, 0x6e, 1.0f, 0);
            }
        });
        return;
    }
    if (t() == 0.0f) {
        static const std::pair<int32_t, int32_t> itemSounds[] = {{4, 0x6d}, {5, 0x6f}, {6, 0x71}, {7, 0x72},
                                                                 {8, 0x74}, {10, 0x76}, {11, 0x77}};
        int32_t id = -1;
        if (special)
            for (const auto &[s, sid] : itemSounds)
                if (s == sub)
                    id = sid;
        if (id >= 0)
            sound(me, id, 1.0f, myId());
        else
            sound(me, 0x3c, randScaled(0.1f, 1.0f), myId());
    }
    if (special && sub >= 4 && sub <= 11 && sub != 9 && t() < 5000.0f && boundary(500)) {
        const Basis y = yawBasis(get<float>(r, YAW));
        const Vec3 o = rotated(y, {0.0f, get<float>(r, DEPTH) * 0.6f, get<float>(r, HEIGHT) * 0.0f});
        burst(moved(me, o), {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 1.0f, 1, 7, 0.5f);
    }
    if (special && sub == 0x0d && reaches(200.0f)) { // opens the gates of kind 9 nearby
        nearZones([&](std::vector<nativeai::Object> &objects, int zx, int zy) {
            for (nativeai::Object &o : objects) {
                if (get<int32_t>(o.data(), O_TYPE) != 9 || o[O_STATE] == 0 || !(squaredBlocks(objectAt(o), me) < 64.0f))
                    continue;
                const uint8_t before = o[O_STATE];
                nativeinteract::openOrShut(o, 0, w.sounds); // 0x2d3a10
                if (o[O_STATE] != before)
                    w.changedZones.insert({zx, zy});
            }
        });
        return;
    }
    if (useMoment) { // the item's own use is an interaction of kind 2
        nativeai::Interaction in = nativeinteract::fresh(2);
        std::memcpy(in.data() + nativeinteract::ITEM, r + ITEM_IN_USE, ITEM_SIZE);
        c->interactions.push_back(in);
        if (special && sub == 8) {
            sound(me, 0x75, 1.0f, 0);
            const nativebuffs::Buff b = nativebuffs::make(0x13, 0.0f, 60000.0f, 0);
            giveAndTell(c, b, true);
            if (Creature *mount = byId(get<int64_t>(r, MOUNT)))
                giveAndTell(mount, b, true);
        }
    }
}

} // namespace

void ability(World &world, nativeai::Creature *c, float dt) {
    Step step(world, c, dt);
    step.run();
}

int flightTime(int32_t fromX, int32_t fromY, int32_t toX, int32_t toY) {
    const int32_t dx = (int32_t)((uint32_t)fromX - (uint32_t)toX), dy = (int32_t)((uint32_t)fromY - (uint32_t)toY);
    int ms = (int)(std::sqrt((double)dx * (double)dx + (double)dy * (double)dy) * 2.5);
    if (ms < 10000)
        ms = 10000;
    if (ms > 30000)
        ms = 30000;
    return ms;
}

} // namespace nativeability
