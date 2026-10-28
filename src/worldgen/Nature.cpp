// Natural structures, written from docs/specs/worldgen-structures-nature.md (sections cited as §n).
#include "worldgen/Nature.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"
#include "worldgen/Mason.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace worldgen {
namespace {

constexpr uint8_t WOOD = 7, LEAVES = 8, SNOWY_LEAVES = 11;
constexpr uint8_t FLAG_LEAVES = 1;
constexpr Block SNOW_CAP = {100, 140, 200, 0, blocktype::SNOW, 1};

struct Int3 {
    int32_t x, y, z;
};
struct Vec3 {
    float x, y, z;
};

// Each column of a box [x0, x1) x [y0, y1), clipped to the zone first (0x146390); x outer.
template <class F> void eachColumn(const ZoneBuild &zone, int64_t x0, int64_t x1, int64_t y0, int64_t y1, F f) {
    x0 = std::max<int64_t>(x0, zone.x0), y0 = std::max<int64_t>(y0, zone.y0);
    x1 = std::min<int64_t>(x1, zone.x0 + ZONE_SIZE), y1 = std::min<int64_t>(y1, zone.y0 + ZONE_SIZE);
    for (int64_t x = x0; x < x1; x++)
        for (int64_t y = y0; y < y1; y++)
            f((int32_t)x, (int32_t)y);
}

// "At least 1": unless r > 1, r = 1.
float atLeastOne(float r) { return r > 1.0f ? r : 1.0f; }
float atLeastTwo(float r) { return r > 2.0f ? r : 2.0f; }

// The climate at a tree's point: warmth, dryness, forest and kind-27 weights (§2, last paragraph).
struct Climate {
    float warm, dry, forest, k27;
};
Climate climateAt(int32_t x, int32_t y) {
    const Neighbourhood n = neighbourhood(x, y);
    const Place p = placeAt(x, y);
    return {warmth(n, p.kind, p.weight), dryness(n), forestRegion(n, p.kind, p.weight), kind27Weight(p.kind, p.weight)};
}

// (target - v) x f + v.
float toward(float v, float target, float f) { return (target - v) * f + v; }

// --- The leaf colours (§2) -------------------------------------------------------------------

struct LeafColours {
    Colour a, b;
};

// The running hash of the leaf colours.
struct LeafHash {
    uint32_t s;
    uint32_t next(uint32_t k) {
        const uint32_t v = xorshift(s);
        s += k;
        return v;
    }
};

// The shift toward forest and kind-27 colours of the birch and the fir.
void shiftColours(LeafColours &c, uint32_t s, float forest, float k27) {
    if (forest > 0.01f) {
        const uint32_t v = xorshift(s);
        s += 1;
        c.a.g = toward(c.a.g, 100.0f, forest), c.a.r = toward(c.a.r, 100.0f, forest);
        c.a.b = toward(c.a.b, (float)(v % 80 + 140), forest);
        const uint32_t w = xorshift(s);
        s += 1;
        c.b.r = toward(c.b.r, 60.0f, forest), c.b.g = toward(c.b.g, 60.0f, forest);
        c.b.b = toward(c.b.b, (float)(w % 30 + 60), forest);
    }
    if (k27 > 0.3f) {
        const uint32_t v = xorshift(s);
        s += 2;
        c.a.g = toward(c.a.g, 0.0f, k27);
        c.a.b = toward(c.a.b, (float)(v % 200), k27);
        c.a.r = toward(c.a.r, (float)(v % 55 + 200), k27);
        const uint32_t w = xorshift(s);
        c.b.r = toward(c.b.r, 100.0f, k27);
        c.b.b = toward(c.b.b, (float)(w % 55 + 200), k27);
        c.b.g = toward(c.b.g, (float)(w % 50 + 100), k27);
    }
}

// 0x2afbf0.
LeafColours leafColours(int32_t x, int32_t y, const Climate &k) {
    LeafHash h{xorshift((uint32_t)wrapAdd(wrapMul(y, 100), 0x3d7)) + xorshift((uint32_t)wrapAdd(wrapMul(x, 100), 0xa8eb))};
    LeafColours c;
    const float t = (float)(h.next(1) % 1000) / 1000.0f;
    bool warmPalette = (k.warm - 0.2f) * 5.0f > t;
    if (!warmPalette && h.next(1) % 3 == 0)
        warmPalette = true;
    if (!warmPalette && 0.5f > k.dry) {
        const uint32_t v = h.next(3);
        c.a.b = (float)(v % 50), c.b.b = c.a.b + 0.0f;
        c.a.g = (float)(v % 150);
        c.a.r = (float)(v % 55 + 200);
        const float add = (float)(int32_t)(h.next(1) % 100 + 60);
        c.b.r = c.a.r + add, c.b.g = c.a.g + add;
    } else if (!warmPalette) {
        uint32_t p = h.s ^ (h.s << 13);
        h.s += 1;
        p ^= p >> 17;
        if (p & 1) { // pale
            const uint32_t v = h.next(3);
            c.a = {(float)(v % 50 + 180), (float)(v % 50 + 120), (float)(v % 50 + 240)};
            const float add = (float)(int32_t)(h.next(1) % 50 + 100);
            c.b = {c.a.r + add, c.a.g + add, c.a.b + add};
        } else { // autumn
            c.a.g = 0.0f;
            const uint32_t v = h.next(2);
            c.a.b = (float)(v % 230);
            c.a.r = (float)(v % 100 + 150);
            const float m = (float)(int32_t)(h.next(1) % 80);
            const uint32_t w = h.next(1);
            c.a.r = c.a.r + m, c.a.g = c.a.g + m, c.a.b = c.a.b + m;
            const float add = (float)(int32_t)(w % 50 + 100);
            c.b = {c.a.r + add, c.a.g + add, c.a.b + add};
        }
    } else {
        const uint32_t v = h.next(3);
        c.a.b = (float)(v % 30);
        c.a.g = (float)(v % 60 + 100);
        c.a.r = (float)(v % 20);
        const float d = (0.5f - k.dry) * 80.0f;
        c.a.r = d + c.a.r;
        if (k.dry > 0.5f)
            c.a.b = c.a.b - d;
        c.b.b = c.a.b + 0.0f;
        const float add = (float)(int32_t)(h.next(1) % 100 + 50);
        c.b.r = c.a.r + add, c.b.g = c.a.g + add;
        c.a.b = c.a.b + 80.0f, c.a.g = c.a.g - 20.0f, c.a.r = c.a.r - 20.0f;
    }
    if (k.forest > 0.01f) {
        const float target = (float)(int32_t)(h.next(1) % 80 + 140);
        c.a.g = toward(c.a.g, 100.0f, k.forest);
        c.a.r = toward(c.a.r, 100.0f, k.forest);
        c.a.b = toward(c.a.b, target, k.forest);
        const float kb = (float)(int32_t)(h.next(1) % 30 + 60);
        c.b.r = toward(c.b.r, 60.0f, k.forest);
        c.b.g = toward(c.b.g, 60.0f, k.forest);
        c.b.b = toward(c.b.b, kb, k.forest);
    }
    if (snowAt(x, y, k.warm, NeighbourHeights{}))
        c.a = c.b = {200, 280, 400};
    if (k.k27 > 0.3f) {
        c.a.g = toward(c.a.g, 0.0f, k.k27);
        const uint32_t v = h.next(2);
        c.a.b = toward(c.a.b, (float)(v % 200), k.k27);
        c.a.r = toward(c.a.r, (float)(v % 55 + 200), k.k27);
        const uint32_t w = xorshift(h.s);
        c.b.r = toward(c.b.r, 100.0f, k.k27);
        c.b.b = toward(c.b.b, (float)(w % 55 + 200), k.k27);
        c.b.g = toward(c.b.g, (float)(w % 50 + 100), k.k27);
    }
    return c;
}

// 0x2ae3a0.
LeafColours birchColours(int32_t x, int32_t y, const Climate &k) {
    LeafColours c{{0, 180, 100}, {255, 320, 0}};
    if (0.15f > k.warm)
        c.a = c.b = {200, 280, 400};
    shiftColours(c, xorshift(xorshift((uint32_t)y) + (uint32_t)x), k.forest, k.k27);
    return c;
}

// 0x2ae840.
LeafColours firColours(int32_t x, int32_t y, const Climate &k) {
    const uint32_t s = xorshift(xorshift((uint32_t)y) + (uint32_t)x);
    const uint32_t v = xorshift(s), w = xorshift(s + 3);
    LeafColours c{{(float)(v % 10), (float)(v % 50 + 100), (float)(v % 80 + 50)},
                  {(float)(w % 10), (float)(w % 50 + 100), (float)(w % 80 + 50)}};
    shiftColours(c, s + 6, k.forest, k.k27);
    return c;
}

// --- The tree painter (§1, §2) -----------------------------------------------------------------

class Tree {
public:
    explicit Tree(ZoneBuild &zone) : zone(zone) {}
    ZoneBuild &zone;
    int32_t zLow = 0, zHigh = 0;
    Colour low, high;
    bool speckle = false;

    void paint(int32_t zl, Colour cl, int32_t zh, Colour ch) { zLow = zl, low = cl, zHigh = zh, high = ch; }

    // 0x262fa0.
    Colour colourAt(int32_t x, int32_t y, int32_t z) const {
        if (speckle) {
            uint32_t h = xorshift((uint32_t)z) + (uint32_t)y;
            h ^= h << 13;
            h ^= h >> 17;
            h = ((h << 5) ^ h) + (uint32_t)x + 0x4d2;
            h ^= h << 13;
            h = (h >> 17) ^ h;
            if ((h & 7) == 0)
                return {50, 50, 50};
        }
        if (z <= zLow)
            return low;
        if (z >= zHigh)
            return high;
        const float t = (float)(z - zLow) / (float)(zHigh - zLow), u = 1.0f - t;
        return {high.r * t + low.r * u, high.g * t + low.g * u, high.b * t + low.b * u};
    }
    Block wood(int32_t x, int32_t y, int32_t z) const { return packed(colourAt(x, y, z), 0, WOOD, 0); }

    // Bark colour (0x2668a0): two draws.
    Colour bark() {
        const float t = (float)zone.draw() / 32767.0f;
        const float g = (float)(zone.draw() % 20 + 160);
        return {t * 40.0f + 260.0f, t * 40.0f + g, t * 30.0f + 60.0f};
    }

    // The ring shade of a trunk's or log's inside.
    static float ring(float d2) { return (float)(trunc32(std::sqrt(d2)) % 2) * 0.2f + 1.3f; }

    // 0x2671e0.
    void trunk(int32_t cx, int32_t cy, int32_t z, float r0, float r1, int32_t n) {
        for (int32_t k = 0; k < n; k++) {
            const float t = (float)k / (float)(n - 1);
            float r = (1.0f - t) * r0 + t * r1;
            if (1.0f > r)
                r = 1.0f;
            const int32_t ri = trunc32(r), h = z + k;
            eachColumn(zone, (int64_t)cx - ri - 1, (int64_t)cx + 2 + ri, (int64_t)cy - ri - 1, (int64_t)cy + 2 + ri,
                       [&](int32_t x, int32_t y) {
                           const float dx = (float)(x - cx) + 0.5f, dy = (float)(y - cy) + 0.5f;
                           const float d2 = dy * dy + dx * dx;
                           if (d2 > r * r)
                               return;
                           const Colour c = colourAt(x, y, h);
                           const float rim = r - 1.0f;
                           const Colour shade = rim * rim > d2 ? scaled(c, ring(d2)) : c;
                           zone.putUnlessProtected(x, y, h, packed(shade, 0, WOOD, 0));
                       });
        }
    }

    // 0x2669e0.
    void woodBall(int32_t cx, int32_t cy, int32_t cz, float r) {
        const int32_t ri = trunc32(r);
        eachColumn(zone, (int64_t)cx - ri - 1, (int64_t)cx + ri + 2, (int64_t)cy - ri - 1, (int64_t)cy + ri + 2,
                   [&](int32_t x, int32_t y) {
                       for (int32_t k = trunc32(-r - 1.0f); r + 1.0f >= (float)k; k++) {
                           const float dk = (float)k + 0.5f, dx = (float)(x - cx) + 0.5f, dy = (float)(y - cy) + 0.5f;
                           const float d2 = dy * dy + dx * dx + dk * dk;
                           if (d2 <= r * r)
                               zone.putUnlessProtected(x, y, cz + k, wood(x, y, cz + k));
                       }
                   });
    }

    // 0x262dc0: a branch of wood balls; returns its end.
    Int3 branch(int32_t x, int32_t y, int32_t z, Vec3 dir, float r0, float r1, int32_t n) {
        int64_t px = dotsOf(x), py = dotsOf(y), pz = dotsOf(z);
        for (int32_t i = 0; i < n; i++) {
            const float t = (float)i / (float)(n - 1);
            const float r = (1.0f - t) * r0 + t * r1;
            woodBall(blocksTowardZero(px), blocksTowardZero(py), blocksTowardZero(pz), r);
            px += trunc64(dir.x * 65536.0f);
            py += trunc64(dir.y * 65536.0f);
            pz += trunc64(t * dir.z * 65536.0f);
        }
        return {blocksOf(px), blocksOf(py), blocksOf(pz)};
    }

    // 0x2674a0: a twig of single blocks, coloured at its start.
    Int3 twig(int32_t x, int32_t y, int32_t z, Vec3 dir, int32_t n) {
        int64_t px = dotsOf(x), py = dotsOf(y), pz = dotsOf(z);
        const Block b = wood(x, y, z);
        for (int32_t i = 0; i < n; i++) {
            const float t = (float)i / (float)(n - 1);
            zone.putUnlessProtected(blocksTowardZero(px), blocksTowardZero(py), blocksTowardZero(pz), b);
            px += trunc64(dir.x * 65536.0f);
            py += trunc64(dir.y * 65536.0f);
            pz += trunc64(t * dir.z * 65536.0f);
        }
        return {blocksOf(px), blocksOf(py), blocksOf(pz)};
    }

    // Roots: four branches down and out.
    void roots(int32_t x, int32_t y, int32_t z, float tr) {
        const float rr = atLeastOne(tr * 0.75f);
        const int32_t n = trunc32(tr + tr);
        branch(x, y, z, {1, 0, -1}, rr, rr, n);
        branch(x, y, z, {-1, 0, -1}, rr, rr, n);
        branch(x, y, z, {0, 1, -1}, rr, rr, n);
        branch(x, y, z, {0, -1, -1}, rr, rr, n);
    }

    // 0x2650e0, and the willow's (0x263db0) when `willow`.
    void cloud(int32_t cx, int32_t cy, int32_t cz, float r, float rz, bool snowy, bool willow = false) {
        const int32_t ri = trunc32(r);
        eachColumn(zone, (int64_t)cx - ri - 1, (int64_t)cx + ri + 2, (int64_t)cy - ri - 1, (int64_t)cy + ri + 2,
                   [&](int32_t x, int32_t y) {
                       const float n = simplex((double)x * 2.0 / (double)r, (double)y * 2.0 / (double)r) * 0.3f;
                       for (int32_t k = trunc32(-rz - 2.0f); rz + 2.0f >= (float)k; k++) {
                           const float v = (float)k / rz + n;
                           const float dx = ((float)(x - cx) + 0.5f) / r, dy = ((float)(y - cy) + 0.5f) / r;
                           if (dy * dy + dx * dx + v * v > 1.0f)
                               continue;
                           const int32_t z = cz + k;
                           if (!isOpen(zone.read(x, y, z)))
                               continue;
                           const Block leaf = packed(colourAt(x, y, z), 0, snowy ? SNOWY_LEAVES : LEAVES, FLAG_LEAVES);
                           zone.putUnlessProtected(x, y, z, leaf);
                           if (willow)
                               hangingLeaves(x, y, z, leaf);
                       }
                   });
    }

    // The willow's hanging strands below a leaf block.
    void hangingLeaves(int32_t x, int32_t y, int32_t z, Block leaf) {
        const uint32_t h1 = xorshift((uint32_t)z) + (uint32_t)y, h2 = xorshift(h1) + (uint32_t)x;
        if (xorshift(h2) % 20 != 0)
            return;
        uint32_t len = xorshift(h2 + 0x2e2) % 10;
        uint32_t t = xorshift(h1 + 0x7c9) + (uint32_t)x;
        t ^= t << 13;
        t ^= t >> 17;
        if ((t & 3) == 0)
            len *= 2;
        for (uint32_t j = 0; j < len; j++)
            zone.putUnlessProtected(x, y, z - 1 - (int32_t)j, leaf);
    }

    // A fruit model (0x4e-0x50) or the leaf model (0x4d): flags 2, the size not written.
    Model treeModel(int32_t type, int32_t x, int32_t y, int32_t z, Colour tint) const {
        Model m;
        m.type = type, m.x = dotsOf(x), m.y = dotsOf(y), m.z = dotsOf(z);
        m.yaw = 0.0f, m.r = tint.r, m.g = tint.g, m.b = tint.b, m.flags = 2;
        return m;
    }
    void fruit(int32_t type, Int3 at) { zone.keepIfInside(treeModel(type, at.x, at.y, at.z, {1, 1, 1})); }
};

constexpr int32_t FRUIT = 0x4e, LEAF_MODEL = 0x4d;

} // namespace

// --- §3 ------------------------------------------------------------------------------------------

void commonTree(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w) {
    Tree tree(zone);
    const Climate k = climateAt(x, y);
    const LeafColours leaves = leafColours(x, y, k);
    const Colour bark1 = tree.bark(), bark2 = tree.bark();
    tree.paint(z, bark2, z + w, bark1);
    const float tr = atLeastOne((float)h * 0.1f);
    tree.trunk(x, y, z, tr, tr * 0.8f, w);
    tree.roots(x, y, z + 1, tr);
    const float br = atLeastOne(tr * 0.5f);
    std::vector<Int3> tips{{x, y, z + w}};
    int32_t count = 8;
    if (h <= 30)
        count = 5;
    if (w > 50)
        count = 10;
    const int32_t len = std::min(h / 2, 15), half = w / 2;
    for (int32_t i = 0, rise = 0; i < count; i++, rise += half) {
        const float a = zone.angle();
        tips.push_back(tree.branch(x, y, rise / (count - 1) + half + z, {std::cos(a), std::sin(a), 1.0f}, br, br, len));
    }
    float q = 0.15f;
    if (!(k.warm >= 0.7f && 0.3f > k.dry))
        q = zone.fraction(0.35f) + 0.35f;
    float fruitSeed = (float)(zone.draw() % 100000);
    const float bigR = (float)h * 0.5f * 0.75f, rz = bigR * q;
    const bool snowy = 0.15f > k.warm;
    for (const Int3 &e : tips) {
        tree.paint(trunc32((float)e.z - rz), leaves.a, trunc32(rz + (float)e.z), leaves.b);
        if (bigR > 16.0f) {
            for (int n = 0; n < 5; n++) {
                const int32_t s = zone.draw() % 7 + 6;
                const int a = zone.draw(), b = zone.draw(), c = zone.draw();
                const int32_t cx = trunc32((float)e.x + (float)(2 * c) * 5.0f / 32767.0f - 5.0f);
                const int32_t cy = trunc32((float)e.y + (float)(2 * b) * 5.0f / 32767.0f - 5.0f);
                const int32_t cz = trunc32((float)e.z + (float)a * 5.0f / 32767.0f - 2.5f + 1.0f);
                tree.cloud(cx, cy, cz, (float)s, q * (float)s, snowy);
            }
        } else {
            tree.cloud(e.x, e.y, e.z + 1, bigR, rz, snowy);
        }
        if (!zone.inZone(e.x, e.y))
            continue;
        const uint32_t v = xorshift((uint32_t)trunc32(fruitSeed));
        fruitSeed = (float)(trunc32(fruitSeed) + 1);
        if (v % 10 == 0) {
            const int32_t type = (int32_t)(xorshift((uint32_t)trunc32(fruitSeed)) % 3) + FRUIT;
            fruitSeed = (float)(trunc32(fruitSeed) + 1);
            tree.fruit(type, e);
        }
        zone.keepIfInside(tree.treeModel(LEAF_MODEL, e.x, e.y, e.z, {leaves.a.r / 255.0f, leaves.a.g / 255.0f, leaves.a.b / 255.0f}));
    }
}

// --- §4 ------------------------------------------------------------------------------------------

void fir(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w) {
    Tree tree(zone);
    const Climate k = climateAt(x, y);
    const LeafColours leaves = firColours(x, y, k);
    const Colour bark1 = tree.bark(), bark2 = tree.bark();
    tree.paint(z, bark2, z + w, bark1);
    const int32_t hh = trunc32((float)h * 0.5f);
    const float hf = (float)hh;
    const int32_t len = trunc32((double)hh * 1.5 + (double)w);
    const float tr = atLeastOne(hf * 0.15f);
    // The tapering trunk (0x266ea0) from z - 1.
    for (int32_t kk = 0; kk < len; kk++) {
        const float u = 1.0f - (float)kk / (float)(len - 1);
        const float c = 1.0f - u * (u * u);
        const float r = (1.0f - c) * tr + c * 0.5f;
        const int32_t i0 = trunc32(-r);
        const int32_t bz = z - 1 + kk;
        for (int32_t i = i0; r >= (float)i; i++)
            for (int32_t j = i0; r >= (float)j; j++) {
                if ((float)j * (float)j + (float)i * (float)i > r * r)
                    continue;
                zone.putUnlessProtected(x + i, y + j, bz, tree.wood(x + i, y + j, bz));
            }
    }
    float a = zone.angle();
    tree.paint(len / 4 + z, leaves.a, z + len, leaves.b);
    const float s = zone.fraction(0.2f) + 0.2f, t = 1.0f - s;
    const float droop = (float)(-hh) * 0.3f;
    const int32_t steps = len / 3, top = z + len;
    const bool snowy = 0.15f > k.warm;
    // A needle disc (0x263120).
    auto disc = [&](int32_t cx, int32_t cy, int32_t cz, float r, Block b) {
        const int32_t ri = trunc32(r);
        eachColumn(zone, (int64_t)cx - ri - 1, (int64_t)cx + ri + 2, (int64_t)cy - ri - 1, (int64_t)cy + ri + 2,
                   [&](int32_t bx, int32_t by) {
                       const float dx = (float)(bx - cx), dy = (float)(by - cy);
                       if (r * r >= dy * dy + dx * dx)
                           zone.putUnlessProtected(bx, by, cz, b);
                   });
    };
    for (int i = 0; i < 30; i++) {
        a = (float)((double)a + 0.9424778335276408);
        const float q = (float)i * t / 30.0f + s;
        const float ca = std::cos(a) * hf, sa = std::sin(a) * hf;
        const float u = 1.0f - q, u2 = u + u;
        const int64_t sx = trunc64((double)x * 65536.0), sy = trunc64((double)y * 65536.0);
        const int64_t sz = trunc64((double)((float)len * q + (float)z) * 65536.0);
        const int64_t ex = sx + trunc64(ca * u2 * 65536.0f), ey = sy + trunc64(sa * u2 * 65536.0f);
        const int64_t ez = sz + trunc64(droop * u2 * 65536.0f);
        for (int32_t kk = steps; kk >= 1; kk--) {
            const float f = (float)kk / (float)steps, g = 1.0f - f;
            auto along = [&](int64_t from, int64_t to) {
                return to + trunc64((float)(from - to) * (1.0f / 65536.0f) * g * 65536.0f);
            };
            const int64_t px = along(sx, ex), py = along(sy, ey), pz = along(sz, ez);
            const float gtu = g * tr * u;
            const float rad = atLeastTwo((f + f) + (gtu + gtu));
            const int32_t bx = blocksTowardZero(px), by = blocksTowardZero(py);
            if (snowy)
                disc(bx, by, blocksTowardZero(pz + DOTS), rad, SNOW_CAP);
            const int32_t bz = blocksTowardZero(pz);
            disc(bx, by, bz, rad, packed(tree.colourAt(bx, by, bz), 0, LEAVES, FLAG_LEAVES));
        }
        for (int32_t hz : {top + 1, top, top - 1})
            zone.putUnlessProtected(x, y, hz, packed(tree.colourAt(x, y, std::max(hz, top)), 0, LEAVES, 0));
    }
    if (zone.draw() % 10 == 0)
        tree.fruit(FRUIT + zone.draw() % 3, {x, y, top - 1});
}

// --- §5 ------------------------------------------------------------------------------------------

void birch(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w) {
    Tree tree(zone);
    tree.paint(z, {255, 300, 300}, w + z, {255, 300, 300});
    tree.speckle = true;
    const float tr = atLeastOne((float)h * 0.05f);
    tree.trunk(x, y, z, tr, tr, w);
    tree.roots(x, y, z, tr);
    std::vector<Int3> tips{{x, y, z + w}};
    const int32_t half = w / 2, len = h / 3;
    for (int32_t i = 0, rise = 0; i < 5; i++, rise += half) {
        const float a = zone.angle();
        tips.push_back(tree.twig(x, y, rise / 4 + z + half, {std::cos(a), std::sin(a), 0.5f}, len));
    }
    tree.speckle = false;
    const Climate k = climateAt(x, y);
    const LeafColours leaves = birchColours(x, y, k);
    const bool snowy = 0.15f > k.warm;
    const float bigH = (float)h * 0.5f;
    for (const Int3 &e : tips) {
        const float r = (zone.fraction(0.25f) + 0.5f) * bigH, rz = r * 0.5f;
        tree.paint(trunc32((float)e.z - rz), leaves.a, trunc32((float)e.z + rz), leaves.b);
        tree.cloud(e.x, e.y, e.z + 1, r, rz, snowy);
        if (zone.draw() % 10 == 0)
            tree.fruit(FRUIT + zone.draw() % 3, e);
    }
}

// --- §6 ------------------------------------------------------------------------------------------

void willow(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w) {
    Tree tree(zone);
    const Climate k = climateAt(x, y);
    const LeafColours leaves = leafColours(x, y, k);
    const int32_t top = w + z;
    tree.paint(z, {200, 150, 80}, top, scaled(leaves.a, 0.5f));
    const float tr = atLeastOne((float)h * 0.1f);
    tree.trunk(x, y, z, tr, tr, w);
    tree.roots(x, y, z + 1, tr);
    const float br = atLeastOne(tr * 0.5f);
    std::vector<Int3> tips{{x, y, top}};
    const int32_t len = h / 2, half = w / 2;
    for (int32_t i = 0, rise = 0; i < 5; i++, rise += half) {
        const float a = zone.angle();
        tips.push_back(tree.branch(x, y, rise / 4 + z + half, {std::cos(a), std::sin(a), 1.0f}, br, br, len));
    }
    uint32_t fruitSeed = (uint32_t)(zone.draw() % 100000);
    const float bigH = (float)h * 0.5f;
    for (const Int3 &e : tips) {
        const float r = (zone.fraction(0.25f) + 0.75f) * bigH, rz = r * 0.5f;
        tree.paint(e.z - half, leaves.a, trunc32((float)e.z + rz), leaves.b);
        tree.cloud(e.x, e.y, e.z + 1, r, rz, false, true);
        if (!zone.inZone(e.x, e.y))
            continue;
        const uint32_t v = xorshift(fruitSeed++);
        if (v % 10 == 0)
            tree.fruit(FRUIT + (int32_t)(xorshift(fruitSeed++) % 3), e);
    }
}

// --- §7 ------------------------------------------------------------------------------------------

void palm(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w) {
    Tree tree(zone);
    const Climate k = climateAt(x, y);
    const LeafColours leaves = leafColours(x, y, k);
    const int32_t len = trunc32((float)w * 1.2f);
    tree.paint(z, {200, 150, 80}, z + len, {200, 150, 80});
    const float hf = (float)h;
    const float tr = atLeastOne(hf * 0.1f);
    const float a = zone.angle();
    const float sa = std::sin(a), ca = std::cos(a);
    const float lean = (float)zone.draw() / 32767.0f;
    const float lx = ca * lean, ly = lean * sa;
    // The trunk, bending with height.
    int64_t topX = dotsOf(x), topY = dotsOf(y), topZ = dotsOf(z);
    for (int32_t kk = -3; kk < len; kk++) {
        const float t = maxOf(0.0f, (float)kk / (float)(len - 1));
        const float r = atLeastOne(tr - t * 0.75f * tr);
        const float u = (1.0f - t) * (1.0f - t);
        const float bend = (1.0f - u) * 0.25f * hf;
        const int64_t cx = trunc64(lx * bend * 65536.0f) + dotsOf(x), cy = trunc64(ly * bend * 65536.0f) + dotsOf(y);
        const int64_t rr = trunc64(r * 65536.0f);
        eachColumn(zone, (cx - rr - DOTS) / DOTS, (cx + 2 * DOTS + rr) / DOTS, (cy - rr - DOTS) / DOTS, (cy + 2 * DOTS + rr) / DOTS,
                   [&](int32_t bx, int32_t by) {
                       const float dx = (float)(dotsOf(bx) - cx + 32768) * (1.0f / 65536.0f);
                       const float dy = (float)(dotsOf(by) - cy + 32768) * (1.0f / 65536.0f);
                       if (dy * dy + dx * dx > r * r)
                           return;
                       const int32_t band = posMod(kk / 2, 2);
                       const float f = (float)band * 0.15f + 0.85f;
                       const int32_t bz = z + kk;
                       zone.putUnlessProtected(bx, by, bz, packed(scaled(tree.colourAt(bx, by, bz), f), 0, WOOD, 0));
                   });
        topX = dotsOf(blocksTowardZero(cx)), topY = dotsOf(blocksTowardZero(cy)), topZ = dotsOf(z + kk);
    }
    // The fronds.
    const int32_t n = zone.draw() % 3 + 5;
    const int32_t half = len / 2;
    tree.paint(half + z, leaves.a, z + len, leaves.b);
    const int32_t reach = h / 2;
    const float hr = (float)reach;
    for (int32_t i = 0; i < n; i++) {
        const float fi = (float)i;
        const float ang = (float)((double)(fi + fi) * 3.141592653589793 / (double)n + (double)a);
        const float fc = std::cos(ang), fs = std::sin(ang);
        const float droop = (float)(zone.draw() % half + half);
        for (int32_t j = 0; j < reach; j++) {
            const float jf = (float)j, uu = jf / hr, u3 = uu * (uu * uu);
            const float v = (float)(j + 1) / hr;
            const int64_t px = trunc64(fc * jf * 65536.0f) + topX, py = trunc64(fs * jf * 65536.0f) + topY;
            const int32_t zA = blocksTowardZero(topZ - trunc64(droop * u3 * 0.5f * 65536.0f) + DOTS);
            const float wide = (1.0f - u3) * 5.0f;
            const int32_t zB = blocksTowardZero(topZ - trunc64(v * v * v * droop * 0.5f * 65536.0f) + DOTS);
            const int32_t m0 = trunc32(wide * -0.5f);
            const float wh = wide * 0.5f;
            for (int32_t zz = zB; zz <= zA; zz++)
                for (int32_t m = m0; wh >= (float)m; m++) {
                    const int32_t bx = blocksTowardZero(px - trunc64(fs * (float)m * 65536.0f));
                    const int32_t by = blocksTowardZero(trunc64(fc * (float)m * 65536.0f) + py);
                    const Block there = zone.read(bx, by, zz);
                    if (!isOpen(there) || (there.flags & 0x40))
                        continue;
                    zone.putUnlessProtected(bx, by, zz, packed(tree.colourAt(bx, by, zz), 0, LEAVES, 0));
                }
        }
    }
}

// --- §8 ------------------------------------------------------------------------------------------

void stump(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t h, int32_t w) {
    Tree tree(zone);
    Colour bark = tree.bark();
    const Climate k = climateAt(x, y);
    const LeafColours leaves = leafColours(x, y, k);
    bark.r = toward(bark.r, 150.0f, k.k27);
    bark.b = toward(bark.b, 200.0f, k.k27);
    bark.g = toward(bark.g, 100.0f, k.k27);
    const Colour high{(bark.r * 0.25f + leaves.a.r * 0.75f) * 0.5f, (bark.g * 0.25f + leaves.a.g * 0.75f) * 0.5f,
                      (bark.b * 0.25f + leaves.a.b * 0.75f) * 0.5f};
    tree.paint(z, bark, w + z, high);
    const float tr = maxOf((float)h * 0.1f, 1.0f);
    tree.roots(x, y, z + 1, tr);
    tree.trunk(x, y, z, tr, tr, trunc32(tr + 1.0f));
}

void fallenLog(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, float r, int32_t len, bool hollow, bool alongY) {
    Tree tree(zone);
    const int32_t zc = trunc32(r - 2.0f + (float)z);
    const Colour bark = tree.bark();
    tree.paint(trunc32((float)zc - r), {(bark.r + 20.0f) * 0.5f, (bark.g + 100.0f) * 0.5f, (bark.b + 30.0f) * 0.5f},
               trunc32(r + (float)zc), bark);
    const int32_t ri = trunc32(r);
    std::vector<Int3> tips;
    for (int side : {1, -1}) {
        if (!(zone.draw() % 2 != 0))
            continue;
        const float d1 = zone.fraction(2.0f) - 1.0f;
        const float d0 = zone.fraction(2.0f) - 1.0f;
        const int32_t tz = zc - 1 + zone.draw() % 3;
        const int32_t off = zone.draw() % (len - 2) - len / 2;
        if (!alongY)
            tips.push_back(tree.twig(x + 1 + off, side > 0 ? y + ri + 1 : y - ri, tz, {d0, (float)side, d1}, ri));
        else
            tips.push_back(tree.twig(side > 0 ? x + ri + 1 : x - ri, y + 1 + off, tz, {(float)side, d0, d1}, ri));
    }
    // The log.
    const int64_t a0 = alongY ? y - len / 2 : x - len / 2, a1 = alongY ? y + len / 2 : x + len / 2;
    const int64_t c0 = (alongY ? x : y) - ri - 1, c1 = (alongY ? x : y) + 2 + ri;
    auto build = [&](int32_t bx, int32_t by) {
        const int32_t across = alongY ? bx - x : by - y;
        for (int32_t bz = zc - ri - 1; bz <= zc + 1 + ri; bz++) {
            const float da = (float)across - 0.5f, dz = (float)(bz - zc) - 0.5f;
            const float d2 = dz * dz + da * da;
            if (d2 > r * r)
                continue;
            const float rim = r - 1.0f;
            Block b;
            if (rim * rim > d2)
                b = hollow ? Block{0, 0, 0, 0, 0, 0} : packed(scaled(tree.colourAt(bx, by, bz), Tree::ring(d2)), 0, WOOD, 0);
            else
                b = packed(tree.colourAt(bx, by, bz), 0, WOOD, 0);
            zone.putUnlessProtected(bx, by, bz, b);
        }
    };
    if (alongY)
        eachColumn(zone, c0, c1, a0, a1, build);
    else
        eachColumn(zone, a0, a1, c0, c1, build);
    const LeafColours leaves = leafColours(x, y, climateAt(x, y));
    for (const Int3 &e : tips) {
        const float q = (float)zone.draw() / 32767.0f + 1.0f;
        tree.paint(trunc32((float)e.z - q), leaves.a, trunc32((float)e.z + q), leaves.b);
        tree.cloud(e.x, e.y, e.z, q, q, false);
    }
}

void mound(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry) {
    eachColumn(zone, (int64_t)x - rx, (int64_t)x + rx + 1, (int64_t)y - ry, (int64_t)y + ry + 1, [&](int32_t X, int32_t Y) {
        const ColumnField f = zone.columnValuesAt(X, Y);
        const float topf = noiseAt(X, Y, 0.01, 4394, 0.01, 8974) * 4.0f + (float)z;
        const int32_t top = trunc32(topf);
        const float a = noiseAt(X, Y, 0.05, 8384, 0.05, 2337) * 0.1f + (float)(X - x) / (float)rx;
        const float b = noiseAt(X, Y, 0.05, 3244, 0.05, 2133) * 0.1f + (float)(Y - y) / (float)ry;
        float d = minOf(b * b + a * a, 1.0f);
        if (d >= 1.0f)
            return;
        d = d * d;
        const float n3 = (float)std::fabs((double)noiseAt(X, Y, 0.15, 34234, 0.15, 2424)) * 0.3f;
        const float m = n3 + absOf(noiseAt(X, Y, 0.08, 1034, 0.08, 545));
        const float e = 1.0f - d;
        const int32_t bottom = trunc32((((float)(z - 10) - e * 10.0f) - m * 10.0f) * e + d * topf);
        for (int32_t Z = top; Z >= bottom; Z--) {
            if (zone.read(X, Y, Z).flags & 0x22)
                continue;
            BlockProps p;
            if (Z == top) {
                if (0.15f > f.colours.warmth)
                    p = SNOW_BLOCK;
                else
                    p = surfaceColour(X, Y, Z, f.colours);
            } else {
                const BlockProps bed = riverBedColour(X, Y, Z, zone.terrain.sand);
                const float k = m * 0.5f + 0.5f;
                const Colour c{bed.r * k, bed.g * k, bed.b * k};
                const float grey = c.r * 0.3f + c.g * 0.59f + c.b * 0.11f;
                const float inv = 1.0f / maxOf(k, 0.001f);
                const float wgt = (1.0f - inv) * grey;
                p = {c.r * inv + wgt, c.g * inv + wgt, c.b * inv + wgt, 0, blocktype::GROUND, 0};
            }
            if (Z == bottom)
                p.type = 0, p.glow = 255;
            zone.store(X, Y, Z, toBlock(p));
            if (Z == top && snowAt(X, Y, f.colours.warmth, NeighbourHeights{}))
                zone.store(X, Y, top + 1, SNOW_CAP);
        }
    });
}

// --- §9 ------------------------------------------------------------------------------------------

void sunkenChest(ZoneBuild &zone, int32_t cx, int32_t cy) {
    const int32_t mx = 16 * cx + 8, my = 16 * cy + 8;
    const int32_t base = trunc32(groundHeight(mx, my, heightmode::CLIFF_STEPS) - 16.0f);
    for (int32_t X = mx - 16; X <= mx + 16; X++)
        for (int32_t Y = my - 16; Y <= my + 16; Y++) {
            const float dx = (float)(X - mx), dy = (float)(Y - my);
            const float t = (dx * dx + dy * dy) * 0.00390625f;
            if (t > 1.0f || !zone.inZone(X, Y))
                continue;
            float u = t - 0.5f;
            u += u;
            const float v = maxOf(u, 0.0f);
            const float k = 1.0f - v * v;
            const float gh = groundHeight(X, Y, heightmode::CLIFF_STEPS);
            const float hgt = (float)base * k + (1.0f - k) * gh;
            const int32_t hb = floorToInt(hgt), g0 = floorToInt(gh);
            const ColumnField f = zone.columnValuesAt(X, Y);
            zone.store(X, Y, hb - 1, toBlock(buriedColour(X, Y, hb, f.colours, zone.terrain.sand)));
            for (int32_t bz = hb; bz <= g0 + 1; bz++)
                if (bz != g0 - 1)
                    zone.store(X, Y, bz, Block{0, 0, 0, 0, 0, 0});
            int32_t g = g0;
            if (!(f.colours.vegetation > 0.5f))
                g -= 1;
            BlockProps p = buriedColour(X, Y, g, f.colours, zone.terrain.sand);
            const float shade = absOf(noiseAt(X, Y, 0.25, 0, 0.25, 0)) * 0.3f + 0.7f;
            p.r = p.r * shade, p.g = p.g * shade, p.b = p.b * shade;
            p.flags |= 0x10;
            zone.store(X, Y, g, toBlock(p));
            zone.store(X, Y, g - 1, toBlock(p));
        }
    Mason m = Mason::atCell(zone, cx, cy);
    const Colour wall = wallColour(62, cx, cy, zone.place.colours());
    m.twoColours(wall, pattern::STONE, 0.0f, wall, pattern::STONE, 20.0f, true);
    m.box(4, 4, base, 8, 8, 2);
    for (Int2 corner : {Int2{0, 0}, Int2{15, 0}, Int2{15, 15}, Int2{0, 15}})
        m.cappedPost(corner.x, corner.y, base, 1, 10, true);
    StaticObject chest = makeObject(objecttype::CHEST, dotsOf(mx), dotsOf(my), dotsOf(base + 2), (uint8_t)(zone.draw() % 4));
    zone.settle(chest, true);
    for (int i = 0; i < 3; i++)
        zone.draw(); // its contents, not stored
    zone.keepIfInside(chest);
}

// --- §10 -----------------------------------------------------------------------------------------

// 0x146180.
bool insideRock(int32_t X, int32_t Y, int32_t Z, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz) {
    const float dz = (float)(Z - z);
    const float a = noiseAt(Y, Z, 0.02, 3432, 0.02, 65465) * 0.25f + (float)(X - x) / ((float)rx * 0.5f);
    const float b = noiseAt(X, Z, 0.02, 4374, 0.02, 7665) * 0.75f + (float)(Y - y) / ((float)ry * 0.5f);
    const float c = noiseAt(X, Y, 0.04, 4374, 0.04, 9898) * 0.5f + (dz > 0.0f ? dz : 0.0f) / ((float)rz * 0.5f);
    // The vertical term is squared twice (c to the fourth power).
    const float c2 = c * c;
    return 1.0f >= (b * b + a * a) + c2 * c2;
}

void rock(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz, uint8_t flags) {
    eachColumn(zone, (int64_t)x - rx, (int64_t)x + rx + 1, (int64_t)y - ry, (int64_t)y + ry + 1, [&](int32_t X, int32_t Y) {
        const ColumnField f = zone.columnValuesAt(X, Y);
        for (int32_t Z = trunc32(f.height); Z <= z + 2 * rz; Z++) {
            if (zone.read(X, Y, Z).flags & 0x22)
                continue;
            if (!insideRock(X, Y, Z, x, y, z, rx, ry, rz))
                continue;
            const BlockProps p = rockColour(X, Y, Z, f.colours.warmth, f.colours.lavaRegion, f.colours.forestRegion);
            zone.store(X, Y, Z, toBlock({p.r, p.g, p.b, 0, blocktype::ROCK, flags}));
        }
    });
}

void boulder(ZoneBuild &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz, uint8_t flags) {
    const BlockProps top = surfaceColour(x, y, z, colourInputs(columnValues(x, y, 0)));
    for (int32_t X = x - rx; X <= x + rx; X++)
        for (int32_t Y = y - ry; Y <= y + ry; Y++) {
            if (!zone.inZone(X, Y))
                continue;
            const ColumnField f = zone.columnValuesAt(X, Y);
            for (int32_t Z = z - 2 * rz; Z <= z + 2 * rz; Z++) {
                const float a = noiseAt(Y, Z, 0.05, 3432, 0.05, 65465) * 0.25f + (float)(X - x) / (float)rx;
                const float b = noiseAt(X, Z, 0.05, 4374, 0.05, 7665) * 0.25f + (float)(Y - y) / (float)ry;
                const float c = noiseAt(X, Y, 0.05, 4374, 0.05, 9898) * 0.5f + (float)(Z - z) / (float)rz;
                if (!(a * a + b * b + c * c <= 1.0f))
                    continue;
                const float t = minOf(maxOf((float)(rz - z + Z) / (float)rz + 0.5f, 0.0f), 1.0f), u = 1.0f - t;
                const BlockProps r = rockColour(X, Y, Z, f.colours.warmth, f.colours.lavaRegion, f.colours.forestRegion);
                zone.store(X, Y, Z,
                           toBlock({r.r * t + u * top.r, r.g * t + u * top.g, r.b * t + u * top.b, 0, blocktype::ROCK, flags}));
            }
        }
}

void hill(ZoneBuild &zone, int32_t x, int32_t y, int32_t topZ, int32_t rx, int32_t ry, bool rocky, const Canyon *canyon) {
    eachColumn(zone, (int64_t)x - rx, (int64_t)x + rx + 1, (int64_t)y - ry, (int64_t)y + ry + 1, [&](int32_t X, int32_t Y) {
        const ColumnField &f = zone.terrain.field[(size_t)ZoneTerrain::index(X - zone.x0, Y - zone.y0)];
        const int32_t base = zone.column(X, Y)->base;
        int32_t top = trunc32(noiseAt(X, Y, 0.01, 4394, 0.01, 8974) * 10.0f + (float)topZ);
        const bool vegetated = f.colours.vegetation > 0.5f;
        if (vegetated)
            top -= 1;
        float floorz = f.height - 1.0f;
        if (canyon) {
            const float dx = (float)(canyon->end.x - x), dy = (float)(canyon->end.y - y);
            const float px = (float)(dotsOf(X) - dotsOf(x)) * (1.0f / 65536.0f), py = (float)(dotsOf(Y) - dotsOf(y)) * (1.0f / 65536.0f);
            const float dot = dx * px + dy * py;
            float d2;
            if (0.0f >= dot) {
                d2 = py * py + px * px;
            } else {
                const float len2 = dy * dy + dx * dx;
                if (dot >= len2) {
                    const float qx = (float)(dotsOf(X) - dotsOf(canyon->end.x)) * (1.0f / 65536.0f);
                    const float qy = (float)(dotsOf(Y) - dotsOf(canyon->end.y)) * (1.0f / 65536.0f);
                    d2 = qy * qy + qx * qx;
                } else {
                    const float t = dot / len2;
                    const float py2 = py - t * dy, px2 = px - t * dx;
                    d2 = px2 * px2 + py2 * py2;
                }
            }
            if (64.0f > d2) {
                const float e = d2 * 0.015625f;
                floorz = floorz + (1.0f - e * e) * 10.0f;
            }
        }
        for (int32_t Z = top; Z >= base; Z--) {
            if (zone.read(X, Y, Z).flags & 0x22)
                break;
            const float a = noiseAt(Y, Z, 0.05, 0, 0.05, 2337) * 0.25f + (float)(X - x) / ((float)rx * 0.5f);
            const float b = noiseAt(X, Z, 0.05, 4374, 0.05, 9898) * 0.25f + (float)(Y - y) / ((float)ry * 0.5f);
            const float sum = b * b + a * a;
            if (!(1.0f >= sum))
                continue;
            if (canyon && floorz > (float)Z) {
                if (canyon->open && sum > 0.85f) {
                    const float n = noiseAt(Y, Z, 0.25, 89003, 0.25, 10010) + noiseAt(X, Z, 0.25, 0, 0.25, 0);
                    const float v = minOf(absOf(n), 1.0f) * 0.3f + 0.7f;
                    const BlockProps c = rocky ? rockColour(X, Y, Z, f.colours.warmth, f.colours.lavaRegion, f.colours.forestRegion)
                                               : groundColour(X, Y, Z, f.colours, zone.terrain.sand);
                    zone.store(X, Y, Z, toBlock({c.r * v, c.g * v, c.b * v, 0, blocktype::GROUND, 0x12}));
                } else if (Z == base) {
                    zone.store(X, Y, Z, toBlock(surfaceColour(X, Y, Z, f.colours)));
                } else {
                    zone.store(X, Y, Z, Block{0, 0, 0, 0, 0, 2});
                }
                continue;
            }
            BlockProps p;
            if (Z == top && !vegetated)
                p = surfaceColour(X, Y, Z, f.colours);
            else if (rocky)
                p = rockColour(X, Y, Z, f.colours.warmth, f.colours.lavaRegion, f.colours.forestRegion), p.type = blocktype::ROCK;
            else
                p = groundColour(X, Y, Z, f.colours, zone.terrain.sand), p.type = blocktype::GROUND;
            if (!(Z == top && !vegetated))
                p.flags = 0, p.glow = 0;
            zone.store(X, Y, Z, toBlock(p));
            if (Z == top && snowAt(X, Y, f.colours.warmth, NeighbourHeights{}))
                zone.store(X, Y, top + 1, SNOW_CAP);
        }
    });
}

// --- The leaf colours for others (the world map) ---------------------------------------------------

TreeLeaves treeLeaves(LeafKind kind, int32_t x, int32_t y, float warmth, float dryness, float forest, float kind27) {
    const Climate k{warmth, dryness, forest, kind27};
    const LeafColours c = kind == LeafKind::Fir     ? firColours(x, y, k)
                          : kind == LeafKind::Birch ? birchColours(x, y, k)
                                                    : leafColours(x, y, k);
    return {c.a, c.b};
}

} // namespace worldgen
