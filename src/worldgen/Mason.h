#pragma once
// The building class, called here the mason (docs/specs/worldgen-structures-mason.md; the game's
// 0xba9a0). It draws in its own integer coordinates, mapped to the world by a 4 x 4 integer
// matrix, and colours its blocks from a scheme of up to three colours that blend with height.
#include "worldgen/ZoneBuild.h"

#include <array>

namespace worldgen {

// The colour scheme (0x41 bytes at +0x10): three colours, each with a pattern and a height.
struct Scheme {
    Colour colour[3] = {{255, 255, 255}, {255, 255, 255}, {255, 255, 255}};
    int pattern[3] = {0, 0, 0};
    float height[3] = {0, 0, 0};
    int param = 0;         // used by patterns 4-7
    bool relative = false; // heights over the ground rather than absolute
};

// Patterns (0xc1140).
namespace pattern {
constexpr int PLAIN = 0;
constexpr int STONE = 1;      // light patches
constexpr int GROUND = 2;     // the ground's colour (not specified: open question 4)
constexpr int RIVER_BED = 3;
constexpr int STRIPES_X = 4;
constexpr int STRIPES_Y = 5;
constexpr int STRIPES_Z = 6;
constexpr int PATCHES = 7;
} // namespace pattern

// Write modes (+0x70): bits that skip some blocks.
namespace writemode {
constexpr int NOT_SOLID = 1;     // skip solid blocks
constexpr int NOT_PROTECTED = 2; // skip protected blocks (flag 2)
constexpr int ONLY_PROTECTED = 4;
} // namespace writemode

class Mason {
public:
    // A mason with the identity frame (0xba9a0).
    explicit Mason(ZoneBuild &zone);
    // A mason at a cell (0xba8c0 from the identity): frame (16 cx, 16 cy, 0, 16, 16, 0, no mirror).
    static Mason atCell(ZoneBuild &zone, int32_t cx, int32_t cy);
    // A child mason (0xbaa80): its own frame inside this one's, the scheme's heights lowered by z.
    Mason child(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int rot, bool mirror) const;

    // The frame (0xba8c0) applied to this mason's matrix.
    void frame(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int rot, bool mirror);

    // --- Schemes (§2) ---
    Scheme scheme;
    void oneColour(Colour c, int p);                                                   // 0xbebe0
    void twoColours(Colour low, int pl, float ul, Colour high, int ph, float uh, bool relative); // 0xbf8c0
    // The three slots set directly, absolute (0xbf840 and the builders' own).
    void slots(Colour c0, int p0, float u0, Colour c1, int p1, float u1, Colour c2, int p2, float u2);

    int writeMode = 0;   // +0x70
    int waterLine = 0;   // +0x74
    uint8_t type = 6;    // +0x78: the block type it builds with

    // --- Positions ---
    struct World {
        int32_t x, y, z;
    };
    World world(int32_t x, int32_t y, int32_t z) const;
    // The facing of a direction in this frame (0xc15d0).
    int facing(int d) const;
    const std::array<int32_t, 16> &matrix() const { return m_; }
    bool columnInZone(int32_t x, int32_t y) const;

    // --- Colours and blocks ---
    Colour colourAt(int32_t x, int32_t y, int32_t z) const; // 0xbb9e0
    Block block(int32_t x, int32_t y, int32_t z, uint8_t blockType, uint8_t flags) const;
    void put(int32_t x, int32_t y, int32_t z, Block b);     // 0xbf600
    Block get(int32_t x, int32_t y, int32_t z) const;       // 0xbc340
    int32_t groundBelow(int32_t x, int32_t y, int32_t z) const;
    bool caps(int32_t x, int32_t y, int32_t z, int capMode) const;          // 0xbd330
    Block capBlock(int32_t x, int32_t y, int32_t z, int capKind, uint8_t flags) const; // 0xbec30

    // --- Shapes (§4) ---
    void box(int32_t x0, int32_t y0, int32_t z0, int32_t sx, int32_t sy, int32_t sz);
    void foundation(int32_t x0, int32_t y0, int32_t z, int32_t sx, int32_t sy, int32_t h, int capKind, int capMode);
    void pillar(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h, int capKind, int capMode);
    void disc(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h, int capKind = 0, int capMode = 0);
    void cone(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h);
    void liquid(int32_t x0, int32_t y0, int32_t z0, int32_t sx, int32_t sy, int32_t sz, uint8_t flags, int kind);
    void air(int32_t x0, int32_t y0, int32_t z0, int32_t sx, int32_t sy, int32_t sz, uint8_t flags);
    void lightDisc(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h, uint8_t flags);
    void tube(bool alongX, int32_t x0, int32_t y0, int32_t z0, int32_t z1, int32_t len, int32_t r);
    void hangingCone(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h);
    void stripedWall(int32_t x, int32_t y, int32_t z, int32_t w, int32_t h);
    void squarePost(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, bool grounded);
    void cappedPost(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, bool grounded);
    void roundPost(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, bool grounded);
    void battlements(bool alongX, int32_t x0, int32_t y0, int32_t z, int32_t len, int32_t w, int32_t h);
    void ramp(bool alongX, int32_t x0, int32_t y0, int32_t z, int32_t sx, int32_t sy, int32_t a, int32_t b, int capKind,
              int capMode);
    void spiralRing(int32_t x0, int32_t y0, int32_t z, int32_t rIn, int32_t rOut, int32_t h);

    ZoneBuild &zone;

private:
    // One column of a shape (§3 "Column").
    void column(int32_t x, int32_t y, int32_t from, int32_t z, int32_t h, int capKind, int capMode, uint8_t bodyType);
    void putCap(int32_t x, int32_t y, int32_t z, int capKind);
    Colour patterned(Colour c, int p, int32_t x, int32_t y, int32_t z) const;
    std::array<int32_t, 16> m_;
};

// Packing a float colour into a block (0xba820): each channel halved, clamped, truncated.
Block packed(Colour c, uint8_t glow, uint8_t type, uint8_t flags);
// The grass colour of a column (0x2b40b0).
Colour grassColour(int32_t x, int32_t y);

} // namespace worldgen
