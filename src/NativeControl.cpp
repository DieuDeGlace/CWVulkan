// The local player's controls (docs/specs/player-control.md). Section numbers in the
// comments ("spec 3.6") are the spec's; RVAs are the game's routines, cited as references.
//
// The movement routine is checked bit for bit against the game's recorded calls
// (EngineTests controloracle), so its arithmetic follows the spec's order of operations,
// including the "times zero" terms, which matter for signed zeros and NaN.
#include "NativeControl.h"

#include "GameMath.h"
#include "worldgen/Residents.h"
#include "NativeCrt.h"
#include "NativeInteract.h"
#include "NativeRules.h"

#include <cmath>
#include <cstring>
#include <map>

namespace nativecontrol {
namespace {

// --- The player record (spec 1.3) ---------------------------------------------------------
constexpr size_t ID = 0x08;
constexpr size_t POSITION = 0x10;     // i64 x3, 1/65536 block
constexpr size_t ROLL = 0x2c;         // tilts the glider
constexpr size_t YAW = 0x30;
constexpr size_t VELOCITY = 0x34;     // f32 x3; z is up
constexpr size_t INTENT = 0x40;       // f32 x3: the way the player wants to go
constexpr size_t PUSH = 0x4c;         // f32 x3: a push (wall jump, blink)
constexpr size_t MOVE_FLAGS = 0x5c;
constexpr size_t ABILITY = 0x68;
constexpr size_t ABILITY_TIME = 0x6c;
constexpr size_t TALL = 0x8c;         // the aim assist aims this far up the target, times 0.45
constexpr size_t STATE_FLAGS = 0x128; // u16
constexpr size_t DODGE_TIMER = 0x12c;
constexpr size_t STUN_TIMER = 0x130;
constexpr size_t LOCK_TIMER = 0x134;
constexpr size_t CLASS = 0x14c;
constexpr size_t SPECIALISATION = 0x14d;
constexpr size_t CHARGED = 0x158;     // how far a charged move has charged
constexpr size_t LOOK = 0x174;        // f32 x3: from the player to the aim point, blocks
constexpr size_t OWNER = 0x1a0;
constexpr size_t LINKED = 0x1a8;      // the mount while riding; the charge's target
constexpr size_t HELD_ITEM_TYPE = 0x1d8;
constexpr size_t SECOND_HAND_SUBTYPE = 0x63c;
constexpr size_t WEAPON_SUBTYPE = 0x6dc;
constexpr size_t STAMINA = 0x9a0;     // 0..1
constexpr size_t WALL_NORMAL = 0x9ac; // f32 x3
constexpr size_t PET = 0x9d8;
constexpr size_t CHARGE_LENGTH = 0x9e8; // ms the warriors' charge lasts
constexpr size_t LOCKED_AIM = 0xae0;  // i64 x3: a beam's aim point
constexpr size_t RENDER_POSITION = 0xb10;
constexpr size_t GLIDE_LIFT = 0xbb4;  // scales the glider's lift
constexpr size_t MOUNT_FLAGS = 0x80;  // of the mount: bit 1, a flying mount

// Movement flags (+0x5c).
constexpr uint32_t ON_GROUND = 1, IN_WATER = 2, ON_CLIMBABLE_WALL = 4, WALL_JUMP_CONTACT = 0x1000;
// State flags (+0x128).
constexpr uint16_t LAMP = 1, LOOKING_AT_OWNER = 4, RUNNING = 0x40, CLEARED_EACH_FRAME = 0x200;
constexpr uint32_t FLYING_MOUNT = 2;

// --- Abilities named here -----------------------------------------------------------------
constexpr int GUARD = 0x09, TELEPORT = 0x33, MAGE_BLINK = 0x34, CHARGE = 0x37, CLEARED_EACH_CALL = 0x36;
constexpr int SNEAK = 0x50, SPRINT = 0x51, MAGE_FLOAT = 0x53, WARRIOR_STANCE = 0x5f, INTERCEPT = 0x32;
constexpr int RIDING_PET = 0x76, GLIDING = 0x78, SAILING = 0x79, CLIMBING = 0x7a, INPUT_LOCK = 0x7d;
constexpr int RIDING = 0x80, LOWER_JUMP_A = 0x86, LOWER_JUMP_B = 0x87, LOOK_AT_OWNER = 0x9a;
constexpr int CALL_PET = 0x9b, EAGLE_A = 0x9c, EAGLE_B = 0x9d, WARRIOR_LEAP = 0x9f;
constexpr int RANGER_DODGE = 0x16, ROGUE_DODGE = 0xa0, IGNORED_0X8C = 0x8c;

// --- Constants (spec 6) -------------------------------------------------------------------
constexpr float WALK_STEP = 80.0f, SAIL_STEP = 5.0f;
constexpr float SAIL_STOP2 = 225.0f, SAIL_CAP2 = 1600.0f, SAIL_CAP = 40.0f;
constexpr float STICK_DEAD2 = 0.01f;
constexpr float GLIDE_FORWARD = 10.0f, GLIDE_ROLL_STEER = 0.05f, GLIDE_SMOOTHING = 0.001f;
constexpr float GLIDE_STAMINA_FLOOR = 0.001f, GLIDE_RATE = 0.002f, GLIDE_DIVE_RATE = 0.0015f;
constexpr float GLIDE_LIFT_SCALE = 0.001f, GLIDE_LIFT_OFFSET = 0.5f, GLIDE_LIFT_SPEED_LIMIT = 5.0f;
constexpr float GLIDE_INTENT_CAP = 12.0f;
constexpr double GLIDE_STEP_PER_MS = 4.0;
constexpr float CLIMB_STEP = 0.2f;
constexpr float DODGE_STAMINA = 0.25f, DODGE_SPEED = 15.0f, DODGE_HOP = 5.0f, DODGE_TIME = 600.0f;
constexpr float WALL_JUMP_SPEED = 15.0f, WALL_JUMP_TURN = 180.0f, WALL_JUMP_STAMINA = 25.0f;
constexpr int LIFT_MS = 100, FLYING_MOUNT_LIFT_MS = 500, TAKE_OFF_WINDOW_MS = 200;
constexpr float LOW_JUMP = 5.0f, JUMP = 10.0f, HIGH_JUMP = 20.0f;
constexpr double WATER_RISE_PER_MS = 0.001, HOLD_RISE_PER_MS = 0.08;
constexpr float RISE_CAP = 10.0f;
constexpr float INPUT_LOCK_MS = 2000.0f, LONG_FINISHED_MS = 10000.0f;
constexpr float DEFAULT_AIM_DISTANCE = 100.0f, DOTS_PER_BLOCK = 65536.0f, BLOCKS_PER_DOT = 1.0f / 65536.0f;
constexpr float CHARGE_SPEED = 50.0f, CHARGE_TIME_SCALE = 1000.0f, CHARGE_SPEED2_CAP = 2500.0f, CHARGE_HOP = 15.0f;
constexpr float CHARGE_EMPTY = 0.01f, STANDING_DODGE2 = 0.01f;
constexpr float WARRIOR_STANCE_LIMIT = 1000.0f, MAGE_FLOAT_LOWEST = -0.2f;
constexpr float BLINK_SPEED = 20.0f, BLINK_HOP = 15.0f;
constexpr float PET_RANGE2 = 36.0f;
constexpr int PET_SPECIAL_ITEM = 3, CALL_ITEM_TYPE = 0x21, CALL_PET_SOUND = 0x7c, CALL_PET_KIND = 10;
constexpr float ASSIST_STICK2 = 0.1f, ASSIST_HEIGHT = 0.45f, ASSIST_RANGED_SPEED = 150.0f, ASSIST_SPEED = 500.0f;
constexpr float ASSIST_RATE = 0.015f, ASSIST_ROLLING_RATE = 0.03f, ASSIST_FALLOFF = -20.0f;
constexpr float DEG_TO_RAD = 0.0174532924f;
constexpr double PI = 3.14159265358979323846;

// A view of a game-layout record by offset.
struct Rec {
    uint8_t *p;
    float &f(size_t o) const { return *reinterpret_cast<float *>(p + o); }
    int64_t &l(size_t o) const { return *reinterpret_cast<int64_t *>(p + o); }
    uint32_t &u(size_t o) const { return *reinterpret_cast<uint32_t *>(p + o); }
    uint16_t &h(size_t o) const { return *reinterpret_cast<uint16_t *>(p + o); }
    uint8_t &b(size_t o) const { return p[o]; }
};

struct Vec3 {
    float x = 0, y = 0, z = 0;
};
Vec3 vec(Rec c, size_t o) { return {c.f(o), c.f(o + 4), c.f(o + 8)}; }
void setVec(Rec c, size_t o, Vec3 v) { c.f(o) = v.x, c.f(o + 4) = v.y, c.f(o + 8) = v.z; }
float length2(Vec3 v) { return (v.x * v.x + v.y * v.y) + v.z * v.z; }
// 0x127f0: one reciprocal, then three products.
Vec3 normalised(Vec3 v) {
    const float k = 1.0f / std::sqrt(length2(v));
    return {v.x * k, v.y * k, v.z * k};
}
// The difference of two positions in blocks.
Vec3 blocksBetween(const int64_t *from, const int64_t *to) {
    return {(float)(to[0] - from[0]) * BLOCKS_PER_DOT, (float)(to[1] - from[1]) * BLOCKS_PER_DOT,
            (float)(to[2] - from[2]) * BLOCKS_PER_DOT};
}
const int64_t *position(Rec c, size_t o) { return &c.l(o); }

nativerules::HasBuff buffTest(const std::function<bool(int)> &hasBuff) {
    if (hasBuff)
        return hasBuff;
    return [](int) { return false; };
}
nativerules::HasBuff buffTest(const nativeai::Creature &creature) {
    return [&creature](int type) {
        for (const auto &e : creature.buffs)
            if (e[0] == (uint8_t)type)
                return true;
        return false;
    };
}

// 0x5ee70: gliding is the glider ability off the ground, neither stunned nor rolling.
bool gliding(Rec c) {
    return c.b(ABILITY) == GLIDING && !(c.u(MOVE_FLAGS) & ON_GROUND) && c.f(STUN_TIMER) <= 0.0f &&
           c.f(DODGE_TIMER) <= 0.0f;
}
// A glider out of stamina cannot steer.
bool gliderSpent(Rec c) {
    return c.b(ABILITY) == GLIDING && !(c.u(MOVE_FLAGS) & ON_GROUND) && c.f(STAMINA) <= 0.0f;
}
bool climbing(Rec c) { return c.b(ABILITY) == CLIMBING && (c.u(MOVE_FLAGS) & ON_CLIMBABLE_WALL); }
// The two moves whose ability time is shortened when jumping, while they last.
bool lowerJump(Rec c, float duration) {
    const int a = c.b(ABILITY);
    return (a == LOWER_JUMP_A || a == LOWER_JUMP_B) && c.f(ABILITY_TIME) < duration;
}

// 0x17840 (creature-physics.md 7.3): the angle `from` turned toward `to` by the shorter way,
// by `share` of the gap; nearly opposite angles jump straight to `to`.
float turnToward(float from, float to, float share) {
    const float a = (float)((double)(from / 180.0f) * PI), b = (float)((double)(to / 180.0f) * PI);
    const double cosA = nativecrt::cosf(a), sinA = nativecrt::sinf(a);
    const double cosB = nativecrt::cosf(b), sinB = nativecrt::sinf(b);
    if (cosB * cosA + sinB * sinA < -0.9)
        return to;
    float d = (float)(sinB * cosA - cosB * sinA);
    d = d < 1.0f ? d : 1.0f;
    d = d > -1.0f ? d : -1.0f;
    return (float)((double)nativecrt::asinf(d) * ((double)share * 180.0 / PI)) + from;
}

// 0x13050 on the identity (creature-physics.md 7.2): the rows of the camera's yaw turn.
// Row 0 points right of where row 1 points; the ground's forward is -row 1.
struct YawBasis {
    float r[3][3];
};
YawBasis yawBasis(float degrees) {
    const float t = degrees * DEG_TO_RAD;
    const float cs = nativecrt::cosf(t), sn = nativecrt::sinf(t);
    return {{{cs, sn, 0.0f}, {-sn, cs, 0.0f}, {0.0f, 0.0f, 1.0f}}};
}

// --- The movement routine's steps (spec 3) --------------------------------------------------

bool anyMoveKey(const Input &in) { return in.forward || in.back || in.left || in.right; }
float stick2(const Input &in) { return in.stickX * in.stickX + in.stickY * in.stickY; }

// Spec 3.1: sailing keeps and caps its intent; any other ability on foot starts from rest.
void sailOrClear(const Input &in, Rec c, float &step) {
    if (c.b(ABILITY) != SAILING) {
        setVec(c, INTENT, {});
        return;
    }
    step = SAIL_STEP;
    if (length2(vec(c, INTENT)) < SAIL_STOP2 && !anyMoveKey(in) && stick2(in) < STICK_DEAD2)
        setVec(c, INTENT, {});
    const float l2 = length2(vec(c, INTENT));
    if (l2 > SAIL_CAP2) { // the boat's wanted speed is capped at 40
        const float k = 1.0f / std::sqrt(l2);
        c.f(INTENT + 4) = k * c.f(INTENT + 4);
        c.f(INTENT) = k * c.f(INTENT);
        c.f(INTENT + 8) = k * c.f(INTENT + 8);
        c.f(INTENT) *= SAIL_CAP, c.f(INTENT + 4) *= SAIL_CAP, c.f(INTENT + 8) *= SAIL_CAP;
    }
}

// Spec 3.2: the glider is steered toward where it flies and turned by its roll. Returns
// false when the glider is out of stamina, which ends the routine.
bool steerGlider(Rec c, double dt) {
    const float vx = c.f(VELOCITY), vy = c.f(VELOCITY + 4);
    const float speed2 = (vx * vx + vy * vy) + 0.0f;
    if (speed2 > 0.0f) {
        const float k = 1.0f / std::sqrt(speed2);
        const float dx = vx * k, dy = vy * k, t = (k * 0.0f) * 0.0f;
        const float sideA = dx - t, sideB = t - dy, sideC = dy * 0.0f - dx * 0.0f;
        const float e = gamemath::blendFactor(dt, GLIDE_SMOOTHING);
        c.f(INTENT) = (dx * GLIDE_FORWARD - c.f(INTENT)) * e + c.f(INTENT);
        c.f(INTENT + 4) = (dy * GLIDE_FORWARD - c.f(INTENT + 4)) * e + c.f(INTENT + 4);
        const float roll = c.f(ROLL);
        c.f(INTENT + 4) = c.f(INTENT + 4) - (sideA * roll) * GLIDE_ROLL_STEER;
        c.f(INTENT) = c.f(INTENT) - (sideB * roll) * GLIDE_ROLL_STEER;
        c.f(INTENT + 8) = c.f(INTENT + 8) - (sideC * roll) * GLIDE_ROLL_STEER;
    }
    if (GLIDE_STAMINA_FLOOR > c.f(STAMINA)) {
        c.h(STATE_FLAGS) &= (uint16_t)~RUNNING;
        const float e = gamemath::blendFactor(dt, GLIDE_RATE);
        c.f(INTENT + 8) = e * (0.0f - c.f(INTENT + 8)) + c.f(INTENT + 8);
        return false;
    }
    return true;
}

// Spec 3.3: holding the jump lifts the glider, pushing forward dives; both in proportion to
// how fast it sinks.
void liftOrDive(const Input &in, Rec c, double dt) {
    float g = c.f(GLIDE_LIFT) * GLIDE_LIFT_SCALE - GLIDE_LIFT_OFFSET;
    g = g > 0.0f ? g : 0.0f;
    g = g < 1.0f ? g : 1.0f;
    const float sink = -c.f(VELOCITY + 8);
    const float fall = sink > 0.0f ? sink : 0.0f;
    if (in.jump && c.f(STAMINA) > 0.0f && c.f(VELOCITY + 8) <= GLIDE_LIFT_SPEED_LIMIT) {
        c.f(INTENT + 8) = (float)((double)(g * GLIDE_RATE) * dt * (double)fall + (double)c.f(INTENT + 8));
    } else {
        const float push = in.forward ? 1.0f : in.stickY;
        if (in.forward || push > 0.0f)
            c.f(INTENT + 8) =
                (float)((double)((g * push) * GLIDE_DIVE_RATE) * dt * (double)fall + (double)c.f(INTENT + 8));
        const float e = gamemath::blendFactor(dt, GLIDE_RATE);
        c.f(INTENT + 8) = e * (0.0f - c.f(INTENT + 8)) + c.f(INTENT + 8);
    }
    if (c.f(VELOCITY + 8) > 0.0f && c.f(INTENT + 8) > GLIDE_INTENT_CAP)
        c.f(INTENT + 8) = GLIDE_INTENT_CAP;
}

enum class Mode { Climbing, Swimming, Walking, None };
Mode keyMode(Rec c) {
    if (climbing(c))
        return Mode::Climbing;
    if ((c.u(MOVE_FLAGS) & IN_WATER) && c.b(ABILITY) != SAILING)
        return Mode::Swimming;
    return gliderSpent(c) ? Mode::None : Mode::Walking;
}

enum class Key { Forward, Back, Right, Left };

// Spec 3.6, on the wall: up and down, or along the wall (across its normal N).
void climbKey(Rec c, Key key, float step) {
    const float k = step * CLIMB_STEP;
    const Vec3 n = vec(c, WALL_NORMAL);
    float *intent = &c.f(INTENT);
    switch (key) {
    case Key::Forward: intent[2] = step * CLIMB_STEP + intent[2]; break;
    case Key::Back: intent[2] = intent[2] - step * CLIMB_STEP; break;
    case Key::Right:
    case Key::Left: {
        const float nz0 = n.z * 0.0f;
        const float along[3] = {(n.y - nz0) * k, (nz0 - n.x) * k, (n.x * 0.0f - n.y * 0.0f) * k};
        for (int i = 0; i < 3; i++)
            intent[i] = key == Key::Right ? intent[i] - along[i] : intent[i] + along[i];
        break;
    }
    }
}

// Spec 3.6, swimming: along the camera's rows A (left) and C (the view).
void swimKey(const Input &in, Rec c, Key key, float step) {
    const float(&a)[3] = in.camera[0], (&b)[3] = in.camera[1], (&v)[3] = in.camera[2];
    float *intent = &c.f(INTENT);
    for (int i = 0; i < 3; i++) {
        float w = 0;
        switch (key) {
        case Key::Forward: w = (b[i] * 0.0f + a[i] * 0.0f) + v[i]; break;
        case Key::Back: w = (b[i] * 0.0f + a[i] * 0.0f) - v[i] * 1.0f; break;
        case Key::Right: w = (b[i] * 0.0f - a[i] * 1.0f) + v[i] * 0.0f; break;
        case Key::Left: w = (b[i] * 0.0f + a[i]) + v[i] * 0.0f; break;
        }
        intent[i] = w * step + intent[i];
    }
}

// Spec 3.6, on foot or flying: by the camera's yaw.
void walkKey(const YawBasis &y, Rec c, Key key, float step) {
    const float(&r0)[3] = y.r[0], (&r1)[3] = y.r[1], (&r2)[3] = y.r[2];
    float *intent = &c.f(INTENT);
    for (int i = 0; i < 3; i++) {
        const float ahead = (r0[i] * 0.0f - r1[i] * 1.0f) + r2[i] * 0.0f;
        const float side = (r1[i] * 0.0f + r0[i]) + r2[i] * 0.0f;
        switch (key) {
        case Key::Forward: intent[i] = ahead * step + intent[i]; break;
        case Key::Back: intent[i] = intent[i] - ahead * step; break;
        case Key::Right: intent[i] = intent[i] - side * step; break;
        case Key::Left: intent[i] = side * step + intent[i]; break;
        }
    }
}

void moveKeys(const Input &in, Rec c, const YawBasis &y, float step) {
    const std::pair<bool, Key> keys[] = {
        {in.forward, Key::Forward}, {in.back, Key::Back}, {in.right, Key::Right}, {in.left, Key::Left}};
    for (const auto &[held, key] : keys) {
        if (!held)
            continue;
        switch (keyMode(c)) { // the mode is judged anew for each key
        case Mode::Climbing: climbKey(c, key, step); break;
        case Mode::Swimming: swimKey(in, c, key, step); break;
        case Mode::Walking: walkKey(y, c, key, step); break;
        case Mode::None: break;
        }
    }
}

// Spec 3.7: the stick, in the same three modes (water here includes sailing).
void moveStick(const Input &in, Rec c, const YawBasis &y, float step) {
    if (stick2(in) == 0.0f)
        return;
    const float sx = -in.stickX;
    float *intent = &c.f(INTENT);
    auto add = [&](const float w[3], float s) {
        intent[2] = w[2] * s + intent[2];
        intent[1] = w[1] * s + intent[1];
        intent[0] = w[0] * s + intent[0];
    };
    if (climbing(c)) {
        const float k = step * CLIMB_STEP;
        const Vec3 n = vec(c, WALL_NORMAL);
        intent[0] = (n.y * sx - n.z * 0.0f) * k + intent[0];
        intent[1] = (n.z * 0.0f - n.x * sx) * k + intent[1];
        intent[2] = (n.x * 0.0f - n.y * 0.0f) * k + intent[2];
        intent[2] = k * in.stickY + intent[2];
    } else if (c.u(MOVE_FLAGS) & IN_WATER) {
        float w[3];
        for (int i = 0; i < 3; i++)
            w[i] = (sx * in.camera[0][i] + in.camera[1][i] * 0.0f) + in.stickY * in.camera[2][i];
        add(w, step);
    } else if (!gliderSpent(c)) {
        const float sy = -in.stickY;
        float w[3];
        for (int i = 0; i < 3; i++)
            w[i] = (sy * y.r[1][i] + sx * y.r[0][i]) + y.r[2][i] * 0.0f;
        add(w, step);
    }
}

// Spec 3.9: a flat roll the way the player wants to go, with a small hop.
void dodgeRoll(Rec c) {
    c.b(ABILITY) = 0;
    float x = c.f(INTENT), yv = c.f(INTENT + 4);
    const float k = 1.0f / std::sqrt((x * x + yv * yv) + 0.0f);
    x = x * k, yv = yv * k;
    const float t = k * 0.0f;
    c.f(VELOCITY) = x * DODGE_SPEED + 0.0f;
    c.f(VELOCITY + 8) = t * DODGE_SPEED + DODGE_HOP;
    c.f(VELOCITY + 4) = yv * DODGE_SPEED + 0.0f;
    c.f(DODGE_TIMER) = DODGE_TIME;
    c.f(STAMINA) = c.f(STAMINA) - DODGE_STAMINA;
}

// Spec 3.10 step 3: off the wall, away from it, facing the other way.
void wallJump(Rec c) {
    c.b(ABILITY) = 0;
    if (!(c.u(MOVE_FLAGS) & WALL_JUMP_CONTACT))
        return;
    const Vec3 n = vec(c, WALL_NORMAL);
    const Vec3 v = {n.x * WALL_JUMP_SPEED, n.y * WALL_JUMP_SPEED, n.z * WALL_JUMP_SPEED};
    setVec(c, VELOCITY, v);
    setVec(c, INTENT, v);
    setVec(c, PUSH, v);
    c.f(DODGE_TIMER) = DODGE_TIME;
    c.f(YAW) = turnToward(c.f(YAW), c.f(YAW) + WALL_JUMP_TURN, 1.0f);
    c.f(STAMINA) = c.f(STAMINA) - WALL_JUMP_STAMINA; // more than a full bar: always empties it
    if (c.f(STAMINA) <= 0.0f)
        c.f(STAMINA) = 0.0f;
}

// Spec 3.10 step 5.1: leaving the ground (or a roll), with the class key's higher jumps.
void takeOff(const Input &in, Rec c, float duration, int liftMs, State &st) {
    const int a = c.b(ABILITY);
    c.f(VELOCITY + 8) = (a == MAGE_FLOAT || lowerJump(c, duration)) ? LOW_JUMP : JUMP;
    if (in.classSkill) {
        const int cls = c.b(CLASS), spec = c.b(SPECIALISATION);
        if (c.f(STAMINA) >= DODGE_STAMINA && (cls == 4 || cls == 2) && spec == 1) { // the double jump
            c.f(DODGE_TIMER) = DODGE_TIME;
            c.f(VELOCITY + 8) = HIGH_JUMP;
            c.f(STAMINA) = c.f(STAMINA) - DODGE_STAMINA;
        }
        if (c.b(ABILITY) != WARRIOR_LEAP && c.f(STAMINA) >= DODGE_STAMINA && cls == 1) {
            c.b(ABILITY) = WARRIOR_LEAP;
            c.f(ABILITY_TIME) = 0.0f;
            c.f(VELOCITY + 8) = HIGH_JUMP;
            c.f(STAMINA) = c.f(STAMINA) - DODGE_STAMINA;
        }
    }
    st.lift = liftMs;
    setVec(c, PUSH, {});
}

// Spec 3.10 and 3.11.
void jump(const Input &in, State &st, Rec c, double dt, float duration, const World &world) {
    int liftMs = LIFT_MS;
    if (c.b(ABILITY) == RIDING && world.byId) {
        const uint8_t *mount = world.byId(c.l(LINKED));
        uint32_t flags = 0;
        if (mount)
            std::memcpy(&flags, mount + MOUNT_FLAGS, 4);
        if (mount && (flags & FLYING_MOUNT))
            liftMs = FLYING_MOUNT_LIFT_MS;
    }
    const bool teleporting = c.b(ABILITY) == TELEPORT && c.f(ABILITY_TIME) <= duration;
    if (!in.jump || teleporting) {
        st.jumpHeld = 0;
    } else {
        if (c.b(ABILITY) == CLIMBING && !st.lastJump)
            wallJump(c);
        st.jumpHeld = (int)((double)st.jumpHeld + dt);
        if (st.jumpHeld > 0) {
            const bool grounded = (c.u(MOVE_FLAGS) & ON_GROUND) || c.f(DODGE_TIMER) != 0.0f;
            if (st.jumpHeld < TAKE_OFF_WINDOW_MS && grounded && c.f(VELOCITY + 8) <= 0.0f)
                takeOff(in, c, duration, liftMs, st);
            if ((c.u(MOVE_FLAGS) & IN_WATER) && c.f(VELOCITY + 8) < RISE_CAP) { // swimming up
                st.lift = liftMs;
                c.f(VELOCITY + 8) = (float)(dt * WATER_RISE_PER_MS + (double)c.f(VELOCITY + 8));
            }
        }
    }
    // Spec 3.11: holding the key raises the jump for a while.
    if (st.lift > 0 && in.jump && c.b(ABILITY) != MAGE_FLOAT && !lowerJump(c, duration) &&
        c.f(VELOCITY + 8) < RISE_CAP) {
        c.f(VELOCITY + 8) = (float)(dt * HOLD_RISE_PER_MS + (double)c.f(VELOCITY + 8));
        if (!(c.f(VELOCITY + 8) < RISE_CAP))
            c.f(VELOCITY + 8) = RISE_CAP;
    }
}

} // namespace

void control(const Input &in, State &st, uint8_t *player, double dt, const World &world) {
    const Rec c{player};
    const nativerules::HasBuff hasBuff = buffTest(world.hasBuff);
    auto duration = [&] { return (float)nativerules::abilityDuration(player, hasBuff); };

    float step = WALK_STEP;
    if (gliding(c)) {
        if (!steerGlider(c, dt))
            return;
    } else {
        sailOrClear(in, c, step);
    }
    if (gliding(c))
        liftOrDive(in, c, dt);

    if (in.blocked || in.panelShown) // spec 3.4: the keys are not the player's now
        return;

    if (gliding(c))
        step = (float)(dt * GLIDE_STEP_PER_MS);
    const YawBasis yaw = yawBasis((float)in.cameraYaw);
    const bool dodgeWanted = !in.ordinaryUi && !in.typing && in.dodge && (c.u(MOVE_FLAGS) & ON_GROUND) && c.f(DODGE_TIMER) == 0.0f &&
                             c.f(STAMINA) >= DODGE_STAMINA;

    moveKeys(in, c, yaw, step);
    moveStick(in, c, yaw, step);

    if (c.b(ABILITY) == CLEARED_EACH_CALL)
        c.b(ABILITY) = 0;

    if (dodgeWanted && length2(vec(c, INTENT)) > 0.0f && c.f(STAMINA) >= DODGE_STAMINA && c.f(DODGE_TIMER) <= 0.0f)
        dodgeRoll(c);

    jump(in, st, c, dt, duration(), world);

    // Spec 3.12.
    st.lift = (int)((double)st.lift - dt);
    if (st.lift < 0)
        st.lift = 0;
    st.lastJump = in.jump;
}

// --- After the movement routine (spec 4) ---------------------------------------------------

void aimPoint(const Aim &aim, const uint8_t *player, int64_t out[3]) {
    const float distance = aim.wallDistance >= 0.0f ? aim.wallDistance : DEFAULT_AIM_DISTANCE;
    for (int i = 0; i < 3; i++)
        out[i] = aim.rayOrigin[i] + (int64_t)(aim.forward[i] * distance * DOTS_PER_BLOCK);
    if (aim.override || aim.aimDepth < 0.0f || player[ABILITY] == TELEPORT)
        return;
    // Spec 4.5 step 3: onto the nearest hostile creature along the view, if it is ahead.
    int64_t onto[3];
    for (int i = 0; i < 3; i++)
        onto[i] = aim.eye[i] + (int64_t)(aim.forward[i] * aim.aimDepth * DOTS_PER_BLOCK);
    int64_t at[3];
    std::memcpy(at, player + POSITION, sizeof at);
    const Vec3 ahead = blocksBetween(at, onto);
    if (ahead.x * aim.forward[0] + ahead.y * aim.forward[1] + ahead.z * aim.forward[2] > 0.0f)
        std::memcpy(out, onto, sizeof onto);
}

double aimAssist(const AimAssist &a) {
    if (!a.target || !a.player)
        return a.turnYaw;
    float health;
    std::memcpy(&health, a.target + worldgen::creature::HEALTH, 4);
    if (!(health > 0.0f) || !(a.stickX * a.stickX + a.stickY * a.stickY > ASSIST_STICK2))
        return a.turnYaw;
    uint8_t *tp = const_cast<uint8_t *>(a.target);
    const Rec t{tp}, p{const_cast<uint8_t *>(a.player)};
    // Toward the target, a little up its height, led by its velocity.
    Vec3 d = blocksBetween(a.rayOrigin, position(t, RENDER_POSITION));
    d.z = d.z + t.f(TALL) * ASSIST_HEIGHT;
    const float speed = nativerules::isRanged(a.player) ? ASSIST_RANGED_SPEED : ASSIST_SPEED;
    const float lead = std::sqrt(length2(blocksBetween(position(t, RENDER_POSITION), position(p, POSITION)))) / speed;
    const Vec3 v = vec(t, VELOCITY);
    d = {d.x + v.x * lead, d.y + v.y * lead, d.z + v.z * lead};
    const Vec3 f = {a.forward[0], a.forward[1], a.forward[2]};
    // The yaw that would face it.
    float yaw = (float)a.cameraYaw;
    if ((d.x * d.x + d.y * d.y) + 0.0f > 0.0f && (f.x * f.x + f.y * f.y) + 0.0f > 0.0f) {
        const Vec3 dn = normalised({d.x, d.y, 0.0f}), fn = normalised({f.x, f.y, 0.0f});
        yaw = (float)(nativecrt::asin((double)(fn.x * dn.y - fn.y * dn.x)) / PI * 180.0 + (double)yaw);
    }
    // Turn toward it: faster the closer the view already is, and while rolling.
    if (!(length2(d) > 0.0f && length2(f) > 0.0f))
        return a.turnYaw;
    const Vec3 dn = normalised(d), fn = normalised(f);
    const Vec3 cross = {fn.y * dn.z - fn.z * dn.y, fn.z * dn.x - fn.x * dn.z, fn.x * dn.y - fn.y * dn.x};
    const float m = std::sqrt(length2(cross));
    const float rate = p.f(DODGE_TIMER) == 0.0f ? ASSIST_RATE : ASSIST_ROLLING_RATE;
    const float share = (1.0f - nativecrt::expf((float)(-((double)rate * a.dt)))) * nativecrt::expf(m * ASSIST_FALLOFF);
    return (double)turnToward((float)a.turnYaw, yaw, share);
}

namespace {

// The charge leap (spec 4.9): along the look vector at twice its length, at most 50 blocks a
// second, with a hop; +0x9e8 how long it lasts.
void chargeLeap(Rec c) {
    const Vec3 look = vec(c, LOOK);
    Vec3 v = {look.x + look.x, look.y + look.y, look.z + look.z};
    const float l2 = length2(v);
    c.f(CHARGE_LENGTH) = (std::sqrt(l2) / CHARGE_SPEED) * CHARGE_TIME_SCALE;
    if (l2 > CHARGE_SPEED2_CAP) {
        v = normalised(v);
        v = {v.x * CHARGE_SPEED, v.y * CHARGE_SPEED, v.z * CHARGE_SPEED};
    }
    v.z = CHARGE_HOP;
    setVec(c, VELOCITY, v);
}

// The mages' blink (spec 4.11): the way they want to go, or backward from where they look.
void blink(Rec c) {
    const Vec3 intent = vec(c, INTENT);
    if (length2(intent) <= 0.0f) {
        const Vec3 look = vec(c, LOOK);
        setVec(c, PUSH, {look.x * -1.0f, look.y * -1.0f, look.z * -1.0f});
    } else {
        setVec(c, PUSH, intent);
    }
    c.f(DODGE_TIMER) = DODGE_TIME;
    c.f(PUSH + 8) = 0.0f;
    Vec3 push = vec(c, PUSH);
    if (length2(push) > 0.0f) {
        push = normalised(push);
        push = {push.x * BLINK_SPEED, push.y * BLINK_SPEED, BLINK_HOP};
        setVec(c, PUSH, push);
        c.f(VELOCITY) = push.x + c.f(VELOCITY);
        c.f(VELOCITY + 4) = push.y + c.f(VELOCITY + 4);
    }
}

// Spec 4.4: the abilities that last until something ends them.
bool lasting(int a) {
    switch (a) {
    case 0x98: case 0x9c: case 0x9d: case 0x57: case 0x76: case 0x80: case 0x79: case 0x78: case 0x7a: case 0x51:
    case 0x5f: case 0x53: case 0x92: case 0x56: case 0x58: case 0x09: case 0x24: case 0x50: case 0x1d: case 0x1e:
    case 0x25:
        return true;
    default:
        return false;
    }
}

// Spec 4.5 step 1: the weapon's second move (the strong attack's); the first is the AI's
// next-hit picker for every weapon not listed here.
struct Moves {
    int first = 0, second = 0;
};
Moves weaponMoves(nativeai::Creature &player) {
    const Rec c{player.record};
    const bool spec1 = c.b(SPECIALISATION) == 1;
    const uint32_t w = c.u(WEAPON_SUBTYPE), off = c.u(SECOND_HAND_SUBTYPE);
    switch (w) {
    case 10: return {spec1 ? 0x21 : 0x1f, spec1 ? 0x22 : 0x20};
    case 12: return {spec1 ? 0x2b : 0x29, spec1 ? 0x2c : 0x26};
    case 11: return {spec1 ? 0x6b : 0x68, spec1 ? 0x6a : 0x69};
    default: break;
    }
    Moves m;
    m.first = nativeai::nextHit(&player, false);
    if (w == 5) m.second = 6;
    else if (w == 3) m.second = 0x12;
    else if (w == 4) m.second = 0x15;
    else if (w == 6 || off == 6) m.second = 0x1a;
    else if (w == 7 || off == 7) m.second = 0x19;
    else if (w == 8) m.second = 0x1c;
    else if (nativerules::holdsTwoHanded(c.p)) m.second = 0x3c;
    else if (off == 0xd) m.second = 9;
    else m.second = 0x40;
    return m;
}

bool finished(Rec c, const nativerules::HasBuff &hasBuff) {
    return c.f(ABILITY_TIME) > (float)nativerules::abilityDuration(c.p, hasBuff);
}

// Spec 4.9 step 3: a charged move goes off as the strong-attack key is let go. Returns
// false when the ability is not a charged one.
bool releaseCharge(Rec c, const nativerules::HasBuff &hasBuff) {
    auto become = [&](int ability) { c.b(ABILITY) = (uint8_t)ability, c.f(ABILITY_TIME) = 0.0f; };
    auto becomeAfterWindUp = [&](int ability) { // its time starts past the wind-up
        c.b(ABILITY) = (uint8_t)ability;
        c.f(ABILITY_TIME) = (float)nativerules::abilityEffectTime(c.p, -1, hasBuff);
    };
    auto empty = [&] { // nothing charged: the move is dropped
        if (c.f(CHARGED) < CHARGE_EMPTY) {
            c.f(CHARGED) = 0.0f;
            c.b(ABILITY) = 0;
            return true;
        }
        return false;
    };
    switch (c.b(ABILITY)) {
    case GUARD: if (!empty()) become(0x74); return true;
    case 0x19: become(0x18); return true;
    case 0x1a: become(0x38); return true;
    case 0x1c: become(0x1b); return true;
    case 0x24: become(0); return true;
    case 0x25: become(0x26); return true;
    case 0x3c:
        if (!empty()) {
            if (c.b(SPECIALISATION) != 1)
                becomeAfterWindUp(0x3d);
            else
                become(0x3e);
        }
        return true;
    case 0x40: if (!empty()) becomeAfterWindUp(0x0c); return true;
    case 0x41: becomeAfterWindUp(0x11); return true;
    case 0x93: become(0x94); return true;
    default: return false;
    }
}

// Spec 4.10: letting go of the class key ends its stances.
void releaseClassKey(Rec c) {
    const int a = c.b(ABILITY);
    if ((a == SNEAK && length2(vec(c, INTENT)) > 0.0f) || a == SPRINT || a == MAGE_FLOAT || a == 0x92 ||
        (a == WARRIOR_STANCE && c.f(ABILITY_TIME) > WARRIOR_STANCE_LIMIT))
        c.b(ABILITY) = 0;
}

// Spec 4.11 steps 1 to 4: the class key's stances.
void classKeyStances(const Input &in, Rec c, const nativerules::HasBuff &hasBuff) {
    const int cls = c.b(CLASS), spec = c.b(SPECIALISATION);
    auto done = [&] { return finished(c, hasBuff); };
    if ((cls == 4 || cls == 2) && spec == 0 && in.classSkill) { // sneaking
        if (nativerules::skillLevel() > 0 && done() && (c.u(MOVE_FLAGS) & ON_GROUND) && c.b(ABILITY) != INTERCEPT)
            c.b(ABILITY) = SNEAK;
    }
    if (((cls == 4 && spec == 1) || (cls == 2 && spec == 1 && c.b(ABILITY) != GLIDING)) && in.classSkill) { // sprinting
        if (done() && c.b(ABILITY) != INTERCEPT && nativerules::skillLevel() != 0)
            c.b(ABILITY) = SPRINT;
    }
    if (cls == 1 && in.classSkill && c.f(STAMINA) >= DODGE_STAMINA) { // the warriors' stance
        if (done() && c.b(ABILITY) != WARRIOR_STANCE) {
            c.b(ABILITY) = WARRIOR_STANCE;
            c.f(ABILITY_TIME) = 0.0f;
        }
    }
    if (cls == 3 && in.classSkill) { // the mages' float
        if (done() && c.f(DODGE_TIMER) == 0.0f && c.b(ABILITY) != MAGE_FLOAT) {
            c.b(ABILITY) = MAGE_FLOAT;
            const float vz = c.f(VELOCITY + 8);
            c.f(VELOCITY + 8) = vz > MAGE_FLOAT_LOWEST ? vz : MAGE_FLOAT_LOWEST; // (NaN: the lowest)
            setVec(c, PUSH, {});
        }
    }
}

// Spec 4.11 step 5: a standing dodge starts the class's own dodge move.
void dodgeKeyMove(nativeai::Creature &player) {
    const Rec c{player.record};
    const int cls = c.b(CLASS), spec = c.b(SPECIALISATION);
    int move = 0;
    if (cls == 3) move = TELEPORT;
    else if (cls == 1) move = CHARGE;
    else if (cls == 2 && spec == 1) move = RANGER_DODGE;
    else if (cls == 4 && spec == 0) move = ROGUE_DODGE;
    else if (cls == 2 && spec == 0) move = MAGE_BLINK;
    if (move == 0 || !nativeai::canUse(&player, move) || nativerules::skillLevel() == 0)
        return;
    c.b(ABILITY) = (uint8_t)move;
    c.f(ABILITY_TIME) = 0.0f;
    c.f(DODGE_TIMER) = 0.0f;
    if (move == CHARGE)
        chargeLeap(c);
    else if (move == MAGE_BLINK)
        blink(c);
}

} // namespace

void attack(const Input &in, State &st, nativeai::Creature &player) {
    // The ordinary-UI gate precedes all late handlers and their previous-key stores.
    if (in.ordinaryUi)
        return;
    const Rec c{player.record};
    const nativerules::HasBuff hasBuff = buffTest(player);
    const Moves moves = weaponMoves(player);

    // Spec 4.9: holding walk makes the normal attack one hit per press.
    const bool normalWanted = in.attack && !(in.walk && st.lastAttack);
    st.lastAttack = in.attack;
    if (in.strongAttack) {
        if (nativeai::startCharge(&player, in.classSkill))
            c.l(LINKED) = in.chargeTarget;
    } else if (!releaseCharge(c, hasBuff) && normalWanted) {
        if (nativeai::startHit(&player, in.classSkill) && c.b(ABILITY) == CHARGE)
            chargeLeap(c);
    }

    if (!in.classSkill)
        releaseClassKey(c);

    if (!in.attack && !in.strongAttack) {
        classKeyStances(in, c, hasBuff);
        const bool standingDodge = in.dodge && !st.lastDodge && length2(vec(c, INTENT)) < STANDING_DODGE2;
        if (standingDodge)
            dodgeKeyMove(player);
    }
    st.lastSkill = in.classSkill;
    st.lastDodge = in.dodge;

    // Spec 4.12: moves held only while their key is.
    if (!in.strongAttack && moves.second == 0x1d && c.b(ABILITY) == 0x1d)
        c.b(ABILITY) = 0;
    if (!in.attack && moves.first == 0x1e && c.b(ABILITY) == 0x1e)
        c.b(ABILITY) = 0;
}

namespace {

// Spec 4.8 step 2: the pet key mounts the pet, leaves it, or calls it.
void petKey(const Input &in, State &st, nativeai::Creature &player, World &world, const nativerules::HasBuff &hasBuff) {
    const Rec c{player.record};
    if (in.pet && !st.lastPet && finished(c, hasBuff)) {
        const uint8_t *pet = world.byId ? world.byId(c.l(PET)) : nullptr;
        bool near = false;
        if (pet) {
            int64_t at[3];
            std::memcpy(at, pet + POSITION, sizeof at);
            near = length2(blocksBetween(position(c, POSITION), at)) < PET_RANGE2;
        }
        if (c.b(ABILITY) == RIDING_PET) {
            c.b(ABILITY) = 0;
        } else if (near && world.ownsSpecialItem && world.ownsSpecialItem(PET_SPECIAL_ITEM)) {
            c.b(ABILITY) = RIDING_PET;
        } else {
            c.b(HELD_ITEM_TYPE) = CALL_ITEM_TYPE; // the call item held up
            c.b(ABILITY) = CALL_PET;
            c.f(ABILITY_TIME) = 0.0f;
            if (world.playSound)
                world.playSound(CALL_PET_SOUND);
            player.interactions.push_back(nativeinteract::fresh(CALL_PET_KIND));
        }
    }
    st.lastPet = in.pet;
}

// Spec 4.5 steps 4 to 6: beams keep the aim they started with; the look vector.
void aimAndLook(const Input &in, nativeai::Creature &player, double dt, const nativerules::HasBuff &hasBuff) {
    const Rec c{player.record};
    const int a = c.b(ABILITY);
    if (a == IGNORED_0X8C)
        (void)nativerules::abilityDuration(c.p, hasBuff); // (the game asks and drops the answer)
    int64_t aim[3];
    std::memcpy(aim, in.aimAt, sizeof aim);
    if (a == 0x1f || a == 0x20 || a == 0x21 || a == 0x22) {
        if ((double)nativerules::abilityEffectTime(c.p, -1, hasBuff) < (double)c.f(ABILITY_TIME) - dt)
            std::memcpy(aim, &c.l(LOCKED_AIM), sizeof aim); // its first part is over: the aim is kept
        else
            std::memcpy(&c.l(LOCKED_AIM), aim, sizeof aim);
    }
    c.h(STATE_FLAGS) &= (uint16_t)~CLEARED_EACH_FRAME;
    if (in.hasAim)
        setVec(c, LOOK, blocksBetween(position(c, POSITION), aim));
}

} // namespace

bool beginFrame(const Input &in, State &st, nativeai::Creature &player, double dt, World &world) {
    const Rec c{player.record};
    const nativerules::HasBuff hasBuff = world.hasBuff ? world.hasBuff : buffTest(player);
    // The controller's update does not call this during the eagle's flight.
    if (c.b(ABILITY) == EAGLE_A || c.b(ABILITY) == EAGLE_B)
        return false;

    // Spec 2. (Dropping the item held on the mouse cursor, step 1, is the inventory's.)
    if (c.b(ABILITY) == CLIMBING && !(c.u(MOVE_FLAGS) & ON_CLIMBABLE_WALL) && (c.u(MOVE_FLAGS) & (ON_GROUND | IN_WATER)))
        c.b(ABILITY) = 0; // off the wall
    if (!gliding(c) && c.b(ABILITY) != SAILING)
        setVec(c, INTENT, {});
    if (c.f(STUN_TIMER) <= 0.0f && c.f(LOCK_TIMER) <= 0.0f) {
        World movement;
        movement.byId = world.byId;
        movement.hasBuff = hasBuff;
        control(in, st, player.record, dt, movement);
    }

    // Spec 4.1.
    if (in.blocked || in.typing || (gliding(c) && c.f(STAMINA) < GLIDE_STAMINA_FLOOR) ||
        (c.b(ABILITY) == INPUT_LOCK && c.f(ABILITY_TIME) < INPUT_LOCK_MS))
        return false;

    // Spec 4.2: running is the default; the glider "runs" while lifting.
    const bool run = (c.b(ABILITY) == GLIDING && !(c.u(MOVE_FLAGS) & ON_GROUND)) ? (c.f(STAMINA) > 0.0f && in.jump)
                                                                                 : !in.walk;
    if (run)
        c.h(STATE_FLAGS) |= RUNNING;
    else
        c.h(STATE_FLAGS) &= (uint16_t)~RUNNING;

    // Spec 4.3.
    if (in.lamp && !st.lastLamp)
        c.h(STATE_FLAGS) ^= LAMP;
    st.lastLamp = in.lamp;

    // Spec 4.4: an ability long over ends.
    if (!lasting(c.b(ABILITY)) && !in.strongAttack && finished(c, hasBuff) && c.f(ABILITY_TIME) > LONG_FINISHED_MS)
        c.b(ABILITY) = 0;

    if (in.nativeAim) {
        Input prepared = in;
        aimPoint(*in.nativeAim, player.record, prepared.aimAt);
        prepared.hasAim = true;
        aimAndLook(prepared, player, dt, hasBuff);
    } else {
        aimAndLook(in, player, dt, hasBuff);
    }
    specialSkillKey(in, st, player, world);

    return true;
}

void ownerLook(nativeai::Creature &player, const World &world) {
    const Rec c{player.record};
    const nativerules::HasBuff hasBuff = world.hasBuff ? world.hasBuff : buffTest(player);
    // Spec 4.7: looking at the owner.
    if (c.b(ABILITY) == LOOK_AT_OWNER) {
        c.h(STATE_FLAGS) &= (uint16_t)~LOOKING_AT_OWNER;
        const uint8_t *owner = world.byId ? world.byId(c.l(OWNER)) : nullptr;
        if (c.f(ABILITY_TIME) < (float)nativerules::abilityDuration(c.p, hasBuff) && owner) {
            int64_t at[3];
            std::memcpy(at, owner + RENDER_POSITION, sizeof at);
            setVec(c, LOOK, blocksBetween(position(c, POSITION), at));
            c.h(STATE_FLAGS) |= LOOKING_AT_OWNER;
        }
    }
}

void pet(const Input &in, State &st, nativeai::Creature &player, World &world) {
    const nativerules::HasBuff hasBuff = world.hasBuff ? world.hasBuff : buffTest(player);
    petKey(in, st, player, world, hasBuff);
}

void frame(const Input &in, State &st, nativeai::Creature &player, double dt, World &world) {
    if (!beginFrame(in, st, player, dt, world))
        return;
    ownerLook(player, world);
    pet(in, st, player, world);
    attack(in, st, player);
}

// --- The special skill (skills-and-interactions.md 1) ----------------------------------------

const char *skillKey(int ability) {
    static const std::map<int, const char *> keys = {
        {2, "SkillAttack"}, {3, "SkillAttack"}, {6, "SkillPerforate"}, {7, "SkillAttack"}, {8, "SkillAttack"},
        {9, "SkillBlock"}, {10, "SkillAttack"}, {11, "SkillAttack"}, {14, "SkillAttack"}, {15, "SkillAttack"},
        {18, "SkillAmbush"}, {19, "SkillAttack"}, {20, "SkillAttack"}, {21, "SkillKick"}, {22, "SkillRangerKick"},
        {23, "SkillAttack"}, {25, "SkillChargedShot"}, {26, "SkillSalvo"}, {27, "SkillAttack"}, {28, "SkillChargedBoomerang"},
        {31, "SkillFireSwirl"}, {32, "SkillFireVortex"}, {33, "SkillWaterSwirl"}, {34, "SkillWaterVortex"},
        {38, "SkillFireball"}, {39, "SkillFirebolt"}, {40, "SkillFirebolt"}, {41, "SkillFirebolt"}, {42, "SkillWaterbolt"},
        {43, "SkillWaterbolt"}, {44, "SkillWaterball"}, {45, "SkillWaterbolt"}, {47, "SkillFireMissiles"},
        {50, "SkillIntercept"}, {51, "SkillTeleport"}, {52, "SkillRetreat"}, {55, "SkillSmash"}, {58, "SkillAttack"},
        {59, "SkillAttack"}, {60, "SkillCrush"}, {64, "SkillSwirl"}, {68, "SkillAttack"}, {80, "SkillSneak"},
        {81, "SkillSprint"}, {83, "SkillFloat"}, {95, "SkillCyclone"}, {97, "SkillFireExplosion"}, {98, "SkillLavaPuddle"},
        {104, "SkillFireBeam"}, {105, "SkillFireRay"}, {106, "SkillWaterRay"}, {107, "SkillWaterBeam"},
        {108, "SkillShuriken"}, {109, "SkillCamouflage"}, {111, "SkillAim"}, {112, "SkillSwiftness"},
        {113, "SkillBulwark"}, {114, "SkillWarFrenzy"}, {115, "SkillManaShield"}, {117, "SkillPortCity"},
        {134, "SkillHealingStream"}, {135, "SkillHealingStream"}, {136, "SkillFireBreath"}, {140, "SkillAssassinate"},
        {141, "SkillNinjutsu"}, {142, "SkillQuicksandTrap"}, {143, "SkillShadowClone"}, {144, "SkillWaterBubbles"},
        {146, "SkillFlameRush"}, {148, "SkillRockFist"}, {149, "SkillHeroicShout"}, {160, "SkillThrowPoisonVial"},
    };
    const auto it = keys.find(ability);
    return it == keys.end() ? nullptr : it->second;
}

// The special skills on R (0x656e0).
constexpr int ROCK_FIST = 0x94, HEROIC_SHOUT = 0x95;     // warriors
constexpr int SHADOW_CLONE = 0x8f, QUICKSAND_TRAP = 0x8e; // rangers
constexpr int FIRE_MISSILES = 0x2f, WATER_BUBBLES = 0x90; // mages
constexpr int CAMOUFLAGE = 0x6d, NINJUTSU = 0x8d;         // rogues

int specialSkill(const uint8_t *creature) {
    const bool firstSpec = creature[0x14d] == 0;
    switch (creature[0x14c]) {
    case 1: return firstSpec ? ROCK_FIST : HEROIC_SHOUT;
    case 2: return firstSpec ? SHADOW_CLONE : QUICKSAND_TRAP;
    case 3: return firstSpec ? FIRE_MISSILES : WATER_BUBBLES;
    case 4: return firstSpec ? CAMOUFLAGE : NINJUTSU;
    default: return 0;
    }
}

int cooldown(const nativeai::Creature &creature, int ability) {
    // Ninjutsu's buff (type 13) makes intercept and the shuriken nearly free of waiting.
    const bool ninjutsu = nativebuffs::find(creature, 13) != nullptr;
    switch (ability) {
    case 0x16: case 0x34: case 0x61:
        return 12000;
    case 0x2f: case 0x6d: case 0x70: case 0x71: case 0x72: case 0x8e: case 0x90: case 0x92:
        return 40000;
    case 0x32: case 0x6c:
        return ninjutsu ? 1800 : 16000;
    case 0x33: case 0x37: case 0x73:
        return 16000;
    case 0x49:
        return 15000;
    case 0x5f: case 0x62: case 0x88: case 0x95:
        return 30000;
    case 0x6f:
        return 4000;
    case 0x8c: case 0x8d: case 0x8f:
        return 60000;
    case 0x94:
        return 20000;
    default: // every other ability, including all below 0x16 and above 0x95
        return 0;
    }
}

void selfBuff(nativeai::Creature &creature, int ability, std::vector<nativebuffs::Notice> &notices) {
    Rec c{creature.record};
    const int64_t self = c.l(ID);
    auto give = [&](uint8_t type, float strength, float duration, uint8_t noticeFlag) {
        const nativebuffs::Buff b = nativebuffs::make(type, strength, duration, self);
        nativebuffs::give(creature, b, true);
        notices.push_back(nativebuffs::notice(self, self, b, noticeFlag));
    };
    switch (ability) {
    case CAMOUFLAGE: give(3, 0.0f, 20000.0f, 0); break;
    case NINJUTSU: give(13, 0.0f, 20000.0f, 1); break;
    case 0x70: give(12, 2.0f, 10000.0f, 1); break;
    case 0x71:
        give(1, 0.5f, 10000.0f, 1);
        if (c.f(STUN_TIMER) > 0.0f) // it also ends a stun
            c.f(STUN_TIMER) = 0.0f;
        break;
    case 0x72: give(2, 0.0f, 10000.0f, 1); break;
    case 0x73: { // four times the user's spell power (0x65c70 with selector 0)
        const std::vector<nativerules::Buff> stats = nativebuffs::forStats(creature);
        give(6, 4.0f * nativerules::spellPower(c.p, stats.data(), stats.size()), 30000.0f, 1);
        break;
    }
    case 0x92: give(16, 2.0f, 10000.0f, 1); break;
    default: break;
    }
    creature.cooldowns[ability] = (float)cooldown(creature, ability);
}

void specialSkillKey(const Input &in, State &st, nativeai::Creature &player, World &world) {
    Rec c{player.record};
    // Retained Widget focus, active chat and eagle flight skip this handler; a
    // merely shown panel is not focus. Skipped handlers retain their previous byte.
    if (in.blocked || in.typing || c.b(ABILITY) == 0x9c || c.b(ABILITY) == 0x9d)
        return;
    const bool pressed = in.ultimate && !st.lastUltimate;
    st.lastUltimate = in.ultimate;
    const int skill = specialSkill(c.p);
    if (!pressed || skill == 0 || !nativeai::canUse(&player, skill))
        return;
    if (skill == CAMOUFLAGE || skill == NINJUTSU) {
        selfBuff(player, skill, world.buffNotes); // the ability running goes on
    } else {
        c.b(ABILITY) = (uint8_t)skill;
        c.f(ABILITY_TIME) = 0.0f;
        c.f(DODGE_TIMER) = 0.0f; // (the dodge roll)
    }
    // The cooldown of the ability running now: the skill's own, or, for the rogues' buffs,
    // whatever ability was already running, which restarts at its full wait (as the game does).
    const int running = c.b(ABILITY);
    player.cooldowns[running] = (float)cooldown(player, running);
}

} // namespace nativecontrol
