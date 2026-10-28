// The creature pose (0x17ab0), written from docs/specs/creature-pose.md, creature-pose-tables.md
// and creature-pose-items.md. One call eases a creature's pose toward what it is doing,
// rebuilds the part matrices kept in its animation state, moves the weapon trails and lists
// what to draw. The steps run in the spec's order (creature-pose.md section 10).
//
// All arithmetic is single precision, one rounded operation at a time, in the order the spec
// gives, unless a comment says "in double": the results must match the game's bit for bit.
// Terms such as "x + 0" or "k * 0" are kept on purpose: they turn -0 into +0 or a
// non-finite value into NaN, as the game's arithmetic does.
#include "NativePose.h"

#include "GameMath.h"
#include "NativeCrt.h"

#include <cmath>
#include <initializer_list>
#include <cstring>

namespace nativepose {
namespace {

// ---------------------------------------------------------------------------------------
// Where things are in a creature record (byte offsets).

constexpr size_t VELOCITY = 0x34;        // f32 x3
constexpr size_t INTENT = 0x40;          // f32 x3: where it wants to go
constexpr size_t LOOK_PITCH = 0x58;      // f32
constexpr size_t MOVE_FLAGS = 0x5c;      // u32: 1 on the ground, 4 at a climbable surface
constexpr size_t HOSTILITY = 0x60;       // u8
constexpr size_t RACE = 0x64;            // i32
constexpr size_t ABILITY = 0x68;         // u8, 0 for none
constexpr size_t ABILITY_TIME = 0x6c;    // f32 ms
constexpr size_t HAIR_COLOUR = 0x7a;     // u8 x3
constexpr size_t LOOKS = 0x80;           // u32 appearance flags
constexpr size_t WIDTH = 0x84, DEPTH = 0x88, HEIGHT = 0x8c; // f32 size
constexpr size_t MODEL_IDS = 0x90;       // i16 x8: head, hair, hands, feet, chest, tail, arm part, wings
constexpr size_t HEAD_SCALE = 0xa0, CHEST_SCALE = 0xa4, HAND_SCALE = 0xa8, FOOT_SCALE = 0xac;
constexpr size_t ARM_SCALE = 0xb0, WEAPON_SCALE = 0xb4, TAIL_SCALE = 0xb8, SHOULDER_SCALE = 0xbc;
constexpr size_t WING_SCALE = 0xc0;
constexpr size_t BODY_PITCH = 0xc4;      // f32
constexpr size_t HAND_TURN = 0xc8;       // f32 x3: the resting hand turn
constexpr size_t FOOT_PITCH = 0xd4, WING_PITCH = 0xd8, TAIL_PITCH = 0xdc;
constexpr size_t CHEST_PLACE = 0xe0, HEAD_PLACE = 0xec, HAND_PLACE = 0xf8, FOOT_PLACE = 0x104;
constexpr size_t TAIL_PLACE = 0x110, WING_PLACE = 0x11c; // f32 x3 each
constexpr size_t STATE_FLAGS = 0x128;    // u16: 1 glove grip, 4 swimming, 0x40 sprinting, 0x400 gliding
constexpr size_t DODGE = 0x12c;          // f32 ms of the roll left
constexpr size_t SITTING = 0x130;        // f32
constexpr size_t CLASS = 0x14c, SPECIALISATION = 0x14d; // u8
constexpr size_t HOLD_OUT = 0x158;       // f32 (state 0x49, the left weapon's fade)
constexpr size_t STANCE = 0x15c, OLD_STANCE = 0x168; // f32 x3 each
constexpr size_t LOOK = 0x174;           // f32 x3: look direction
constexpr size_t RING_HEIGHT = 0x17c;    // f32: race 0x12e's height (the look's z)
constexpr size_t MANA = 0x188;           // f32 (race 0x11d's size)
constexpr size_t RACE_MODE = 0x19c;      // u8
constexpr size_t ITEM_IN_USE = 0x1d8, HEAD_ITEM = 0x278, NECK_ITEM = 0x318, CHEST_ITEM = 0x3b8;
constexpr size_t FEET_ITEM = 0x458, HANDS_ITEM = 0x4f8, SHOULDERS_ITEM = 0x598;
constexpr size_t LEFT_ITEM = 0x638, RIGHT_ITEM = 0x6d8; // items, 0xa0 bytes each
constexpr size_t HEIGHT_OFFSET = 0x990;  // f32 blocks, taken off the drawn height
constexpr size_t FADE = 0x994;           // f32: most draws' fourth number
constexpr size_t STRIDE = 0x998;         // f32 stride length
constexpr size_t STRIDE_PHASE = 0x99c;   // f32 radians
constexpr size_t SWING_COUNT = 0xad8;    // i32
constexpr size_t DRAWN_POSITION = 0xb10; // i64 x3 (1/65536 block)
constexpr size_t DRAWN_PITCH = 0xb34, DRAWN_ROLL = 0xb38, DRAWN_YAW = 0xb3c; // f32
constexpr size_t THROW = 0xba8;          // f32 (state 0x48)

// In an item record.
constexpr size_t ITEM_CATEGORY = 0x00; // u8
constexpr size_t ITEM_KIND = 0x04;     // u32
constexpr size_t ITEM_MATERIAL = 0x1c; // u8

// In the animation state (offsets from creature + ANIM).
constexpr size_t CLOCK = 0x08;          // f32 ms in the pose state
constexpr size_t STATE = 0x0c;          // i32 pose state
constexpr size_t LEFT_SHIFT = 0x20, RIGHT_SHIFT = 0x2c, LEFT_TURN = 0x38, RIGHT_TURN = 0x44;
constexpr size_t BODY_TURN = 0x68, WHOLE_ANGLES = 0x74, BODY_SHIFT = 0x80, HEAD_SHIFT = 0x8c;
constexpr size_t HEAD_ANGLES = 0x98, TAIL_ANGLES = 0xa4, STANCE_SHIFT = 0xb0, SWIM_LEAN = 0xbc;
constexpr size_t WHOLE_SHIFT = 0xc0, KICK = 0xcc, DROP = 0xd0;
constexpr size_t BODY_FRAME = 0x138, HANDS_FRAME = 0x178, CHEST_MATRIX = 0x1b8, HEAD_MATRIX = 0x1f8;
constexpr size_t HAIR_MATRIX = 0x238, LEFT_ARM_MATRIX = 0x278, RIGHT_ARM_MATRIX = 0x2b8;
constexpr size_t RIGHT_WEAPON_MATRIX = 0x2f8, LEFT_WEAPON_MATRIX = 0x338;
constexpr size_t LEFT_HAND_MATRIX = 0x378, RIGHT_HAND_MATRIX = 0x3b8;
constexpr size_t SHOULDERS_MATRIX = 0x3f8, NECK_MATRIX = 0x438;
constexpr size_t LEFT_WING_MATRIX = 0x478, RIGHT_WING_MATRIX = 0x4b8;
constexpr size_t LEFT_FOOT_MATRIX = 0x4f8, RIGHT_FOOT_MATRIX = 0x538, TAIL_MATRIX = 0x578;
constexpr size_t SIZE_FACTOR = 0x5b8;   // f32
constexpr size_t RIGHT_TIP_TRAIL = 0x5bc, RIGHT_BASE_TRAIL = 0x67c; // 16 points each
constexpr size_t LEFT_TIP_TRAIL = 0x73c, LEFT_BASE_TRAIL = 0x7fc;
constexpr int TRAIL_POINTS = 16;
constexpr size_t LOOK_EASED = 0x8bc;    // f32 x3
constexpr size_t POSED = 0x8c8;         // u8
constexpr size_t CAMERA = 0x8d0;        // i64 x2

// Model ids the pose asks the world for.
constexpr int QUIVER_MODEL = 0x400, ARROW_MODEL = 0x401, WEAPON_EFFECT_MODEL = 0xa46;
constexpr int SWUNG_MODEL = 0xb04, GLOVE_GRIP_MODEL = 0xd0a, HEAD_PIECE_MODEL = 0xdeb;
constexpr int BIG_HAND_MODEL = 0xe21, SPARK_MODEL_A = 0xe2f, SPARK_MODEL_B = 0xe30;
constexpr int RING_MODEL = 0xe33, EAGLE_MODEL = 0xe35;

constexpr float DOT = 1.0f / 65536.0f; // one position unit, in blocks
constexpr float DEG = 0.0174532924f;   // degrees to radians, as the rotations use it
constexpr double PI = 3.141592653589793;
constexpr double HALF_PI = 1.5707963267948966;

template <typename T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <typename T> void put(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }

// The SSE minimum and maximum with the constant first: a NaN x passes through.
float minK(float k, float x) { return k < x ? k : x; }
float maxK(float k, float x) { return k > x ? k : x; }
float clamp01(float x) { return maxK(0.0f, minK(1.0f, x)); }

// ---------------------------------------------------------------------------------------
// Vectors.

struct Vec3 {
    float x = 0, y = 0, z = 0;
};
Vec3 V(float x, float y, float z) { return {x, y, z}; }
Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 mul(float s, Vec3 v) { return {s * v.x, s * v.y, s * v.z}; }
Vec3 halfSum(Vec3 a, Vec3 b) { return mul(0.5f, add(a, b)); }
// k * a + kk * b, per component.
Vec3 mix(float k, Vec3 a, float kk, Vec3 b) { return {k * a.x + kk * b.x, k * a.y + kk * b.y, k * a.z + kk * b.z}; }
float length2(Vec3 v) { return (v.x * v.x + v.y * v.y) + v.z * v.z; }  // 0x2b650
Vec3 cross(Vec3 a, Vec3 b) {                                            // 0x17380
    return {a.y * b.z - a.z * b.y, b.x * a.z - a.x * b.z, a.x * b.y - b.x * a.y};
}
Vec3 normalised(Vec3 v) { // 0x127f0
    const float k = 1.0f / std::sqrt(length2(v));
    return {v.x * k, v.y * k, v.z * k};
}
Vec3 getVec(const uint8_t *p, size_t at) { return {get<float>(p, at), get<float>(p, at + 4), get<float>(p, at + 8)}; }
void putVec(uint8_t *p, size_t at, Vec3 v) {
    put(p, at, v.x), put(p, at + 4, v.y), put(p, at + 8, v.z);
}

uint32_t xorshift(uint32_t v) { // 0x1777e0
    v ^= v << 13;
    v ^= v >> 17;
    v ^= v << 5;
    return v;
}

// ---------------------------------------------------------------------------------------
// Matrices: 16 floats, column-major; each transform is applied on the right (creature-pose.md
// section 2a).

struct Mat {
    float m[16];

    static Mat identity() { // 0x17800
        Mat r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1;
        return r;
    }
    static Mat load(const uint8_t *p, size_t at) {
        Mat r;
        std::memcpy(r.m, p + at, sizeof r.m);
        return r;
    }
    void store(uint8_t *p, size_t at) const { std::memcpy(p + at, m, sizeof m); }

    void translate(float x, float y, float z) { // 0x2b7a0
        for (int r = 0; r < 4; r++)
            m[12 + r] = ((x * m[r] + y * m[4 + r]) + z * m[8 + r]) + m[12 + r];
    }
    void translate(Vec3 v) { translate(v.x, v.y, v.z); }
    void scale(float x, float y, float z) { // 0x2b580: a factor of exactly 1 leaves its column alone
        const float f[3] = {x, y, z};
        for (int c = 0; c < 3; c++)
            if (f[c] != 1.0f)
                for (int r = 0; r < 4; r++)
                    m[4 * c + r] = f[c] * m[4 * c + r];
    }
    void scale(float k) { scale(k, k, k); }
    // Moves the model's middle to the origin.
    void centre(const Model &model);
    void rotZ(float degrees) { // 0x13050
        const float a = degrees * DEG, c = nativecrt::cosf(a), s = nativecrt::sinf(a);
        for (int r = 0; r < 4; r++) {
            const float p = m[r], q = m[4 + r];
            m[r] = s * q + p * c;
            m[4 + r] = c * q - p * s;
        }
    }
    void rotY(float degrees) { // 0x2b470
        const float a = degrees * DEG, c = nativecrt::cosf(a), s = nativecrt::sinf(a);
        for (int r = 0; r < 4; r++) {
            const float p = m[r], q = m[8 + r];
            m[r] = p * c - s * q;
            m[8 + r] = c * q + p * s;
        }
    }
    void rotX(float degrees) { // 0x2b360
        const float a = degrees * DEG, c = nativecrt::cosf(a), s = nativecrt::sinf(a);
        for (int r = 0; r < 4; r++) {
            const float p = m[4 + r], q = m[8 + r];
            m[4 + r] = s * q + p * c;
            m[8 + r] = c * q - p * s;
        }
    }
    // A rotation about an axis (0x2b160), done in full even by 0 degrees.
    void rotAxis(float degrees, float x, float y, float z) {
        const float len = std::sqrt((x * x + y * y) + z * z);
        if (len == 0.0f)
            return;
        x = x / len, y = y / len, z = z / len;
        const float a = degrees * DEG, c = nativecrt::cosf(a), s = nativecrt::sinf(a), k = 1.0f - c;
        float R[16] = {};
        R[0] = (x * x) * k + c;
        R[1] = (x * y) * k + s * z;
        R[2] = (x * z) * k - s * y;
        R[4] = (x * y) * k - s * z;
        R[5] = (y * y) * k + c;
        R[6] = (y * z) * k + s * x;
        R[8] = (x * z) * k + s * y;
        R[9] = (y * z) * k - s * x;
        R[10] = (z * z) * k + c;
        R[15] = 1;
        const Mat old = *this; // 0x173e0
        for (int j = 0; j < 4; j++)
            for (int r = 0; r < 4; r++)
                m[4 * j + r] = ((old.m[r] * R[4 * j] + old.m[4 + r] * R[4 * j + 1]) + old.m[8 + r] * R[4 * j + 2]) +
                               old.m[12 + r] * R[4 * j + 3];
    }
    void rotAxis(float degrees, Vec3 axis) { rotAxis(degrees, axis.x, axis.y, axis.z); }
    // Where the matrix takes a point (0x2b680).
    Vec3 point(Vec3 v) const {
        const float w = 1.0f / (((v.x * m[3] + v.y * m[7]) + v.z * m[11]) + m[15]);
        return {w * (((v.x * m[0] + v.y * m[4]) + v.z * m[8]) + m[12]),
                w * (((v.x * m[1] + v.y * m[5]) + v.z * m[9]) + m[13]),
                w * (((v.x * m[2] + v.y * m[6]) + v.z * m[10]) + m[14])};
    }
};

float sizeX(const Model *m) { return m ? (float)m->size[0] : 0.0f; }
float sizeY(const Model *m) { return m ? (float)m->size[1] : 0.0f; }
float sizeZ(const Model *m) { return m ? (float)m->size[2] : 0.0f; }

void Mat::centre(const Model &model) {
    translate((float)model.size[0] * -0.5f, (float)model.size[1] * -0.5f, (float)model.size[2] * -0.5f);
}

// Eases an angle a toward b by s (0x17840); nearly opposite angles jump to b.
float easeAngle(float a, float b, float s) {
    const float ra = (float)((double)(a / 180.0f) * PI), rb = (float)((double)(b / 180.0f) * PI);
    const float ca = nativecrt::cosf(ra), sa = nativecrt::sinf(ra);
    const float cb = nativecrt::cosf(rb), sb = nativecrt::sinf(rb);
    if (!((double)cb * (double)ca + (double)sb * (double)sa >= -0.8999999761581421))
        return b;
    float d = (float)((double)sb * (double)ca - (double)cb * (double)sa);
    d = maxK(-1.0f, minK(1.0f, d));
    return (float)((double)nativecrt::asinf(d) * (((double)s * 180.0) / PI)) + a;
}
float easeTo(float value, float target, float s) { return (target - value) * s + value; }
Vec3 easeTo(Vec3 v, Vec3 target, float s) {
    return {easeTo(v.x, target.x, s), easeTo(v.y, target.y, s), easeTo(v.z, target.z, s)};
}

// ---------------------------------------------------------------------------------------
// Items (creature-pose-items.md section 8).

uint8_t category(const uint8_t *item) { return item[ITEM_CATEGORY]; }
uint32_t kind(const uint8_t *item) { return get<uint32_t>(item, ITEM_KIND); }

// A weapon held in both hands (category 3, the kinds of mask 0x878de0).
bool twoHanded(const uint8_t *item) {
    const uint32_t k = kind(item);
    return category(item) == 3 && k < 24 && ((0x878de0u >> k) & 1);
}
// Category 3 and not one of the kinds 0x16 and 0x17 (a bow's arrow is shown in the other hand).
bool takesArrow(const uint8_t *item) { return category(item) == 3 && kind(item) - 0x16u > 1; }

float itemGlow(const uint8_t *item) { // 0x10a420
    const uint8_t c = category(item), m = item[ITEM_MATERIAL];
    const uint32_t k = kind(item);
    if (c == 0)
        return 0;
    if (m == 1 || m == 0xb || m == 0xc || m == 0xd || m == 0x17)
        return 1;
    if (m == 0x1d || m == 4)
        return 0.5f;
    if (m == 0x21 || m == 0x23)
        return 0.25f;
    if (c == 0x18 && (k == 4 || k == 5 || k == 7 || k == 8 || k == 0xa || k == 0x12))
        return 1;
    if (c == 0x17)
        return 1;
    return 0;
}

float itemSize(const uint8_t *item) { // 0x10a330
    const uint8_t c = category(item);
    const uint32_t k = kind(item);
    if (c == 0xb && k == 0x24)
        return 0.05f;
    if (c == 0xb && (k == 0 || (k >= 0x20 && k <= 0x23)))
        return 0.15f;
    if (c == 0xc)
        return 0.04f;
    if (c == 0x18 && (k == 1 || k == 2))
        return 0.15f;
    if (c == 0xb && k == 0x2a)
        return 0.15f;
    return 0.0714285746f;
}

// The rider's extra shift by the race ridden (0x65040): y and z; x is always 0.
struct MountOffset {
    int race;
    float y, z;
};
constexpr MountOffset MOUNT_OFFSETS[] = {
    {0x125, 0, -0.4f}, {0x126, 0, -0.4f}, {0x1a, 0, -0.9f},  {0x1e, 0, -0.9f},  {0x21, 0, -0.2f},
    {0x4a, 0, -0.3f},  {0xc8, 0, -0.3f},  {0xc9, 0, -0.3f},  {0x4b, 0, 0.1f},   {0x117, 0, 0.1f},
    {0x97, 0, -0.35f}, {0xd2, -0.3f, -0.3f}, {0xd3, 0, -0.6f}, {0xd5, 0, 0.25f}, {0xd7, 0, 0.4f},
    {0x127, 0, 0.4f},  {0x106, 0, -1.0f}, {0x107, 0, -2.0f}, {0x10e, 0, -1.6f},
};
Vec3 mountOffset(int race) {
    for (const MountOffset &o : MOUNT_OFFSETS)
        if (o.race == race)
            return {0, o.y, o.z};
    return {0, 0, 0};
}

// Abilities during which weapons are carried on the back (creature-pose-items.md 3.1).
bool weaponsAway(int ability) {
    static const uint8_t AWAY[] = {0,    1,    0x16, 0x34, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
                                   0x58, 0x59, 0x5a, 0x5b, 0x5d, 0x5e, 0x64, 0x6c, 0x75, 0x76, 0x77, 0x78,
                                   0x79, 0x7a, 0x7c, 0x7d, 0x80, 0x81, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x98,
                                   0x9a, 0x9b, 0x9c, 0x9d, 0xa0, 0xa1, 0xa2, 0xa3};
    for (uint8_t a : AWAY)
        if (a == ability)
            return true;
    return false;
}

// The trail states (creature-pose-items.md 6.3).
bool inList(int v, std::initializer_list<int> list) {
    for (int x : list)
        if (x == v)
            return true;
    return false;
}

// ---------------------------------------------------------------------------------------
// One call of the pose.

struct Sprites {
    const Model *head = nullptr, *hair = nullptr, *armPart = nullptr, *hands = nullptr, *wings = nullptr;
    const Model *tail = nullptr, *feet = nullptr, *chest = nullptr;
    const Model *rightWeapon = nullptr, *leftWeapon = nullptr, *shoulders = nullptr, *neck = nullptr;
    const Model *headItem = nullptr, *shield = nullptr;
};

class Poser {
public:
    Poser(uint8_t *creature, const Call &call, const Models &models, std::vector<Draw> *draws)
        : c(creature), a(creature + ANIM), call(call), models(models), draws(draws) {}
    void run();

private:
    uint8_t *c, *a; // the creature record and its animation state
    const Call &call;
    const Models &models;
    std::vector<Draw> *draws;

    // The rules' values (creature-pose.md 1.2).
    int T = 0, d1 = 0, d2 = 0, d3 = 0, U = 0;
    bool buffE = false;
    float t = 0, c6c = 0; // the state clock and the ability clock
    int ability = 0, race = 0;
    uint32_t looks = 0, moveFlags = 0;
    uint16_t stateFlags = 0;

    Sprites sprite;

    // Appearance values (4.1).
    float headScale = 0, chestScale = 0, handScale = 0, footScale = 0, armScale = 0, weaponScale = 0;
    float tailScale = 0, shoulderScale = 0, heldScale = 0;
    Vec3 Cp, Hp;   // chest and head places
    Vec3 RTr, LTr; // resting hand turns

    // The walk swing and what derives from it (4.2, 4.3).
    float phase = 0, swing = 0, baseSwing = 0;
    bool climbing = false, running = false;
    float splay = 0, legL = 0, legR = 0, liftL = 0, liftR = 0;
    bool turnOut = false;

    // Rest places (4.4): hands, arm parts, feet, tail.
    Vec3 Lr, Rr, Lm, Rm, Fl, Fr, Tp;

    // Targets of the pose fields (creature-pose-tables.md), all 0 at the start of a call.
    Vec3 RT, LT, RS, LS, BT, WA, BS, HS, HA, ST, WS;
    float TAx = 0, TAz = 0, LN = 0, KS = 0, DR = 0;
    float rate = 0.01f;
    float k4 = 4; // the kept value of creature-pose.md 5.3

    // After easing (6).
    Vec3 RTurn, LTurn;

    // Frames (8).
    Mat base{}, body{}, hands{}, headFrame{};
    float bob = 0, x9 = 0;
    bool glowAll = false;
    Vec3 back; // the creature's place, to make the trails relative to it

    // --- helpers ---
    float f32(size_t at) const { return get<float>(c, at); }
    Vec3 vec(size_t at) const { return getVec(c, at); }
    float stride() const { return f32(STRIDE); } // state 1 sets it to 0 mid-call
    int state() const { return get<int32_t>(a, STATE); }
    void endState() { put<int32_t>(a, STATE, 0); }
    float animF(size_t at) const { return get<float>(a, at); }
    Vec3 animVec(size_t at) const { return getVec(a, at); }
    const uint8_t *item(size_t slot) const { return c + slot; }
    uint32_t leftKind() const { return kind(item(LEFT_ITEM)); }
    uint32_t rightKind() const { return kind(item(RIGHT_ITEM)); }
    bool oneWeapon() const { return sprite.rightWeapon && !sprite.leftWeapon; }
    bool holdsTwoHander() const { return nativerules::holdsTwoHanded(c); } // 0x5e9b0
    // "X > t": the clock has not reached X ms.
    bool until(int ms) const { return (float)ms > t; }
    float bobble() const { return nativecrt::cosf(t * 0.5f) * 0.5f; }

    const Model *byId(int id) const { return id < 0 || !models.byId ? nullptr : models.byId(id); }
    const Model *forItem(size_t slot) const { return models.forItem ? models.forItem(item(slot)) : nullptr; }
    const Model *worn(size_t slot) const { return nativerules::mayUse(c, item(slot)) ? forItem(slot) : nullptr; }

    void draw(const Model *m, const Mat &M, const float tint[4], float n1, float n2, float n3, float n4, float n5,
              int kind = 0, const uint8_t *forItem = nullptr);
    void drawPart(const Model *m, const Mat &M, float glow, int kind = 0) {
        draw(m, M, call.tint, call.alpha, x9, glow, f32(FADE), call.depth, kind);
    }
    float glow(size_t slot) const { return itemGlow(item(slot)); }

    // The steps, in order.
    void drawRings();
    void pickSprites();
    void readAppearance();
    void walkSwing();
    void derivedValues();
    void restPlaces();
    void glide();
    void stateTargets();
    void commonSteps();
    void easePose();
    void hideParts();
    void sparks();
    void buildBase();
    void buildBody();
    void drawRidden();
    void drawItemInUse(const Mat &oldHands);
    void bodyParts();
    void buildHands();
    void drawBetweenHands();
    void drawQuivers();
    void rightWeapon();
    void leftWeapon();
    void drawShield();
    void weaponTips(const Mat &M, const Model &m, size_t tipTrail, size_t baseTrail);
    void moveTrails();
    void handsAndFeet();

    // The pose states (NativePoseTables.inc).
    struct StateRow {
        int state;
        void (Poser::*entry)();
    };
    static const StateRow STATES[];
    void grip();
    void oneWeaponGrip();
    void standUp(bool leftShieldTurn);
    void ended();
    void circle(bool withBob, float b);
    void bobWhileEating();
    void handsCircling();
    void gather(float k);
    void state1();
    void state2();
    void state3();
    void state4();
    void state5();
    void state6();
    void state7();
    void state8();
    void state9();
    void stateA();
    void stateB();
    void swingF10();
    void blow11_12();
    void states13_14();
    void state15();
    void state16();
    void states17_1a_1c();
    void states18_19();
    void state1b();
    void state1d();
    void state1e();
    void state1f();
    void swings20_31_32();
    void state21();
    void state22();
    void state23();
    void state24();
    void state26();
    void state27();
    void state28();
    void state29();
    void state2a();
    void state2b();
    void state2c();
    void bite2d_50();
    void state2e();
    void state2f();
    void state30();
    void state33();
    void using34_35();
    void state36();
    void state37();
    void sitting38_39();
    void state3a();
    void hanging3b_3c();
    void state3d();
    void state3e();
    void riding3f_40();
    void state41();
    void stirring42_44();
    void state45();
    void state47();
    void state48();
    void state49();
    void state4a();
    void state4b();
    void state4c();
    void state4d();
    void eating4e_4f();
    void state51();
    void state52();
    void state54();
    void state55();
    void state56();
    void circling57_58();
    void state59();
    void busy5a_62();
    void state61();
    void state63();
    void swaying64_66();
};

void Poser::draw(const Model *m, const Mat &M, const float tint[4], float n1, float n2, float n3, float n4, float n5,
                 int kind, const uint8_t *forItem) {
    if (!draws || !m)
        return;
    Draw d;
    d.model = m;
    std::memcpy(d.matrix, M.m, sizeof d.matrix);
    std::memcpy(d.tint, tint, sizeof d.tint);
    d.a = n1, d.b = n2, d.c = n3, d.d = n4, d.e = n5;
    d.kind = kind;
    d.item = forItem;
    draws->push_back(d);
}

const float WHITE[4] = {1, 1, 1, 1};

// Race 0x12e's stack of rings (items 5.1), before anything else.
void Poser::drawRings() {
    if (race != 0x12e || !draws)
        return;
    const Model *ring = byId(RING_MODEL);
    if (!ring)
        return;
    const float height = f32(RING_HEIGHT);
    const int n = (int)(height * 0.5f);
    const float s = 3.0f / sizeZ(ring);
    const int64_t drop = (int64_t)((f32(HEIGHT) * 0.5f) * 65536.0f);
    const float x = (float)(get<int64_t>(c, DRAWN_POSITION) + call.cameraX) * DOT;
    const float y = (float)(get<int64_t>(c, DRAWN_POSITION + 8) + call.cameraY) * DOT;
    const int64_t z0 = get<int64_t>(c, DRAWN_POSITION + 16);
    const float yaw = f32(DRAWN_YAW);
    for (int k = 0; k < n; k++) {
        Mat M = Mat::identity();
        M.translate(x, y, (float)((z0 - drop) + (int64_t)k * 0x20000) * DOT);
        M.rotZ((float)(k * 90) + yaw);
        M.scale(s);
        M.translate(sizeX(ring) * -0.5f, sizeY(ring) * -0.5f, 0);
        draw(ring, M, WHITE, call.alpha, 0, 0, 0, 0);
    }
    // The last, partial ring.
    const int64_t z = ((int64_t)(n + n) * 65536 - drop) + z0;
    Mat M = Mat::identity();
    M.translate(x, y, (float)z * DOT);
    M.rotZ((float)(n * 90) + yaw);
    M.scale(s * (height * 0.5f - (float)n));
    M.translate(sizeX(ring) * -0.5f, sizeY(ring) * -0.5f, 0);
    draw(ring, M, WHITE, call.alpha, 0, 0, 0, 0);
}

// Section 3: the model of every body part and held weapon.
void Poser::pickSprites() {
    put<uint8_t>(a, POSED, 1);
    put<int64_t>(a, CAMERA, call.cameraX);
    put<int64_t>(a, CAMERA + 8, call.cameraY);
    Sprites &s = sprite;
    s.headItem = worn(HEAD_ITEM);
    s.hands = worn(HANDS_ITEM);
    s.feet = worn(FEET_ITEM);
    s.shoulders = worn(SHOULDERS_ITEM);
    s.neck = worn(NECK_ITEM);
    s.rightWeapon = nativerules::mayUse(c, item(RIGHT_ITEM)) && !twoHanded(item(LEFT_ITEM)) ? forItem(RIGHT_ITEM) : nullptr;
    s.leftWeapon = nativerules::mayUse(c, item(LEFT_ITEM)) && !twoHanded(item(RIGHT_ITEM)) ? forItem(LEFT_ITEM) : nullptr;
    s.chest = worn(CHEST_ITEM);
    // What is still missing comes from the creature's appearance.
    auto appearance = [&](int part) { return byId(get<int16_t>(c, MODEL_IDS + 2 * part)); };
    s.head = appearance(0);
    s.hair = appearance(1);
    if (!s.hands)
        s.hands = appearance(2);
    if (!s.feet)
        s.feet = appearance(3);
    if (!s.chest)
        s.chest = appearance(4);
    s.tail = appearance(5);
    s.wings = appearance(7);
    s.armPart = appearance(6);
}

// 4.1: scales, places and the resting hand turns.
void Poser::readAppearance() {
    headScale = f32(HEAD_SCALE), chestScale = f32(CHEST_SCALE), handScale = f32(HAND_SCALE);
    footScale = f32(FOOT_SCALE), armScale = f32(ARM_SCALE), weaponScale = f32(WEAPON_SCALE);
    tailScale = f32(TAIL_SCALE), shoulderScale = f32(SHOULDER_SCALE);
    heldScale = weaponScale;
    Cp = vec(CHEST_PLACE), Hp = vec(HEAD_PLACE);
    if (state() == 0) {
        const Vec3 turn = vec(HAND_TURN);
        RTr = turn;
        LTr = {turn.x, -turn.y, -turn.z};
    }
}

// 4.2: the walk swing.
void Poser::walkSwing() {
    const int st = state();
    phase = nativecrt::sinf((float)((double)f32(STRIDE_PHASE) + HALF_PI));
    swing = (float)((double)stride() * 2.199114820062152 * (double)phase);
    if (ability == 0x50 || (ability == 0x7a && (moveFlags & 1)))
        swing = swing * 0.5f;
    if (ability == 0x76 || ability == 0x77 || ability == 0x79)
        swing = swing * 0.1f;
    climbing = ability == 0x7a && (moveFlags & 4);
    const Vec3 intent = vec(INTENT), velocity = vec(VELOCITY);
    running = ability == 0x32 || ability == 0x52 ||
              (!climbing && (stateFlags & 0x40) && length2(intent) > 0.1f &&
               velocity.x * velocity.x + velocity.y * velocity.y > 0.1f);
    if (running)
        swing = swing * 1.25f;
    if ((looks & 2) && !(moveFlags & 1))
        swing = 0;
    if (looks & 1)
        swing = swing * 0.6f;
    baseSwing = swing;
    if (rightKind() == 5 && ability != 0)
        swing = swing * 0.25f;
    if ((st == 0x34 || st == 0x35) && (float)U > c6c)
        swing = swing * 0.1f;
    if (st == 0x36 || st == 0x51)
        swing = swing * 0.1f;
    if (((st >= 0x5a && st <= 0x60) || st == 0x62) && 5000.0f > t)
        swing = swing * 0.1f;
    if (inList(ability, {9, 0x17, 0x1d, 0x25, 0x40}) || inList(st, {2, 3, 0x26, 0x31, 0x32}))
        swing = swing * 0.5f;
    if (ability == 0x78)
        swing = 0;
}

// 4.3: the board's drop, the swim lean, the legs.
void Poser::derivedValues() {
    const int st = state();
    if (ability == 0x78 && !(moveFlags & 1) && f32(SITTING) <= 0 && f32(DODGE) <= 0) // 0x5ee70
        DR = f32(HEIGHT) * 0.5f;
    if (stateFlags & 4) { // swimming: lean into the turn
        float ix = f32(INTENT), iy = f32(INTENT + 4), fx = f32(LOOK), fy = f32(LOOK + 4);
        const float v2 = ix * ix + iy * iy;
        if (v2 > 5.0f) {
            const float f2 = fx * fx + fy * fy;
            if (f2 > 0.0f) {
                const float kv = 1.0f / std::sqrt(v2);
                ix = ix * kv, iy = iy * kv;
                const float kf = 1.0f / std::sqrt(f2);
                fx = fx * kf, fy = fy * kf;
                const float d = maxK(-1.0f, minK(1.0f, fx * iy - fy * ix));
                LN = (float)((double)nativecrt::asinf(d) * 57.29577951308232);
                if (fx * ix + fy * iy < -0.2f)
                    LN = -LN;
            }
        }
    }
    splay = (float)((double)((baseSwing * 0.0f) * 80.0f) / PI);
    const float swingDegrees = (float)((double)(baseSwing * 180.0f) / PI);
    legL = swingDegrees * 1.5f;
    legR = (-swingDegrees) * 1.5f;
    const float stride4 = stride() * 4.0f;
    liftR = (1.0f - phase) * stride4;
    liftL = stride4 * phase;
    if ((looks & 2) && !(moveFlags & 1))
        liftL = liftR = 0;
    if (inList(st, {0x3e, 0x3f, 0x40, 0x35}) || ability == 0x9c || ability == 0x9d)
        splay = 10, legL = legR = 40;
    if (st == 0x56)
        legL = 30, legR = -10;
    if (st == 0x38)
        legL = legR = -10;
    if (st == 0x3a)
        legL = legR = -40, splay = 5;
}

// 4.4 and 4.5: where hands, arm parts, feet and tail rest; the shield.
void Poser::restPlaces() {
    const float hx = f32(HAND_PLACE), hy = f32(HAND_PLACE + 4), hz = f32(HAND_PLACE + 8);
    const float s = stride();
    if (looks & 1) { // the hands are front feet
        Lr = {-hx, hy, hz};
        Rr = {hx, hy, hz};
    } else {
        const float r = hx * s + hx;
        Lr.x = -(nativecrt::cosf(swing) * r);
        Rr.x = -Lr.x;
        Lr.y = -(nativecrt::sinf(swing) * r);
        Rr.y = -Lr.y;
        Lr.z = (hz - 4.1f) + s * 6.0f;
        Rr.z = s * 6.0f + (hz - 4.1f);
        Lr.y += hy;
        Rr.y += hy;
        if (climbing) {
            const float k = nativecrt::sinf((float)((double)s * PI * (double)phase)) * 8.0f;
            Lr.z += k;
            Rr.z -= k;
        }
    }
    Lr.y += 2.1f - s * 5.0f;
    Rr.y += 2.1f - s * 5.0f;

    Lm = {(hx * -0.3f + Lr.x) * 0.5f, ((Lr.y - 8.0f) * 0.5f) * 0.5f, (Lr.z + (hz + 12.0f)) * 0.5f};
    Rm = {(Rr.x + hx * 0.3f) * 0.5f, (float)((double)((Rr.y - 8.0f) * 0.5f) * 0.5), (Rr.z + (hz + 12.0f)) * 0.5f};

    const Vec3 foot = vec(FOOT_PLACE);
    Fl = {-foot.x, foot.y, foot.z};
    Fr = foot;
    Tp = vec(TAIL_PLACE);

    if (category(item(LEFT_ITEM)) == 3 && leftKind() == 0xe) { // a shield is carried on the back
        sprite.shield = forItem(LEFT_ITEM);
        sprite.leftWeapon = nullptr;
    }
}

// 4.6: gliding.
void Poser::glide() {
    if (!(stateFlags & 0x400))
        return;
    WA = V(-70, 0, 0);
    HS = V(0, -1, 0);
    WS.z = (f32(DEPTH) - f32(HEIGHT)) * 0.5f;
    RS = V(0, 3, 1);
    LS = V(0, 3, 1);
    HA.x = 70;
    if (rightKind() == 7)
        RS.y = 11;
    legL = legR = -20;
    if (ability != 0 && (float)T > c6c) {
        RS.x += 0, LS.x += 0;
        RS.y += -5, RS.z += 6;
        LS.y += -5, LS.z += 6;
    }
}

// 5.4: what follows every state's entry.
void Poser::commonSteps() {
    constexpr float K90 = 90, K10 = 10, K5 = 5, K2 = 2, K30 = 30;
    if (ability == 0x52)
        legR = K30, turnOut = true;
    if (state() == 0)
        put<float>(a, CLOCK, 0.0f);
    Lm = halfSum(Lr, Lm);
    Rm = halfSum(Rr, Rm);
    if (f32(SITTING) > 0) {
        if (!(looks & 1)) {
            WA = V(K90, 0, 0);
            WS.z = (f32(DEPTH) - f32(HEIGHT)) * 0.5f;
        } else {
            WA = V(0, K90, 0);
            WS.z = (f32(WIDTH) - f32(HEIGHT)) * 0.5f;
        }
    }
    if (climbing) {
        RS.z += 7, LS.z += 7;
        LT.x += K90, RT.x += K90;
    }
    if (leftKind() == 0x14) {
        LS.y += k4, LS.z += k4;
        LT.x = 0;
    }
    if (!(looks & 1) && !inList(ability, {0x76, 0x77, 0x79, 0x80, 0x9c, 0x9d})) { // leaning into a run
        if (running) {
            WA.x += -25;
            HA.x += K10;
            if (ability == 0x51 || ability == 0x52 || ability == 0x92) {
                WA.x += -20, LS.y += -5, RS.y += -5;
                if (ability == 0x52)
                    BT.z = 30;
            }
        } else {
            const Vec3 v = vec(VELOCITY), i = vec(INTENT);
            if (v.x * v.x + v.y * v.y > 0.1f && (v.y * i.y + v.x * i.x) + v.z * i.z > 0.0f)
                WA.x += -10, HA.x += K5;
        }
    }
    const uint8_t hostility = c[HOSTILITY], mode = c[RACE_MODE];
    if (hostility == 8 && race >= 0xf4 && race <= 0xf7) {
        if (mode != 0)
            WS.z = 10;
        else
            WS.x = 0;
    }
    if (race >= 0xb5 && race <= 0xb8)
        HA.x = mode == 2 ? 45.0f : 0.0f;
    if (race == 0xea || race == 0xeb)
        HA.x = mode != 0 ? 45.0f : -45.0f;
    if (hostility == 8 && race >= 0xc2 && race <= 0xc5)
        HA = mode != 0 ? V(0, 0, 120) : V(0, 0, 0);
    if (buffE)
        WS.z = WS.z - f32(HEIGHT) * 0.25f;
    if (ability == 0x7a && (moveFlags & 5) == 1) { // hanging on a ledge
        BS.y += K2, BS.z += -3;
        BT.x = -30;
        LS.y = 2, RS.y = 2;
        LS.x += 1, RS.x += -1;
    }
}

// Section 6: ease the pose fields toward the targets; the final hand places.
void Poser::easePose() {
    const float s = ease(call.dt, rate);
    auto vecField = [&](size_t at, Vec3 target) { putVec(a, at, easeTo(animVec(at), target, s)); };
    auto floatField = [&](size_t at, float target) { put(a, at, easeTo(animF(at), target, s)); };
    auto angleField = [&](size_t at, float target) { put(a, at, easeAngle(animF(at), target, s)); };
    vecField(RIGHT_TURN, RT);
    vecField(LEFT_TURN, LT);
    vecField(RIGHT_SHIFT, RS);
    vecField(LEFT_SHIFT, LS);
    vecField(BODY_TURN, BT);
    vecField(WHOLE_SHIFT, WS);
    floatField(DROP, DR);
    angleField(WHOLE_ANGLES, WA.x), angleField(WHOLE_ANGLES + 4, WA.y), angleField(WHOLE_ANGLES + 8, WA.z);
    angleField(HEAD_ANGLES, HA.x), angleField(HEAD_ANGLES + 4, HA.y), angleField(HEAD_ANGLES + 8, HA.z);
    angleField(TAIL_ANGLES, TAx), angleField(TAIL_ANGLES + 4, 0.0f), angleField(TAIL_ANGLES + 8, TAz);
    floatField(SWIM_LEAN, LN);
    vecField(STANCE_SHIFT, ST);
    floatField(KICK, KS);
    vecField(BODY_SHIFT, BS);
    vecField(HEAD_SHIFT, HS);

    // The look direction, eased for ability 0x1d only.
    if (ability == 0x1d) {
        Vec3 look = vec(LOOK);
        if (length2(look) > 0.0f)
            look = normalised(look);
        putVec(a, LOOK_EASED, easeTo(animVec(LOOK_EASED), look, ease(call.dt, 0.01f)));
    } else
        putVec(a, LOOK_EASED, {0, 0, 0});

    RTurn = add(RTr, animVec(RIGHT_TURN));
    LTurn = add(LTr, animVec(LEFT_TURN));
    Rr = add(Rr, animVec(RIGHT_SHIFT));
    Lr = add(Lr, animVec(LEFT_SHIFT));
    Rm = add(mul(0.4f, animVec(RIGHT_SHIFT)), Rm);
    Lm = add(mul(0.4f, animVec(LEFT_SHIFT)), Lm);
}

// 7.1: parts the call hides.
void Poser::hideParts() {
    const uint32_t hide = call.hide;
    if (hide & 1)
        sprite.head = nullptr;
    if (hide & 2)
        sprite.hair = nullptr;
    if (race == 0x10c && c[RACE_MODE] == 0)
        sprite.hair = nullptr;
    if (hide & 8)
        sprite.chest = nullptr;
    if (hide & 4)
        sprite.shoulders = nullptr;
    if (hide & 0x40)
        sprite.rightWeapon = sprite.leftWeapon = nullptr;
}

// The sparks of state 0x61 (items section 7): they gather round the creature while the state
// lasts and fly off when it ends.
void Poser::sparks() {
    if (!call.sparks)
        return;
    Sparks &list = *call.sparks;
    const bool active = state() == 0x61;
    if (!active && list.empty())
        return;
    if (list.empty() && call.rand)
        for (int n = 0; n < 6; n++) {
            const int r1 = call.rand(), r2 = call.rand();
            list.push_back({((float)r2 * 20.0f) / 32767.0f - 10.0f, ((float)r1 * 20.0f) / 32767.0f - 10.0f, 20.0f});
        }
    const float s = ease(call.dt, 0.0025f);
    const int64_t px = get<int64_t>(c, DRAWN_POSITION), py = get<int64_t>(c, DRAWN_POSITION + 8);
    const int64_t pz = get<int64_t>(c, DRAWN_POSITION + 16);
    uint32_t j = 0;
    for (auto &spark : list) {
        j++;
        Vec3 p = {spark[0], spark[1], spark[2]};
        if (active) {
            const uint32_t ra = xorshift(j + 0x3d6) % 100, rb = xorshift(j) % 100;
            const Vec3 target = {((float)rb / 100.0f - 0.5f) * f32(WIDTH), ((float)ra / 100.0f - 0.5f) * f32(DEPTH),
                                 (((float)ra * 0.25f) / 100.0f + 0.75f) * f32(DEPTH)};
            p = easeTo(p, target, s);
        } else {
            const Vec3 step = mul(0.05f, normalised(p));
            p = {p.x + step.x * call.dt, p.y + step.y * call.dt, p.z + step.z * call.dt};
            if (length2(p) > 400.0f) {
                list.clear();
                return;
            }
        }
        spark = {p.x, p.y, p.z};
        if (!draws)
            continue;
        const bool frameA = (((uint32_t)(int32_t)t + xorshift(j)) / 200) & 1;
        const Model *m = byId(frameA ? SPARK_MODEL_A : SPARK_MODEL_B);
        if (!m)
            continue;
        const int64_t x = (px + (int64_t)(p.x * 65536.0f)) + call.cameraX;
        const int64_t y = (py + (int64_t)(p.y * 65536.0f)) + call.cameraY;
        const int64_t z = ((pz + (int64_t)(p.z * 65536.0f)) - (int64_t)(f32(HEIGHT_OFFSET) * 65536.0f)) +
                          (int64_t)(animF(WHOLE_SHIFT + 8) * 65536.0f);
        Mat M = Mat::identity();
        M.translate((float)x * DOT, (float)y * DOT, (float)z * DOT);
        M.rotZ(((float)(xorshift(j + 0x2667) % 1000) / 1000.0f) * 360.0f);
        M.scale(0.05f);
        M.centre(*m);
        draw(m, M, WHITE, 1, 0, 0, 0, 0);
    }
}

// 8.1, 8.2: the base matrix, at the creature's drawn place, turned and sized.
void Poser::buildBase() {
    const bool rolling = f32(DODGE) != 0.0f &&
                         !(c[CLASS] == 4 && c[SPECIALISATION] == 1 && (float)T > c6c &&
                           (ability == 6 || ability == 0x12 || ability == 0x15));
    const int64_t z = (int64_t)(animF(WHOLE_SHIFT + 8) * 65536.0f) +
                      (get<int64_t>(c, DRAWN_POSITION + 16) - (int64_t)(f32(HEIGHT_OFFSET) * 65536.0f));
    base = Mat::identity();
    base.translate((float)(get<int64_t>(c, DRAWN_POSITION) + call.cameraX) * DOT,
                   (float)(get<int64_t>(c, DRAWN_POSITION + 8) + call.cameraY) * DOT, (float)z * DOT);
    const float raise = (f32(HEIGHT) * 5.0f) / f32(WIDTH);
    if (rolling) {
        float angle = (1.0f - f32(DODGE) / 600.0f) * -360.0f;
        if (ability == 0x6c || ability == 0xa0)
            angle = (c6c / (float)T) * -360.0f;
        const Vec3 up = {0, 0, 1};
        Vec3 axis = cross(vec(VELOCITY), up);
        if (length2(axis) < 0.0001f)
            axis = cross(vec(LOOK), up);
        if (race == 0x65)
            axis = vec(VELOCITY);
        if (length2(axis) > 0.01f)
            base.rotAxis(angle, normalised(axis));
        if (ability != 0x6c)
            Lr.z += raise, Rr.z += raise;
    }
    if (ability == 0x78)
        Lr.z += raise, Rr.z += raise;
    const float drop = animF(DROP);
    base.rotZ(f32(DRAWN_YAW));
    base.translate(0, 0, drop);
    base.rotY(f32(DRAWN_ROLL));
    base.rotX(f32(DRAWN_PITCH));
    base.translate(animF(WHOLE_SHIFT), animF(WHOLE_SHIFT + 4), 0);
    base.rotZ(animF(WHOLE_ANGLES + 8));
    base.rotY(animF(WHOLE_ANGLES + 4));
    base.rotX(animF(WHOLE_ANGLES));
    base.translate(0, 0, -drop);
    float size = f32(DEPTH) > f32(WIDTH) ? f32(DEPTH) : f32(WIDTH);
    size = size / 11.2f;
    if (race == 0x11d)
        size = size * f32(MANA);
    size = size * animF(SIZE_FACTOR);
    base.scale(size);
    // Some races beat with the clock.
    float beat = 0;
    if (race == 0x11d || (race >= 0x25 && race <= 0x28))
        beat = 0.1f;
    else if (race == 0x125 || race == 0x126)
        beat = 0.05f;
    else if (race >= 0xef && race <= 0xf1)
        beat = 0.03f;
    if (beat != 0) {
        const float u = (float)(call.nowMs % 1000) / 1000.0f;
        const float x = nativecrt::cosf((float)((double)(u + u) * PI)) * beat;
        base.scale(1.0f - x, 1.0f - x, x + 1.0f);
    }
}

void Poser::buildBody() {
    bob = std::fabs(baseSwing) * 4.0f;
    if (ability == 0x50)
        bob = bob * 4.0f;
    body = base;
    body.translate(0, 0, bob);
    body.translate(animVec(BODY_SHIFT));
    body.scale(call.scale);
    body.rotZ(animF(BODY_TURN + 8));
    body.rotY(animF(BODY_TURN + 4));
    body.rotX(f32(BODY_PITCH) + animF(BODY_TURN));
    body.store(a, BODY_FRAME);
}

// Items 5.2: the board, raft or eagle under the creature.
void Poser::drawRidden() {
    if (!draws)
        return;
    const float cs2 = chestScale + chestScale;
    auto under = [&](float v) { return add(V(0, 0, (f32(HEIGHT) * v) / f32(WIDTH)), Cp); };
    Mat M = body;
    if (ability == 0x78) {
        const Model *m = forItem(ITEM_IN_USE);
        if (!m)
            return;
        M.translate(under(8));
        M.rotX(-animF(WHOLE_ANGLES));
        M.scale(cs2);
        M.centre(*m);
        draw(m, M, call.tint, call.alpha, x9, 0, 0, call.depth);
    } else if (ability == 0x79 || ability == 0x9c || ability == 0x9d) {
        const bool raft = ability == 0x79;
        const Model *m = raft ? forItem(ITEM_IN_USE) : byId(EAGLE_MODEL);
        if (!m)
            return;
        M.translate(under(raft ? -9.0f : -10.5f));
        M.rotX(-animF(BODY_TURN));
        M.rotZ(-animF(WHOLE_ANGLES + 8));
        M.scale(cs2);
        if (raft)
            M.translate(1, 2, 0);
        M.translate(sizeX(m) * -0.5f, sizeY(m) * -0.5f, 0);
        draw(m, M, call.tint, call.alpha, x9, 0, 0, call.depth);
    }
}

// Items 4.1: what is held while eating, playing or busy, placed in the previous call's hands
// frame (it is drawn before this call rebuilds that frame).
void Poser::drawItemInUse(const Mat &oldHands) {
    if (!draws)
        return;
    const Model *m = forItem(ITEM_IN_USE);
    if (!m)
        return;
    const int st = state();
    Mat M = oldHands;
    const Vec3 between = halfSum(Rr, Lr);
    if (((st == 0x4e || st == 0x4f) && 5000.0f > t) || (st == 0x63 && 300.0f > t)) {
        const float k = itemSize(item(ITEM_IN_USE)) * 12.0f;
        M.translate(add(V(0, 0, 8), between));
        M.rotZ(t * 0.02f);
        M.rotY(45);
        M.rotX(45);
        M.scale(k);
        M.centre(*m);
    } else if (st == 0x5a && 5000.0f > t) {
        M.translate(add(V(0, 0, -1), between));
        M.scale(0.8f);
        M.centre(*m);
    } else if (st == 0x5b && 5000.0f > t) {
        M.translate(add(V(7.5f, 0, 0), Lr));
        M.scale(0.75f);
        M.centre(*m);
    } else if (st == 0x5e && 5000.0f > t) {
        if (looks & 1)
            M.rotAxis(legL, 1, 0, 0);
        M.translate(Rr);
        M.rotZ(RTurn.z);
        M.rotY(RTurn.y);
        M.rotX(RTurn.x);
        M.scale(0.75f);
        M.translate(sizeX(m) * -0.5f, sizeY(m) * -0.5f, -sizeZ(m));
    } else if (st == 0x5f && 5000.0f > t) {
        M.translate(add(V(7.5f, 0, 0), Lr));
        M.scale(0.85f);
        M.centre(*m);
    } else if (st == 0x5d && 5000.0f > t) {
        M.translate(between);
        M.scale(0.75f);
        M.centre(*m);
    } else if (st == 0x62 && 1000.0f > t) {
        M.translate(Rr);
        M.translate(-2.3f, 0.3f, 0.13f);
        M.rotZ(RT.z); // this call's target, not the eased turn
        M.scale(0.75f);
        M.centre(*m);
    } else if (st == 0x5c && 5000.0f > t) {
        M.translate(add(V(0, -7, 0), Lr));
        M.scale(0.85f);
        M.centre(*m);
    } else if (st == 0x60 && 5000.0f > t) {
        M.translate(Rr);
        M.rotZ(RTurn.z);
        M.rotY(RTurn.y);
        M.rotX(RTurn.x);
        M.scale(0.5f);
        M.translate(sizeX(m) * -0.5f, sizeY(m) * -0.5f, -sizeZ(m));
    } else
        return;
    draw(m, M, call.tint, call.alpha, x9, glow(ITEM_IN_USE), 0, call.depth);
}

// 8.3 to 8.6: chest, neck item, shoulders item, tail, head, the piece over the head, hair
// and the head item.
void Poser::bodyParts() {
    const float bodyTurn = (float)((double)((swing * 0.5f) * 180.0f) / PI + (double)(animF(SWIM_LEAN) * 0.5f));
    const float fade = f32(FADE);
    if (const Model *m = sprite.chest) {
        Mat M = body;
        M.translate(Cp);
        M.rotZ(bodyTurn);
        M.scale(chestScale);
        if (race == 0x127) { // it breathes
            const float k = nativecrt::cosf(c6c * 0.01f) * 0.05f;
            M.scale(1.0f - k, 1.0f - k, k + 1.0f);
        }
        M.centre(*m);
        M.store(a, CHEST_MATRIX);
        drawPart(m, M, glowAll ? 1.0f : glow(CHEST_ITEM));
    }
    if (const Model *m = sprite.neck) {
        Mat M = body;
        M.translate(add(V(0, 3, 6.05f), Cp));
        M.rotZ(bodyTurn);
        M.rotX((float)((double)swing * 0.7 * (double)swing * 180.0 / PI));
        M.scale(chestScale);
        M.translate(sizeX(m) * -0.5f, -1.5f, -sizeZ(m));
        M.store(a, NECK_MATRIX);
        drawPart(m, M, glowAll ? 1.0f : glow(NECK_ITEM));
    }
    if (const Model *m = sprite.shoulders) {
        Mat M = body;
        M.translate(0, 0, (stride() + stride()) + 4.0f);
        M.translate(Cp);
        M.rotZ((float)((double)((swing * 0.5f) * 180.0f) / PI));
        M.scale(shoulderScale);
        M.scale(chestScale + 0.1f);
        M.centre(*m);
        M.store(a, SHOULDERS_MATRIX);
        drawPart(m, M, glowAll ? 1.0f : glow(SHOULDERS_ITEM));
    }
    if (const Model *m = sprite.tail) {
        Mat M = body;
        M.translate(Cp);
        M.translate(Tp);
        M.rotX(f32(TAIL_PITCH));
        M.rotZ((float)((double)(swing * -180.0f) / PI));
        M.rotZ(animF(TAIL_ANGLES + 8));
        M.rotY(animF(TAIL_ANGLES + 4));
        M.rotX(animF(TAIL_ANGLES));
        M.scale(tailScale);
        M.scale(chestScale);
        M.translate(sizeX(m) * -0.5f, -sizeY(m), sizeZ(m) * -0.5f);
        M.store(a, TAIL_MATRIX);
        drawPart(m, M, 0);
    }

    // The head (8.4): the head frame is shared with the hair and the head item.
    const bool horned = race >= 0xb5 && race <= 0xb8;                          // 0x5ee10
    const bool special = c[HOSTILITY] == 8 && race >= 0xc2 && race <= 0xc5;    // 0x5edb0
    const Vec3 headShift = animVec(HEAD_SHIFT);
    const Vec3 headAngles = animVec(HEAD_ANGLES);
    const Vec3 bodyTurnNow = animVec(BODY_TURN);
    const Vec3 headAt = {Hp.x + headShift.x, Hp.y + headShift.y, (Hp.z + headShift.z) - bob * 0.5f};
    auto headTurns = [&](Mat &M) {
        M.rotX(-bodyTurnNow.x - f32(BODY_PITCH));
        M.rotY(-bodyTurnNow.y);
        M.rotZ(headAngles.z - bodyTurnNow.z);
        M.rotY(headAngles.y);
        M.rotX((f32(LOOK_PITCH) - f32(DRAWN_PITCH)) + headAngles.x);
        M.translate(0, 0, 3);
    };
    headFrame = body;
    headFrame.translate(headAt);
    headFrame.translate(0, 0, -3);
    Mat headM = headFrame;
    headTurns(headFrame);
    if (const Model *m = sprite.head) {
        const float sx = sizeX(m), sy = sizeY(m), sz = sizeZ(m);
        if (horned)
            headM.translate(0, sy * -0.25f, 0);
        else if (special)
            headM.translate((sx * -0.5f) * headScale, (sy * 0.5f) * headScale, 0);
        headTurns(headM);
        headM.scale(headScale);
        if (horned)
            headM.translate(sx * -0.5f, 0, sz * -0.5f);
        else if (race == 0x65)
            headM.translate(sx * -0.5f, sy * -0.2f, sz * -0.5f);
        else if (special)
            headM.translate(0, -sy, sz * -0.5f);
        else
            headM.centre(*m);
        headM.store(a, HEAD_MATRIX);
        drawPart(m, headM, 0);
    }

    // Items 5.3: the piece over the head of ability 0x8a.
    if (ability == 0x8a && draws)
        if (const Model *m = byId(HEAD_PIECE_MODEL)) {
            Mat M = body;
            M.translate(headAt);
            M.translate(0, 0, -3);
            M.rotY(-bodyTurnNow.y);
            M.rotZ(headAngles.z - bodyTurnNow.z);
            M.rotY(headAngles.y);
            M.translate(0, 0, 3);
            M.scale(4);
            M.translate(sizeX(m) * -0.5f, sizeY(m) * -0.5f, 0);
            drawPart(m, M, 0);
        }

    // 8.5: the hair, in the creature's hair colour.
    if (const Model *m = sprite.hair) {
        Mat M = headFrame;
        M.scale(headScale + 0.001f);
        M.centre(*m);
        M.store(a, HAIR_MATRIX);
        float colour[4];
        for (int i = 0; i < 3; i++)
            colour[i] = call.tint[i] * ((float)c[HAIR_COLOUR + i] / 255.0f);
        colour[3] = call.tint[3] * 1.0f;
        float second = (looks & 0x400) ? 1.0f : 0.0f;
        if (looks & 0x400000) {
            second = -1;
            const float g = (float)c[HAIR_COLOUR + 1];
            for (int i = 0; i < 4; i++)
                colour[i] = (g * call.tint[i]) / 255.0f;
        }
        draw(m, M, colour, call.alpha, second, 0, fade, call.depth + 0.0001f);
    }

    // 8.6: the head item, sized against the hair.
    if (const Model *m = sprite.headItem; m && draws) {
        Mat M = headFrame;
        M.scale(headScale);
        M.translate(sizeX(m) * -0.5f, sizeY(m) * -0.5f, sizeZ(sprite.hair) * -0.5f);
        drawPart(m, M, glow(HEAD_ITEM));
    }
}

// 8.7: the hands frame and the arm parts.
void Poser::buildHands() {
    const int st = state();
    hands = base;
    hands.translate(0, 0, bob);
    hands.translate(animVec(BODY_SHIFT));
    if (!climbing && st != 2 && st != 3 && st != 0x26)
        hands.rotX(f32(LOOK_PITCH) + f32(LOOK_PITCH));
    if (stateFlags & 0x400)
        hands.rotX(60);
    hands.translate(0, 0, -4);
    hands.rotY(animF(STANCE_SHIFT + 4));
    hands.translate(0, 0, 4);
    hands.rotZ(animF(BODY_TURN + 8));
    hands.rotY(animF(BODY_TURN + 4));
    hands.rotX(animF(BODY_TURN));
    hands.store(a, HANDS_FRAME);

    if (const Model *m = sprite.armPart) {
        Mat L = hands;
        L.translate(Lm);
        L.scale(armScale);
        L.centre(*m);
        L.store(a, LEFT_ARM_MATRIX);
        drawPart(m, L, 0);
        Mat R = hands;
        R.translate(Rm);
        R.scale(armScale);
        R.centre(*m);
        R.store(a, RIGHT_ARM_MATRIX);
        drawPart(m, R, 0);
    }
}

// Items 4.2: what is held between the hands.
void Poser::drawBetweenHands() {
    if (!draws)
        return;
    const bool using54 = (ability == 0x54 || ability == 0x55) && (float)U > c6c;
    if (!using54 && ability != 0x56)
        return;
    const Model *m = forItem(ITEM_IN_USE);
    if (!m)
        return;
    Mat M = hands;
    M.translate(halfSum(Rr, Lr));
    M.rotX(LTurn.x);
    if (using54) {
        M.scale(itemSize(item(ITEM_IN_USE)) * (handScale * 12.0f));
        M.centre(*m);
    } else {
        M.scale(handScale);
        M.translate(sizeX(m) * -0.5f, sizeY(m) * -0.5f, 0);
    }
    drawPart(m, M, 0);
}

// Items 3.3: quivers on the back for bows and crossbows.
void Poser::drawQuivers() {
    if (!draws)
        return;
    const Model *quiver = byId(QUIVER_MODEL);
    if (!quiver)
        return;
    const float quarter = (float)((double)((swing * 0.25f) * 180.0f) / PI);
    const struct {
        size_t slot;
        float turn;
    } sides[2] = {{LEFT_ITEM, 45}, {RIGHT_ITEM, -45}};
    for (const auto &side : sides) {
        const uint8_t *it = item(side.slot);
        if (category(it) != 3 || (kind(it) != 6 && kind(it) != 7))
            continue;
        Mat M = body;
        M.rotZ(quarter);
        M.translate(0, -7.1f, 0.1f);
        M.rotAxis(side.turn, 0, 1, 0);
        M.rotAxis(90, 0, 0, 1);
        M.scale(weaponScale + 0.01f);
        M.centre(*quiver);
        drawPart(quiver, M, 0);
    }
}

// Items 3.2: the right weapon, in the hand or on the back.
void Poser::rightWeapon() {
    const Model *m = sprite.rightWeapon;
    if (!m)
        return;
    const uint32_t k = rightKind();
    const float sx = sizeX(m), sy = sizeY(m), sz = sizeZ(m);
    Vec3 offset = {sx * -0.5f, sy * -0.5f, sz * -0.5f};
    Mat M;
    if (!weaponsAway(ability) || k == 4 || k == 0xc || k == 0x15) {
        M = hands;
        M.translate(Rr);
        M.rotZ(RTurn.z);
        M.rotY(RTurn.y);
        M.rotX(RTurn.x);
        M.rotAxis(0, 0, 0, 1);
        M.rotAxis(0, 0, 1, 0);
        M.rotAxis(f32(LOOK_PITCH) * 0.5f + 0.0f, 1, 0, 0);
        M.scale(weaponScale);
        if (k == 0x15) {
            M.rotX(-45);
            M.translate(0, 0, 5);
        }
        if (m == byId(ARROW_MODEL))
            offset = {sx * -0.5f - 1.0f, 0.5f, 1.0f};
        else if (k == 0xa)
            offset.z = -10;
        else if (k == 0xc) {
            M.scale(0.6f);
            offset.y = (float)(1 - m->size[1]);
        } else if (k == 6)
            offset.y = 1.5f - sy;
        else if (k == 7)
            offset.y = 0;
        else if (k == 8)
            offset.y = -2, offset.z = -4;
        else if (k != 4)
            offset.z = -4;
    } else {
        M = body;
        M.rotZ((float)((double)((swing * 0.25f) * 180.0f) / PI));
        M.translate(0, -7.1f, 0.1f);
        M.rotAxis(k == 0xa ? -45.0f : -135.0f, 0, 1, 0);
        M.rotAxis(90, 0, 0, 1);
        M.scale(weaponScale);
    }
    M.translate(offset);
    M.store(a, RIGHT_WEAPON_MATRIX);
    const float g = glowAll ? 1.0f : glow(RIGHT_ITEM);
    drawPart(m, M, g);
    draw(byId(WEAPON_EFFECT_MODEL), M, call.tint, call.alpha, x9, g, f32(FADE), call.depth, 1, item(RIGHT_ITEM));
    weaponTips(M, *m, RIGHT_TIP_TRAIL, RIGHT_BASE_TRAIL);
}

// Items 3.2: the left weapon, in the hand or on the back.
void Poser::leftWeapon() {
    const Model *m = sprite.leftWeapon;
    if (!m)
        return;
    const uint32_t k = leftKind();
    const int st = state();
    const float sx = sizeX(m), sy = sizeY(m), sz = sizeZ(m);
    float fade = f32(FADE);
    if (f32(HOLD_OUT) > 0)
        fade = fade + (nativecrt::cosf(t * 0.01f) + 1.0f) * 0.25f;
    Vec3 offset = {sx * -0.5f, sy * -0.5f, sz * -0.5f};
    Mat M;
    if (!weaponsAway(ability) || k == 4 || k == 0xc || k == 0x14 || k == 0x15) {
        M = hands;
        if (st == 0x4d)
            M.translate(add(V(0, 0, 8), halfSum(Rr, Lr)));
        else
            M.translate(Lr.x + 0.01f, Lr.y + 0.01f, Lr.z + 0.01f);
        M.rotZ(LTurn.z);
        M.rotY(LTurn.y);
        M.rotX(LTurn.x);
        M.rotAxis(0, 0, 0, 1);
        M.rotAxis(0, 0, 1, 0);
        M.rotAxis(f32(LOOK_PITCH) * 0.5f + 0.0f, 1, 0, 0);
        M.scale(heldScale);
        if (m == byId(ARROW_MODEL))
            offset = {1.0f - sx * 0.5f, 0.5f, 1.0f};
        else if (st == 0x4d) {
        } else if (k == 6)
            offset.y = 1.5f - sy;
        else if (k == 7)
            offset.y = 0;
        else if (k == 0xc) {
            M.scale(0.6f);
            offset.y = (float)(1 - m->size[1]);
        } else if (k == 0xd)
            offset.y = 3;
        else if (rightKind() == 8)
            offset.y = -2, offset.z = -4;
        else if (k == 4)
            M.rotY(180);
        else
            offset.z = -4;
    } else {
        M = body;
        M.translate(0.5f, -0.65f, 0.5f);
        M.rotZ((float)((double)((swing * 0.25f) * 180.0f) / PI));
        if (k == 0xd) {
            M.translate(0, -8.2f, 0);
            M.rotAxis(180, 0, 0, 1);
        } else {
            if (k == 6 || k == 7) {
                M.translate(0, -6.2f, 0);
                M.rotAxis(-45, 0, 1, 0);
            } else {
                M.translate(0, -7.2f, 0);
                M.rotAxis(-225, 0, 1, 0);
            }
            M.rotAxis(90, 0, 0, 1);
        }
        M.scale(heldScale);
        offset = {0.001f - sx * 0.5f, 0.001f - sy * 0.5f, 0.001f - sz * 0.5f};
    }
    M.translate(offset);
    M.store(a, LEFT_WEAPON_MATRIX);
    const float g = glowAll ? 1.0f : glow(LEFT_ITEM);
    draw(m, M, call.tint, call.alpha, x9, g, fade, call.depth);
    draw(byId(WEAPON_EFFECT_MODEL), M, call.tint, call.alpha, x9, g, fade, call.depth, 1, item(LEFT_ITEM));
    weaponTips(M, *m, LEFT_TIP_TRAIL, LEFT_BASE_TRAIL);
}

// Items 3.4: the shield on the back.
void Poser::drawShield() {
    const Model *m = sprite.shield;
    if (!m || !draws)
        return;
    Mat M = body;
    M.translate(0.65f, -0.5f, 0.3f);
    M.rotZ((float)((double)((swing * 0.25f) * 180.0f) / PI));
    M.translate(0, -6.2f, 0);
    M.rotAxis(-45, 0, 1, 0);
    M.rotAxis(90, 0, 0, 1);
    M.scale(weaponScale);
    M.centre(*m);
    drawPart(m, M, 0);
}

// Items 6.1: the newest point of a weapon's two trails, its base and its tip.
void Poser::weaponTips(const Mat &M, const Model &m, size_t tipTrail, size_t baseTrail) {
    const float sx = (float)m.size[0], sy = (float)m.size[1], sz = (float)m.size[2];
    putVec(a, baseTrail, M.point({sx * 0.5f, sy * 0.5f, 0}));
    putVec(a, tipTrail, M.point({sx * 0.5f, sy * 0.5f, sz}));
}

// Items 6.2, 6.3: the trails made relative to the creature, then moved along.
void Poser::moveTrails() {
    auto at = [](size_t trail, int i) { return trail + 12 * (size_t)i; };
    auto point = [&](size_t trail, int i) { return animVec(at(trail, i)); };
    auto setPoint = [&](size_t trail, int i, Vec3 v) { putVec(a, at(trail, i), v); };
    auto easeFollow = [&](size_t trail, int i, float s) { // entry i + 1 eases toward entry i
        const Vec3 target = point(trail, i), v = point(trail, i + 1);
        setPoint(trail, i + 1,
                 {s * (target.x - v.x) + v.x, s * (target.y - v.y) + v.y, s * (target.z - v.z) + v.z});
    };
    auto copyFirst = [&](size_t trail, Vec3 v) {
        for (int i = 1; i < TRAIL_POINTS; i++)
            setPoint(trail, i, v);
    };

    back = {(float)(-(get<int64_t>(c, DRAWN_POSITION) + get<int64_t>(a, CAMERA))) * DOT,
            (float)(-(get<int64_t>(c, DRAWN_POSITION + 8) + get<int64_t>(a, CAMERA + 8))) * DOT, 0};
    for (size_t trail : {LEFT_BASE_TRAIL, LEFT_TIP_TRAIL, RIGHT_BASE_TRAIL, RIGHT_TIP_TRAIL})
        setPoint(trail, 0, add(point(trail, 0), back));

    const int st = state();
    const float clock = animF(CLOCK);
    if (st == 0x4b && clock > (float)d1 && (float)(d1 + d2) > clock) {
        // A whirl: the right trails trace a circle round the creature.
        Mat M = Mat::identity();
        const int64_t z = (int64_t)(animF(WHOLE_SHIFT + 8) * 65536.0f) +
                          (get<int64_t>(c, DRAWN_POSITION + 16) - (int64_t)(f32(HEIGHT_OFFSET) * 65536.0f));
        M.translate((float)(get<int64_t>(c, DRAWN_POSITION) + call.cameraX) * DOT,
                    (float)(get<int64_t>(c, DRAWN_POSITION + 8) + call.cameraY) * DOT, (float)z * DOT);
        M.rotZ(f32(DRAWN_YAW));
        const float outer = ((clock - (float)d1) * 20.0f) / (float)(d1 + d2), inner = outer * 0.5f;
        for (int i = 0; i < TRAIL_POINTS; i++) {
            const float angle = (float)((double)i * PI * 0.0625 - HALF_PI);
            const Vec3 dir = {nativecrt::sinf(angle), nativecrt::cosf(angle), 0};
            setPoint(RIGHT_BASE_TRAIL, i, add(M.point(mul(inner, dir)), back));
            setPoint(RIGHT_TIP_TRAIL, i, add(M.point(mul(outer, dir)), back));
        }
    } else if (inList(st, {1, 2, 3, 8, 0xa, 0xc, 0xe, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x24, 0x31, 0x32, 0x41}) &&
               clock > (float)d1 && (float)T > clock) {
        // A swing: every trail follows its weapon.
        const float s = ease(call.dt, 0.05f);
        for (int i = 0; i < TRAIL_POINTS - 1; i++) {
            easeFollow(RIGHT_BASE_TRAIL, i, s);
            easeFollow(RIGHT_TIP_TRAIL, i, s);
            easeFollow(LEFT_BASE_TRAIL, i, s);
            easeFollow(LEFT_TIP_TRAIL, i, s);
        }
    } else if ((st == 0xf || st == 0x11 || st == 0x13) && c6c > (float)d1) {
        // The left weapon swings: only its trails follow.
        const float s = ease(call.dt, 0.05f);
        const Vec3 tip = point(LEFT_TIP_TRAIL, 0), root = point(LEFT_BASE_TRAIL, 0);
        setPoint(LEFT_TIP_TRAIL, 0, add(mul(0.0f, V(tip.x - root.x, tip.y - root.y, tip.z - root.z)), tip));
        const Vec3 rightRoot = point(RIGHT_BASE_TRAIL, 0);
        for (int i = 0; i < TRAIL_POINTS - 1; i++) {
            setPoint(RIGHT_BASE_TRAIL, i + 1, rightRoot);
            setPoint(RIGHT_TIP_TRAIL, i + 1, rightRoot);
            easeFollow(LEFT_BASE_TRAIL, i, s);
            easeFollow(LEFT_TIP_TRAIL, i, s);
        }
    } else if (inList(st, {0x10, 0x12, 0x14, 0x57}) && c6c > (float)d1 && (float)(d1 + d2 + d3 / 2) > c6c) {
        // The right weapon swings: only its trails follow.
        const float s = ease(call.dt, 0.075f);
        const Vec3 tip = point(RIGHT_TIP_TRAIL, 0);
        Vec3 root = point(RIGHT_BASE_TRAIL, 0);
        root = add(mul(0.5f, tip), mul(0.5f, root));
        setPoint(RIGHT_BASE_TRAIL, 0, root);
        setPoint(RIGHT_TIP_TRAIL, 0, add(mul(0.0f, V(tip.x - root.x, tip.y - root.y, tip.z - root.z)), tip));
        const Vec3 leftRoot = point(LEFT_BASE_TRAIL, 0);
        for (int i = 0; i < TRAIL_POINTS - 1; i++) {
            easeFollow(RIGHT_BASE_TRAIL, i, s);
            easeFollow(RIGHT_TIP_TRAIL, i, s);
            setPoint(LEFT_BASE_TRAIL, i + 1, leftRoot);
            setPoint(LEFT_TIP_TRAIL, i + 1, leftRoot);
        }
    } else
        for (size_t trail : {RIGHT_BASE_TRAIL, RIGHT_TIP_TRAIL, LEFT_BASE_TRAIL, LEFT_TIP_TRAIL})
            copyFirst(trail, point(trail, 0));

    if (!sprite.rightWeapon) {
        copyFirst(RIGHT_BASE_TRAIL, point(RIGHT_BASE_TRAIL, 0));
        copyFirst(RIGHT_TIP_TRAIL, point(RIGHT_TIP_TRAIL, 0));
    }
    if (!sprite.leftWeapon) {
        copyFirst(LEFT_BASE_TRAIL, point(LEFT_BASE_TRAIL, 0));
        copyFirst(LEFT_TIP_TRAIL, point(LEFT_TIP_TRAIL, 0));
    }
}

// 8.8, 8.9: hands (and the glove grip), wings and feet.
void Poser::handsAndFeet() {
    const float fade = f32(FADE);
    const bool frontFeet = looks & 1;
    if (const Model *m = sprite.hands) {
        if (!(call.hide & 0x10)) {
            Mat M = hands;
            if (frontFeet)
                M.rotAxis(legR, 1, 0, 0);
            M.translate(Lr);
            M.rotZ(LTurn.z);
            M.rotY(LTurn.y);
            if (frontFeet)
                M.rotAxis(legR, 1, 0, 0);
            M.rotX(LTurn.x);
            M.scale(handScale);
            M.translate(0.0017f - sizeX(m) * 0.5f, 0.0017f - sizeY(m) * 0.5f, 0.0017f - sizeZ(m) * 0.5f);
            M.store(a, LEFT_HAND_MATRIX);
            drawPart(m, M, glow(HANDS_ITEM));
            // Items 5.4: the glove grip.
            if (get<uint16_t>(c, STATE_FLAGS) & 1)
                if (const Model *g = byId(GLOVE_GRIP_MODEL); g && draws) {
                    Mat G = hands;
                    G.translate(Lr.x, Lr.y + 2.0f, Lr.z - 2.0f);
                    G.scale(0.8f);
                    G.translate(sizeX(g) * -0.5f, 1.0f - sizeY(g) * 0.5f, (float)(2 - g->size[2]));
                    draw(g, G, WHITE, call.alpha, 1, 0, fade, call.depth);
                }
        }
        if (!(call.hide & 0x20)) {
            const Model *hm = m;
            float k = handScale;
            if (ability == 0x93 || (ability == 0x94 && (float)T > c6c)) {
                hm = byId(BIG_HAND_MODEL);
                k = handScale * 0.75f;
            }
            Mat M = hands;
            if (frontFeet)
                M.rotAxis(legL, 1, 0, 0);
            M.translate(Rr);
            M.rotZ(RTurn.z);
            M.rotY(RTurn.y);
            if (frontFeet)
                M.rotAxis(legL, 1, 0, 0);
            M.rotX(RTurn.x);
            M.scale(-k, k, k);
            M.translate(0.0023f - sizeX(hm) * 0.5f, 0.0023f - sizeY(hm) * 0.5f, 0.0023f - sizeZ(hm) * 0.5f);
            M.store(a, RIGHT_HAND_MATRIX);
            drawPart(hm, M, 0, 2);
        }
    }

    if (const Model *m = sprite.wings) {
        const float flap = nativecrt::cosf(f32(STRIDE_PHASE) * 0.5f) * (stride() * 90.0f) - 45.0f;
        const Vec3 place = vec(WING_PLACE);
        const float wingScale = f32(WING_SCALE);
        for (int side = 0; side < 2; side++) {
            Mat M = hands;
            M.translate(side == 0 ? -place.x : place.x, place.y, place.z);
            M.rotX(f32(WING_PITCH));
            M.rotY(side == 0 ? flap : -flap);
            M.rotZ(-90);
            M.rotX(90);
            M.scale(wingScale);
            M.translate(sizeX(m) * -0.5f, 0, sizeZ(m) * -0.5f);
            M.store(a, side == 0 ? LEFT_WING_MATRIX : RIGHT_WING_MATRIX);
            drawPart(m, M, 0);
        }
    }

    if (const Model *m = sprite.feet) {
        const float sb = std::fabs(baseSwing), kick = animF(KICK), lean = animF(SWIM_LEAN);
        const float footPitch = f32(FOOT_PITCH);
        if (!(call.hide & 0x80)) {
            Mat M = base;
            M.rotZ(lean);
            M.rotAxis((legL + kick) + footPitch, 1, 0, 0);
            M.rotY(splay);
            M.translate(Fl.x, Fl.y, (sb * 3.0f + Fl.z) + liftL);
            if (ability == 0x50)
                M.rotX(legL * -1.5f);
            M.scale(footScale);
            if (turnOut)
                M.rotZ(60);
            else
                M.rotAxis(legL + kick, 1, 0, 0);
            M.centre(*m);
            M.store(a, LEFT_FOOT_MATRIX);
            drawPart(m, M, glow(FEET_ITEM));
        }
        if (!(call.hide & 0x100)) {
            Mat M = base;
            M.rotZ(lean);
            M.rotAxis(legR + kick, 1, 0, 0);
            M.rotY(-splay);
            M.translate(Fr.x, Fr.y, (sb * 3.0f + Fr.z) + liftR);
            if (ability == 0x50)
                M.rotX(legR * -1.5f);
            M.scale(-footScale, footScale, footScale);
            M.rotAxis((legR + kick) + footPitch, 1, 0, 0);
            M.centre(*m);
            M.store(a, RIGHT_FOOT_MATRIX);
            drawPart(m, M, glow(FEET_ITEM), 2);
        }
    }
}

void Poser::run() {
    const nativerules::HasBuff hasBuff = call.hasBuff ? call.hasBuff : nativerules::HasBuff([](int) { return false; });
    T = nativerules::abilityDuration(c, hasBuff);
    d1 = nativerules::abilityEffectTime(c, -1, hasBuff);
    d2 = nativerules::abilityMiddleTime(c, -1, hasBuff);
    d3 = nativerules::abilityRecoveryTime(c, -1, hasBuff);
    U = nativerules::useTime(item(ITEM_IN_USE));
    buffE = hasBuff(0xe);
    t = animF(CLOCK);
    c6c = f32(ABILITY_TIME);
    ability = c[ABILITY];
    race = get<int32_t>(c, RACE);
    looks = get<uint32_t>(c, LOOKS);
    moveFlags = get<uint32_t>(c, MOVE_FLAGS);
    stateFlags = get<uint16_t>(c, STATE_FLAGS);
    x9 = (looks & 0x400000) ? -1.0f : 0.0f;
    glowAll = (looks & 0x1000000) != 0;

    drawRings();
    pickSprites();
    readAppearance();
    walkSwing();
    derivedValues();
    restPlaces();
    glide();
    stateTargets();
    commonSteps();
    easePose();
    t = animF(CLOCK); // a finished blow reset the clock
    hideParts();
    sparks();
    buildBase();
    buildBody();
    const Mat oldHands = Mat::load(a, HANDS_FRAME);
    drawRidden();
    drawItemInUse(oldHands);
    bodyParts();
    buildHands();
    drawBetweenHands();
    drawQuivers();
    rightWeapon();
    leftWeapon();
    drawShield();
    moveTrails();
    handsAndFeet();
}

#include "NativePoseTables.inc"

} // namespace

float ease(double dt, float rate) { return gamemath::blendFactor(dt, rate); }

void pose(uint8_t *creature, const Call &call, const Models &models, std::vector<Draw> *draws) {
    Poser(creature, call, models, draws).run();
}

} // namespace nativepose
