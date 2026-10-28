// Creature looks, written from docs/specs/worldgen-creatures.md §4.2 and
// worldgen-creatures-tables.md T7.
#include "worldgen/Looks.h"

namespace worldgen {
namespace {

using A = Appearance;

// The flags that grow a creature, in the order they apply, and how much (§4.2 step 4).
struct Growth {
    uint32_t flag;
    float factor;
};
constexpr Growth GROWTHS[3] = {{looksflag::BIG, 1.5f}, {looksflag::STRONG, 1.5f}, {looksflag::GIANT, 7.0f}};
constexpr uint32_t GROWTH_FLAGS = looksflag::BIG | looksflag::STRONG | looksflag::GIANT;
constexpr float LARGEST = 8.0f; // no creature is wider than 8 blocks

void setModel(A &a, size_t at, int v) { a.set<uint16_t>(at, (uint16_t)v); }
int model(const A &a, size_t at) { return a.get<uint16_t>(at); }

// A float drawn in single precision: (float)rand x scale / 32767 + offset.
float drawn(Draw rand, float scale, float offset) { return (float)rand() * scale / 32767.0f + offset; }

void setSize(A &a, float x, float y, float z) {
    a.set(A::SCALE, x), a.set(A::SCALE + 4, y), a.set(A::SCALE + 8, z);
}

// The colour bytes (c3, c2, c1) from three draws, low bytes.
void colour(A &a, int c1, int c2, int c3) {
    a.bytes[A::COLOUR] = (uint8_t)c3;
    a.bytes[A::COLOUR + 1] = (uint8_t)c2;
    a.bytes[A::COLOUR + 2] = (uint8_t)c1;
}

// A playable people's look: colour, a head from l2 and hair from l3.
struct HeadAndHair {
    int race;
    bool coloured;
    int heads, headBase; // heads 0: no head written
    int hairs, hairBase; // hairs 0: no hair written
};
constexpr HeadAndHair HEAD_AND_HAIR[] = {
    {0, true, 4, 0x5a3, 10, 0x5d1},   {1, true, 6, 0x5a7, 11, 0x5db},   {2, true, 6, 0x5ad, 15, 0x5b3},
    {3, true, 6, 0x5c2, 9, 0x5c8},    {4, true, 5, 0x97, 6, 0x9c},      {5, true, 5, 0xa2, 6, 0xa7},
    {7, true, 2, 0xae, 6, 0xb0},      {8, true, 5, 0xb6, 6, 0xb0},      {9, true, 5, 399, 3, 0x194},
    {10, true, 5, 0x197, 6, 0x19c},   {11, true, 5, 0x5e6, 8, 0x5f3},   {12, true, 5, 0x5eb, 4, 0x5fd},
    {13, false, 5, 0x601, 5, 0x606},  {14, false, 4, 0x60b, 4, 0x60f},  {15, true, 6, 0x1a5, 6, 0x1ab},
    {16, true, 6, 0x1b1, 6, 0x1b7},   {0x94, true, 3, 0x614, 3, 0x617}, {0x95, true, 3, 0x61c, 3, 0x61f},
    {0xa5, false, 4, 0x1c3, 0, 0},    {0xec, false, 0, 0, 4, 0x626},    {0xed, false, 0, 0, 4, 0x626},
    {0x104, true, 0, 0, 8, 0x5f3},    {0x105, true, 0, 0, 4, 0x5fd},
};

// The races' own cases (T7), after the five common draws. Returns whether the race draws its size.
bool raceCase(int32_t race, Draw rand, A &a, int c1, int c2, int c3, int l2, int l3) {
    for (const HeadAndHair &h : HEAD_AND_HAIR)
        if (h.race == race) {
            if (h.coloured)
                colour(a, c1, c2, c3);
            if (h.heads)
                setModel(a, A::HEAD, l2 % h.heads + h.headBase);
            if (h.hairs)
                setModel(a, A::HAIR, l3 % h.hairs + h.hairBase);
            if (race == 2) // the hands follow the head
                setModel(a, A::HANDS, 0x265 + (model(a, A::HEAD) > 0x5af));
            if (race == 3)
                setModel(a, A::HANDS, 0x265 + (model(a, A::HEAD) > 0x5c4));
            return false;
        }
    switch (race) {
    case 6: {
        const int r1 = rand(), r2 = rand(), r3 = rand();
        colour(a, r1, r2, r3);
        setModel(a, A::HEAD, rand() % 3 + 0xcad);
        return false;
    }
    case 0x2e: {
        const int r1 = rand(), r2 = rand(), r3 = rand();
        colour(a, r1, r2, r3);
        return false;
    }
    case 0x3a: {
        const bool first = rand() % 2 == 0;
        setModel(a, A::HEAD, first ? 0x184 : 0x180);
        setModel(a, A::FEET, first ? 0x185 : 0x181);
        setModel(a, A::CHEST, first ? 0x183 : 0x17f);
        setModel(a, A::WINGS, first ? 0x186 : 0x182);
        return false;
    }
    case 0x53: case 0x54: {
        const int base = race == 0x53 ? 0x118 : 0x11b;
        const int head = rand() % 3 + base;
        setModel(a, A::HEAD, head);
        setModel(a, A::HANDS, 0x120 + (head != base));
        return false;
    }
    case 0xe3: case 0xe4:
        setModel(a, A::HEAD, rand() % 5 + (race == 0xe3 ? 0x122 : 0x127));
        setModel(a, A::HANDS, 0x12e);
        return false;
    case 0x76: setModel(a, A::CHEST, rand() % 2 + 0xb3a); return false;
    case 0x77: {
        setModel(a, A::HEAD, rand() % 5 + 0xb30);
        setModel(a, A::CHEST, rand() % 9 + 0xb24);
        const int limbs = rand() % 3 + 0xb2d;
        setModel(a, A::HANDS, limbs);
        setModel(a, A::FEET, limbs);
        setModel(a, A::TAIL, rand() % 2 + 0xb35);
        a.set(0x28, drawn(rand, 0.1f, 0.4f));
        a.set(0x7c, drawn(rand, 0.5f, -1.0f));
        a.set(0x80, drawn(rand, 1.0f, 3.0f));
        a.set(0x8c, drawn(rand, 1.0f, 3.0f));
        setSize(a, 1.3f, 1.3f, 1.12f * 1.3f);
        return true;
    }
    case 0x7c: setModel(a, A::CHEST, 0xae9 - (rand() % 2 != 0)); return false;
    case 0x8f: setModel(a, A::CHEST, rand() % 4 + 0xb10); return false;
    case 0x91: case 0x92: {
        const float s = drawn(rand, 0.5f, 1.2f) * 0.8f;
        setSize(a, s, s, s);
        return true;
    }
    case 0x9f: {
        const float k = drawn(rand, 1.0f, 6.0f);
        setSize(a, k, k, 1.12f * k);
        return true;
    }
    case 0xa1: {
        const float k = drawn(rand, 0.5f, 4.0f);
        setSize(a, k, k, 1.12f * k);
        return true;
    }
    case 0xca: case 0xcb:
        setModel(a, A::HEAD, rand() % 9 + (race == 0xca ? 0x69 : 0x72));
        setModel(a, A::CHEST, rand() % 9 + 0x7b);
        return false;
    case 0xcc: setModel(a, A::CHEST, rand() % 5 + 0xa4f); return false;
    case 0xcd: setModel(a, A::CHEST, rand() % 5 + 0xa54); return false;
    case 0xce: rand(); return false; // drawn and not used
    case 0xcf: setModel(a, A::CHEST, rand() % 2 + 0xa5a); return false;
    case 0xd0: setModel(a, A::HEAD, rand() % 2 + 0x27); return false;
    case 0xd4: setModel(a, A::CHEST, rand() % 2 + 0xdd7); return false;
    case 0xe7: setModel(a, A::CHEST, rand() % 5 + 0xaef); return false;
    case 0xe9: setModel(a, A::CHEST, rand() % 6 + 0xaf8); return false;
    case 0x121: setModel(a, A::CHEST, rand() % 4 + 0xaf4); return false;
    default: return false;
    }
}

} // namespace

void growSize(float size[3], uint32_t flags) {
    for (const Growth &g : GROWTHS) {
        if (!(flags & g.flag))
            continue;
        for (int i = 0; i < 3; i++)
            size[i] = g.factor * size[i];
        if (size[0] > LARGEST) {
            const float k = LARGEST / size[0];
            for (int i = 0; i < 3; i++)
                size[i] = k * size[i];
        }
    }
}

bool makeLooks(const CreatureTables &tables, int32_t race, bool wild, Draw rand, Appearance &a) {
    // 1. The five common draws.
    const int c1 = rand(), c2 = rand(), c3 = rand();
    const int l2 = rand() % 100, l3 = rand() % 100;
    const Appearance *base = tables.baseAppearance(race);
    if (!base)
        return false;
    // 2. The race's fixed values, then its drawn parts.
    const uint32_t before = a.flags();
    a = *base;
    const uint32_t own = base->flags();
    const bool drawsSize = raceCase(race, rand, a, c1, c2, c3, l2, l3);
    // 3. Flags.
    const uint32_t flags = before | own | (wild ? tables.wildFlags(race) : 0);
    a.setFlags(flags);
    // 4. Growth. A fixed size is stored grown already by the race's own flags; only the flags the
    // caller added grow it further.
    float size[3] = {a.get<float>(A::SCALE), a.get<float>(A::SCALE + 4), a.get<float>(A::SCALE + 8)};
    growSize(size, drawsSize ? flags : (flags & ~own & GROWTH_FLAGS));
    setSize(a, size[0], size[1], size[2]);
    return true;
}

} // namespace worldgen
