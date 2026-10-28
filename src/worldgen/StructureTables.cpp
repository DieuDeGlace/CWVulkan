// Part B's per-type tables, written from docs/specs/worldgen-structures-tables.md (T1-T15).
#include "worldgen/StructureTables.h"

namespace worldgen {
namespace {

bool between(int v, int lo, int hi) { return v >= lo && v <= hi; }

// A style colour SC(k, m, a, r, g, b, n) (T11): K = xs(s + k) mod m + a, then each channel
// K + xs(s + salt) mod n, unsigned, converted to float.
Colour styleColour(uint32_t s, uint32_t k, uint32_t m, uint32_t a, uint32_t r, uint32_t g, uint32_t b, uint32_t n) {
    const uint32_t base = xorshift(s + k) % m + a;
    return {(float)(base + xorshift(s + r) % n), (float)(base + xorshift(s + g) % n), (float)(base + xorshift(s + b) % n)};
}

// "three(mr, ar, mg, ag, mb, ab)" of the roof and timber colours (T13, T14).
Colour three(uint32_t s, uint32_t mr, uint32_t ar, uint32_t mg, uint32_t ag, uint32_t mb, uint32_t ab) {
    return {(float)(xorshift(s + 0x20b3) % mr + ar), (float)(xorshift(s + 0x291) % mg + ag),
            (float)(xorshift(s + 0x110e) % mb + ab)};
}

// The patchwork walls of culture 2 (T11).
Colour patchwork(int32_t cx, int32_t cy, uint32_t s) {
    const uint32_t h = random(cellKey(floorDivBy(cx, 3), floorDivBy(cy, 3)));
    const uint32_t sp = h % 3 == 0 ? h / 3 : s;
    const uint32_t a = xorshift(sp + 0x2c4) & 0xff;
    const float g = (float)(xorshift(sp + 0x7b) % 200 + 50) * 0.5f;
    const float r = (float)a * 0.5f;
    const float b = (float)(255 - a) * 0.5f;
    return {r + 120.0f, g + 120.0f, b + 120.0f};
}

// "mossy" and "aged" (T12).
Colour mossy(Colour wall) {
    const Colour q = scaled(wall, 0.25f);
    return {22.5f + q.r, 60.0f + q.g, 22.5f + q.b};
}
Colour aged(Colour wall) {
    const Colour q = scaled(wall, 0.75f);
    return {q.r + 15.0f, q.g + 30.0f, q.b + 15.0f};
}

} // namespace

// --- T2-T6 -----------------------------------------------------------------------------------

int roofKind(int t) {
    if (t == 22 || t == 23 || t == 24 || t == 26)
        return 1;
    if (between(t, 40, 43))
        return 2;
    if (t == 13 || t == 49 || t == 50 || t == 51 || t == 53 || t == 60 || t == 62 || t == 69 || t == 70 || between(t, 71, 75))
        return 3;
    if (between(t, 35, 39))
        return 4;
    if (between(t, 45, 48))
        return 5;
    if (t == 86 || t == 94)
        return 6;
    if (between(t, 76, 80))
        return 7;
    if (between(t, 87, 89))
        return 8;
    if (t == 90 || t == 91)
        return 9;
    if (t == 95)
        return 10;
    if (between(t, 81, 85) || t == 100)
        return 11;
    return 0;
}

int floorHeight(int t) {
    if (between(t, 22, 26) || between(t, 87, 89))
        return 8;
    if (t == 16 || between(t, 35, 39) || t == 49 || t == 50 || t == 51 || t == 53 || t == 57 || t == 61 || between(t, 63, 65) ||
        between(t, 71, 85) || t == 90 || t == 91 || t == 96)
        return 10;
    if (t == 13 || t == 40 || t == 43 || t == 60 || t == 86 || t == 94 || t == 95 || t == 100)
        return 15;
    if (between(t, 45, 47))
        return 25;
    return 0;
}

int buildingHeight(int t) {
    if (t == 16 || t == 35 || t == 36 || t == 37 || t == 39 || t == 49 || t == 50 || t == 51 || t == 53 || t == 57 || t == 61 ||
        between(t, 63, 65) || between(t, 71, 80) || between(t, 82, 85) || t == 90)
        return 10;
    if (t == 13 || t == 60 || t == 86 || t == 94 || t == 95 || t == 100)
        return 15;
    if (t == 23)
        return 16;
    if (t == 69)
        return 18;
    if (t == 38)
        return 20;
    if (t == 40 || t == 41 || t == 42 || t == 45 || t == 46 || t == 70)
        return 25;
    if (t == 91)
        return 30;
    if (t == 81)
        return 35;
    if (t == 43 || t == 47 || t == 96)
        return 60;
    return 8;
}

int mostFloors(int t) {
    if (t == 13 || t == 22 || t == 24 || t == 25 || t == 26 || t == 35 || t == 36 || t == 37 || t == 39 || t == 45 || t == 46 ||
        t == 49 || t == 50 || t == 51 || t == 53 || t == 55 || t == 58 || t == 59 || t == 60 || between(t, 71, 91) || t == 94 ||
        t == 95 || t == 100)
        return 1;
    if (t == 23 || t == 38 || t == 40 || t == 47)
        return 2;
    if (t == 43 || t == 96)
        return 3;
    return 0;
}

int cellReach(int t) {
    if (between(t, 1, 15) || t == 52 || t == 53 || t == 60 || t == 66 || t == 69 || t == 70)
        return 2;
    if (t == 16 || between(t, 22, 25) || between(t, 35, 43) || between(t, 45, 47) || between(t, 49, 51) || t == 54 || t == 55 ||
        t == 57 || t == 61 || t == 62 || between(t, 71, 91) || t == 95 || t == 96 || t == 100)
        return 1;
    return 0;
}

// --- T7 ----------------------------------------------------------------------------------------

bool keepsHeight(int t, uint32_t style) {
    if (t == 52)
        return xorshift2(style + 0x2367) & 1;
    return t == 13 || between(t, 22, 25) || between(t, 35, 40) || t == 43 || t == 47 || t == 49 || t == 53 || t == 60 ||
           between(t, 71, 91) || t == 95 || t == 96 || t == 100;
}

bool hasBattlements(int t, uint32_t style) {
    if (t == 52)
        return ~xorshift2(style + 0x2ee3) & 1;
    return (between(t, 40, 47) && t != 44) || roofKind(t) == 9;
}

bool joinsTower(int a, int b) {
    auto tower = [](int t) { return between(t, 40, 47) && t != 44; };
    return tower(a) && tower(b) && buildingHeight(a) <= buildingHeight(b);
}

bool roofed(int t) {
    switch (t) {
    case 13: case 16: case 25: case 26: case 49: case 50: case 51: case 53: case 60: case 86: case 94: case 95: return false;
    default: return roofKind(t) != 9;
    }
}

bool attachedHouse(int t) {
    const int r = roofKind(t);
    return (r == 1 && t != 26) || r == 8 || r == 2 || r == 7 || t == 25;
}

bool waterLineTest(int t, uint32_t style) {
    return roofKind(t) == 3 && t != 13 && t != 51 && t != 53 && t != 60 && (xorshift2(style + 0xd55) & 1);
}

bool grassyFloor(int t, uint32_t style) { return roofKind(t) == 3 && (xorshift2(style + 0x13c55) & 1); }

bool hasDoor(int t) {
    switch (t) {
    case 24: case 36: case 37: case 46: case 71: case 76: case 81: case 89: case 92: case 100: return true;
    default: return false;
    }
}

bool isHouse(int t) { return between(t, 22, 26) || t == 92; }

bool crateCell(int t) {
    switch (t) {
    case 14: case 15: case 17: case 18: case 21: case 60: case 62: case 100: return true;
    default: return false;
    }
}

bool loreCell(int t) { return t == 13 || t == 37 || t == 55 || t == 69 || t == 70; }

bool fenceFreeNeighbour(int t) { return !between(t, 27, 34) && floorHeight(t) <= 0; }

bool lightObject(int o) {
    switch (o) {
    case 0: case 16: case 63: case 64: case 65: case 66: case 83: case 112: case 113: case 114: case 126: return true;
    default: return false;
    }
}

bool restampedCategory(int c) { return between(c, 2, 9) || c == 15 || c == 22 || c == 24 || c == 26 || c == 30; }

// --- T8 ----------------------------------------------------------------------------------------

ObjectSize objectSize(int o) {
    switch (o) {
    case 0: return {2, 2, 4};
    case 1: case 94: return {4, 4, 4};
    case 2: return {2, 2, 3};
    case 3: case 128: case 129: return {2, 1, 3};
    case 4: case 5: return {4, 1, 4};
    case 6: case 127: return {2, 1, 2};
    case 7: return {2, 1, 4};
    case 21: case 22: return {3, 2, 1};
    case 24: return {3, 3, 1};
    case 28: return {1, 1, 0.5f};
    case 30: case 31: return {3, 0.4f, 0.4f};
    case 32: return {1.65f, 2.1f, 1};
    case 37: case 38: case 100: case 101: return {2, 2, 2};
    case 39: case 102: case 103: case 104: return {2, 2, 1};
    case 42: return {3, 1, 4.5f};
    case 44: return {4, 1, 1};
    case 60: case 61: case 62: return {2, 1, 1};
    case 63: case 64: case 66: return {2, 2, 8};
    case 65: return {1, 1, 4};
    case 83: return {2, 2, 0.5f};
    case 84: case 85: case 86: case 87: return {3, 3, 4};
    case 88: case 93: case 116: case 126: return {3, 3, 2};
    case 89: return {1, 1.5f, 1};
    case 95: case 96: return {2, 2, 0.25f};
    case 98: return {3, 3, 3};
    case 99: return {1.5f, 1.5f, 1};
    case 125: return {3, 1, 5};
    case 130: return {2, 3, 3};
    case 131: case 132: case 133: return {4, 4, 2.5f};
    default: break;
    }
    if (between(o, 8, 16))
        return {1, 0.1f, 1};
    if (between(o, 45, 56))
        return {3, 1, 4.5f};
    if (between(o, 67, 73))
        return {2, 1, 1};
    if (between(o, 118, 124))
        return {1.5f, 1.5f, 2};
    return {1, 1, 1};
}

// --- T9 ----------------------------------------------------------------------------------------

int buildingCulture(uint32_t style, int biome) {
    const uint32_t u = xorshift2(style + 0x2317);
    switch (biome) {
    case 3: case 6: return 3 - (int)(u & 1);
    case 4: return (u & 1) ? 6 : 5;
    case 5: case 12: return 4;
    case 7: return 1;
    case 9: return 5;
    case 10: return 6;
    default: break;
    }
    static constexpr int BY_BITS[4] = {0, 2, 1, 3};
    return BY_BITS[u & 3];
}

// --- T11-T14 -----------------------------------------------------------------------------------

Colour timberColour(const ColourContext &c) {
    if (c.culture == 4)
        return three(c.style, 10, 130, 10, 120, 10, 110);
    if (c.culture == 6)
        return three(c.style, 50, 150, 30, 120, 30, 90);
    return three(c.style, 50, 130, 30, 100, 30, 50);
}

Colour wallColour(int t, int32_t cx, int32_t cy, const ColourContext &c) {
    const uint32_t s = c.style;
    if (between(t, 45, 47))
        return styleColour(s, 0xd6a, 150, 100, 0x18148, 0x108a, 0xbf99, 50);
    switch (t) {
    case 16: return {140, 140, 140};
    case 25: return timberColour(c);
    case 52: return styleColour(s, 0x2c4, 150, 100, 0x14cb, 0x7b, 0xdf, 50);
    case 53: return {240, 250, 260};
    case 55: return {200, 200, 200};
    case 57: case 61: return {135, 130, 140};
    case 63: case 64: case 65: return {160, 80, 10};
    case 66: case 67: case 68: return {128, 128, 140};
    case 94: return {260, 250, 130};
    case 95: return {180, 170, 90};
    case 96: case 97: return {180, 170, 120};
    default: break;
    }
    switch (roofKind(t)) {
    case 1:
        if (t != 26)
            switch (c.culture) {
            case 2: return patchwork(cx, cy, s);
            case 3: case 6: return timberColour(c);
            case 4: return styleColour(s, 0x2c4, 40, 40, 0x14cb, 0x7b, 0xdf, 30);
            case 5:
                return {(float)(xorshift(s + 0x237b) % 30 + 250), (float)(xorshift(s + 0x13c3) % 30 + 240),
                        (float)(xorshift(s + 0x2245) % 30 + 230)};
            default: break;
            }
        return styleColour(s, 0x2c4, 100, 100, 0x14cb, 0x7b, 0xdf, 30);
    case 3: return styleColour(s, 0x2c4, 150, 100, 0x14cb, 0x7b, 0xdf, 50);
    case 4: return {135, 130, 140};
    case 6: return {200, 200, 200};
    case 7: return {200, 100, 20};
    case 8: return {80, 80, 80};
    case 9: return styleColour(s, 0x2c4, 100, 100, 0x14cb, 0x7b, 0xdf, 30);
    case 11: return {180, 170, 90};
    default: return {240, 240, 240};
    }
}

Colour trimColour(int t, int32_t cx, int32_t cy, const ColourContext &c) {
    const uint32_t s = c.style;
    const Colour wall = wallColour(t, cx, cy, c);
    if (between(t, 45, 47)) {
        switch (xorshift(s + 0x38e2b) % 3) {
        case 1: return mossy(wall);
        case 2: return styleColour(s, 0x2385, 150, 100, 0x89a, 0x158, 0x933, 50);
        default: return wall;
        }
    }
    const int roof = roofKind(t);
    if (roof == 1 || roof == 9) {
        switch (c.culture) {
        case 2: case 3: return scaled(wall, 0.8f);
        case 4: return styleColour(s, 0x11240, 40, 40, 0x18d0, 0x7b, 0x14a, 30);
        case 5:
            return {(float)(xorshift(s + 0x15a9c) % 30 + 250), (float)(xorshift(s + 0x17845) % 30 + 240),
                    (float)(xorshift(s + 0xf8b0) % 30 + 230)};
        case 6: return aged(wall);
        default: return styleColour(s, 0x1a925, 100, 100, 0x18d0, 0x15a9e, 0x127, 30);
        }
    }
    if (roof == 3 || t == 52) {
        switch (xorshift(s + 0x9167) % 3) {
        case 1: return roof == 3 ? aged(wall) : mossy(wall);
        case 2: return styleColour(s, 0x1a0b, 150, 100, 0x18a93, 0x7fc, 0x1538, 50);
        default: return wall;
        }
    }
    return wall;
}

Colour roofColour(int t, int32_t cx, int32_t cy, const ColourContext &c) {
    const uint32_t s = c.style;
    switch (t) {
    case 25: return timberColour(c);
    case 47: return {50, 50, 220};
    case 52:
        return {(float)(xorshift(s + 0xf8d) % 200 + 50), (float)(xorshift(s + 0xd0d) % 200 + 50),
                (float)(xorshift(s + 0x16c9) % 200 + 50)};
    case 55: return {200, 200, 200};
    case 63: case 64: case 65: return {160, 80, 10};
    case 94: return {260, 250, 130};
    case 95: return {180, 170, 90};
    case 96: case 97: return {120, 110, 90};
    default: break;
    }
    switch (roofKind(t)) {
    case 1: case 9:
        switch (c.culture) {
        case 3: return scaled(timberColour(c), 0.7f);
        case 4: return three(s, 20, 50, 20, 50, 20, 50);
        case 5: return wallColour(t, cx, cy, c);
        case 6: return three(s, 50, 200, 100, 100, 30, 50);
        default: break;
        }
        switch (xorshift(s + 0x1bfb) % 3) {
        case 1: return three(s, 20, 100, 20, 100, 100, 150);
        case 2: return three(s, 50, 130, 30, 100, 30, 50);
        default: return three(s, 20, 50, 20, 50, 20, 50);
        }
    case 3: case 6: return wallColour(t, cx, cy, c);
    case 4: return {135, 130, 140};
    case 8: return {60, 40, 10};
    case 11: return {180, 170, 90};
    default: return {50, 50, 50};
    }
}

// --- T15 ---------------------------------------------------------------------------------------

Colour castleFirstColour(int style, int b) {
    if (style == 2)
        return {255, 255, 30};
    switch (b) {
    case 40: return {120, 100, 180};
    case 41: return {100, 160, 90};
    case 42: return {250, 200, 80};
    case 43: return {250, 280, 300};
    case 44: return {200, 250, 250};
    case 45: return {270, 250, 100};
    case 46: return {250, 230, 100};
    case 47: return {130, 100, 120};
    default: return {100, 160, 200};
    }
}

Colour castleSecondColour(int style, int b) {
    if (style == 2)
        return {255, 150, 100};
    switch (b) {
    case 40: return {60, 180, 160};
    case 41: return {100, 180, 160};
    case 42: return {129, 255, 255};
    case 43: return {180, 200, 260};
    case 44: return {50, 160, 40};
    case 45: return {270, 200, 100};
    case 46: return {200, 230, 100};
    case 47: return {130, 160, 160};
    default: return {129, 255, 255};
    }
}

bool castleLightColour(int kind, Colour &out) {
    switch (kind) {
    case 9: out = {10, 10, 10}; return true;
    case 10: out = {20, 100, 100}; return true;
    case 11: out = {260, 100, 10}; return true;
    case 12: out = {50, 150, 20}; return true;
    default: return false;
    }
}

int castleRoomDepth(int kind) {
    switch (kind) {
    case 9: return 40;
    case 10: return 20;
    case 11: case 12: return 8;
    default: return 0;
    }
}

int castleRoomRise(int style) { return style == 1 ? 20 : 16; }

} // namespace worldgen
