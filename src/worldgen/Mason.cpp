// The building class (the mason), written from docs/specs/worldgen-structures-mason.md (sections
// cited as §n).
#include "worldgen/Mason.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"

#include <cmath>

namespace worldgen {

Block packed(Colour c, uint8_t glow, uint8_t type, uint8_t flags) { return toBlock({c.r, c.g, c.b, glow, type, flags}); }

Colour grassColour(int32_t x, int32_t y) {
    // The first step of the surface colour (0x2a3e30); a dryness of 0.5 adds nothing after it.
    ColourInputs plain;
    plain.dryness = 0.5f;
    const BlockProps p = surfaceColour(x, y, 0, plain);
    return {p.r, p.g, p.b};
}

namespace {

constexpr std::array<int32_t, 16> IDENTITY = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

float noise2(int32_t x, int32_t y, double s, double ox, double oy) { return noiseAt(x, y, s, ox, s, oy); }

} // namespace

// --- §1 ----------------------------------------------------------------------------------------

Mason::Mason(ZoneBuild &zone) : zone(zone), m_(IDENTITY) {}

Mason Mason::atCell(ZoneBuild &zone, int32_t cx, int32_t cy) {
    Mason m(zone);
    m.frame(wrapMul(cx, 16), wrapMul(cy, 16), 0, 16, 16, 0, false);
    return m;
}

void Mason::frame(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int rot, bool mirror) {
    std::array<int32_t, 16> &m = m_;
    // 1. Translate.
    for (int row = 0; row < 4; row++)
        m[12 + row] += m[row] * x + m[4 + row] * y + m[8 + row] * z;
    // 2. Move the corner so that the turned frame covers the same cells.
    auto addColumn = [&](int from, int32_t k) {
        for (int row = 0; row < 4; row++)
            m[12 + row] += m[from * 4 + row] * k;
    };
    const int r = rot % 4;
    if (r == 1)
        addColumn(1, sy - 1);
    else if (r == 2)
        addColumn(0, sx - 1), addColumn(1, sy - 1);
    else if (r == 3)
        addColumn(0, sx - 1);
    // 3. Turn (0xbd4a0).
    static constexpr int32_t COS[4] = {1, 0, -1, 0}, SIN[4] = {0, -1, 0, 1};
    const int32_t c = COS[r & 3], s = SIN[r & 3];
    for (int i = 0; i < 4; i++) {
        const int32_t a = m[i], b = m[4 + i];
        m[i] = b * s + a * c;
        m[4 + i] = b * c - a * s;
    }
    // 4. Mirror.
    if (mirror) {
        addColumn(0, (rot & 1) ? sy - 1 : sx - 1);
        for (int row = 0; row < 4; row++)
            m[row] = -m[row];
    }
}

Mason Mason::child(int32_t x, int32_t y, int32_t z, int32_t sx, int32_t sy, int rot, bool mirror) const {
    Mason c(zone);
    c.frame(x, y, z, sx, sy, rot, mirror);
    c.scheme = scheme;
    for (float &h : c.scheme.height)
        h = h - (float)z;
    std::array<int32_t, 16> product{};
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            int32_t sum = 0;
            for (int k = 0; k < 4; k++)
                sum += m_[k * 4 + row] * c.m_[col * 4 + k];
            product[col * 4 + row] = sum;
        }
    c.m_ = product;
    return c;
}

Mason::World Mason::world(int32_t x, int32_t y, int32_t z) const {
    const std::array<int32_t, 16> &m = m_;
    return {m[0] * x + m[4] * y + m[8] * z + m[12], m[1] * x + m[5] * y + m[9] * z + m[13],
            m[2] * x + m[6] * y + m[10] * z + m[14]};
}

int Mason::facing(int d) const {
    if (m_[0] == 1 || m_[0] == -1)
        return m_[5] == 1 ? d : (d + 2) % 4;
    return m_[4] == 1 ? (d + 3) % 4 : (d + 1) % 4;
}

bool Mason::columnInZone(int32_t x, int32_t y) const {
    const World w = world(x, y, 0);
    return zone.inZone(w.x, w.y);
}

// --- §2 ----------------------------------------------------------------------------------------

void Mason::oneColour(Colour c, int p) {
    for (int i = 0; i < 3; i++)
        scheme.colour[i] = c, scheme.pattern[i] = p, scheme.height[i] = 0.0f;
}

void Mason::twoColours(Colour low, int pl, float ul, Colour high, int ph, float uh, bool relative) {
    scheme.colour[0] = low, scheme.pattern[0] = pl, scheme.height[0] = ul;
    for (int i = 1; i < 3; i++)
        scheme.colour[i] = high, scheme.pattern[i] = ph, scheme.height[i] = uh;
    scheme.relative = relative;
}

void Mason::slots(Colour c0, int p0, float u0, Colour c1, int p1, float u1, Colour c2, int p2, float u2) {
    scheme.colour[0] = c0, scheme.pattern[0] = p0, scheme.height[0] = u0;
    scheme.colour[1] = c1, scheme.pattern[1] = p1, scheme.height[1] = u1;
    scheme.colour[2] = c2, scheme.pattern[2] = p2, scheme.height[2] = u2;
    scheme.relative = false;
}

Colour Mason::patterned(Colour c, int p, int32_t x, int32_t y, int32_t z) const {
    switch (p) {
    case pattern::STONE: {
        const World w = world(x, y, z);
        const int32_t sx = (int32_t)((uint32_t)((w.z + w.x) / 2) * 154u + (uint32_t)w.z * 53u);
        const int32_t sy = (int32_t)((uint32_t)((w.z + w.y) / 2) * 154u + (uint32_t)w.z * 53u);
        const float n = simplex((double)sx, (double)sy);
        if (n > 0.7f)
            return scaled(c, (n - 0.7f) / 0.3f * 0.1f + 1.0f);
        return c;
    }
    case pattern::RIVER_BED: {
        const World w = world(x, y, z);
        const BlockProps b = riverBedColour(w.x, w.y, w.z, zone.terrain.sand);
        return {b.r, b.g, b.b};
    }
    case pattern::STRIPES_X:
    case pattern::STRIPES_Y:
    case pattern::STRIPES_Z: {
        const int32_t v = p == pattern::STRIPES_X ? x : p == pattern::STRIPES_Y ? y : z;
        const float n = simplex((double)wrapMul(v, 77), (double)wrapMul(scheme.param, 97));
        return scaled(c, n * 0.15f + 1.0f);
    }
    case pattern::PATCHES: {
        const float n = simplex((double)wrapAdd(wrapMul(x, 77), scheme.param), (double)wrapAdd(wrapMul(y, 97), scheme.param));
        return scaled(c, n * 0.15f + 1.0f);
    }
    default: return c; // 0, and 2 (not specified)
    }
}

Colour Mason::colourAt(int32_t x, int32_t y, int32_t z) const {
    const Scheme &s = scheme;
    float h = (float)z;
    if (s.relative) {
        const World w = world(x, y, 0);
        h = h - groundHeight(w.x, w.y, heightmode::NO_BEDS);
    }
    auto slot = [&](int i) { return patterned(s.colour[i], s.pattern[i], x, y, z); };
    auto blend = [](Colour a, Colour b, float f) {
        const float g = 1.0f - f;
        return Colour{a.r * f + b.r * g, a.g * f + b.g * g, a.b * f + b.b * g};
    };
    if (s.height[0] >= h)
        return slot(0);
    if (s.height[1] > h) {
        if (s.height[0] >= s.height[1])
            return slot(1);
        const float f = (h - s.height[0]) / (s.height[1] - s.height[0]);
        return blend(slot(1), slot(0), f);
    }
    if (s.colour[2].r == 0.0f && s.colour[2].g == 0.0f && s.colour[2].b == 0.0f)
        return slot(1);
    if (!(s.height[2] > h) || s.height[1] >= s.height[2])
        return slot(2);
    const float f = (h - s.height[1]) / (s.height[2] - s.height[1]);
    return blend(slot(2), slot(1), f);
}

Block Mason::block(int32_t x, int32_t y, int32_t z, uint8_t blockType, uint8_t flags) const {
    return packed(colourAt(x, y, z), 0, blockType, flags);
}

// --- §3 ----------------------------------------------------------------------------------------

void Mason::put(int32_t x, int32_t y, int32_t z, Block b) {
    const World w = world(x, y, z);
    if (waterLine > 0) {
        const float top = noise2(w.x, w.y, 0.05, 9076, 8998) * 10.0f + (float)waterLine +
                          groundHeight(w.x, w.y, heightmode::NO_BEDS);
        if ((float)w.z > top)
            return;
    }
    if (writeMode != 0) {
        const Block g = zone.read(w.x, w.y, w.z);
        if ((writeMode & writemode::NOT_SOLID) && !isOpen(g))
            return;
        if ((writeMode & writemode::NOT_PROTECTED) && (g.flags & 2))
            return;
        if ((writeMode & writemode::ONLY_PROTECTED) && !(g.flags & 2))
            return;
    }
    zone.store(w.x, w.y, w.z, b);
}

Block Mason::get(int32_t x, int32_t y, int32_t z) const {
    const World w = world(x, y, z);
    return zone.read(w.x, w.y, w.z);
}

int32_t Mason::groundBelow(int32_t x, int32_t y, int32_t z) const {
    for (;; z--) {
        const Block b = get(x, y, z);
        if (!isOpen(b) && !(b.flags & 1))
            return z;
    }
}

bool Mason::caps(int32_t x, int32_t y, int32_t z, int capMode) const {
    const World w = world(x, y, z);
    switch (capMode) {
    case 1: return false;
    case 2: return noise2(w.x, w.y, 0.06, 7631, 983) > 0.0f;
    case 3: {
        const double k = (double)wrapMul(w.z, 53);
        return simplex((double)((w.z + w.x) / 2 * 2) * 0.1 + k, (double)((w.z + w.y) / 2 * 2) * 0.1 + k) > -0.3f;
    }
    default: return true;
    }
}

Block Mason::capBlock(int32_t x, int32_t y, int32_t z, int capKind, uint8_t flags) const {
    const World w = world(x, y, z);
    switch (capKind) {
    case 2: return packed(grassColour(w.x, w.y), 0, blocktype::GRASS, flags);
    case 8: {
        const uint8_t v = ((y / 4 + x / 4) & 1) ? 127 : 50;
        return {v, v, v, 0, blocktype::ROCK, flags};
    }
    case 9:
        if (!isOpen(get(x, y, z)) && !isOpen(get(x, y, z + 1)))
            return block(x, y, z, blocktype::ROCK, flags);
        return capBlock(x, y, z, 6, flags);
    case 6: {
        const BlockProps p = surfaceColour(w.x, w.y, w.z, colourInputs(columnValues(w.x, w.y, 0)));
        return toBlock({p.r, p.g, p.b, p.glow, p.type, (uint8_t)(p.flags | flags)});
    }
    default: return block(x, y, z, type, flags); // 0 and 7 (others are not specified)
    }
}

void Mason::putCap(int32_t x, int32_t y, int32_t z, int capKind) {
    // The surface cap (kind 6, and kind 9 off the rock) is stored directly, without the put rules.
    const bool direct = capKind == 6 || (capKind == 9 && !(!isOpen(get(x, y, z)) && !isOpen(get(x, y, z + 1))));
    const Block cap = capBlock(x, y, z, capKind, 2);
    if (direct) {
        const World w = world(x, y, z);
        zone.store(w.x, w.y, w.z, cap);
    } else {
        put(x, y, z, cap);
    }
}

void Mason::column(int32_t x, int32_t y, int32_t from, int32_t z, int32_t h, int capKind, int capMode, uint8_t bodyType) {
    const bool capped = caps(x, y, z + h - 1, capMode);
    const int32_t end = capped ? z + h : z + h - 1;
    for (int32_t bz = from; bz < end; bz++) {
        if (capped && bz == end - 1)
            putCap(x, y, bz, capKind);
        else
            put(x, y, bz, block(x, y, bz, bodyType, 2));
    }
}

// --- §4 ----------------------------------------------------------------------------------------

void Mason::box(int32_t x0, int32_t y0, int32_t z0, int32_t sx, int32_t sy, int32_t sz) {
    for (int32_t x = x0; x < x0 + sx; x++)
        for (int32_t y = y0; y < y0 + sy; y++) {
            if (!columnInZone(x, y))
                continue;
            for (int32_t z = z0; z < z0 + sz; z++)
                put(x, y, z, block(x, y, z, type, 2));
        }
}

void Mason::foundation(int32_t x0, int32_t y0, int32_t z, int32_t sx, int32_t sy, int32_t h, int capKind, int capMode) {
    for (int32_t x = x0; x < x0 + sx; x++)
        for (int32_t y = y0; y < y0 + sy; y++)
            if (columnInZone(x, y))
                column(x, y, groundBelow(x, y, z), z, h, capKind, capMode, type);
}

namespace {
// The round shapes' test: the column's centre within r of (x0, y0).
float roundDistance(int32_t x, int32_t y, int32_t x0, int32_t y0) {
    const float dx = (float)(x - x0) + 0.5f, dy = (float)(y - y0) + 0.5f;
    return dx * dx + dy * dy;
}
} // namespace

void Mason::pillar(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h, int capKind, int capMode) {
    for (int32_t x = x0 - r; x <= x0 + r; x++)
        for (int32_t y = y0 - r; y <= y0 + r; y++) {
            if (!columnInZone(x, y) || roundDistance(x, y, x0, y0) >= (float)(r * r))
                continue;
            column(x, y, groundBelow(x, y, z), z, h, capKind, capMode, blocktype::ROCK);
        }
}

void Mason::disc(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h, int capKind, int capMode) {
    for (int32_t x = x0 - r; x <= x0 + r; x++)
        for (int32_t y = y0 - r; y <= y0 + r; y++) {
            if (!columnInZone(x, y) || roundDistance(x, y, x0, y0) >= (float)(r * r))
                continue;
            column(x, y, z, z, h, capKind, capMode, type);
        }
}

void Mason::cone(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h) {
    for (int32_t x = x0 - r; x <= x0 + r; x++)
        for (int32_t y = y0 - r; y <= y0 + r; y++) {
            if (!columnInZone(x, y))
                continue;
            const float d2 = roundDistance(x, y, x0, y0);
            if (!(d2 < (float)(r * r)))
                continue;
            const int32_t top = trunc32((1.0f - std::sqrt(d2) / (float)r) * (float)h + (float)(z + 1));
            for (int32_t bz = z; bz < top; bz++)
                put(x, y, bz, block(x, y, bz, type, 0));
        }
}

void Mason::liquid(int32_t x0, int32_t y0, int32_t z0, int32_t sx, int32_t sy, int32_t sz, uint8_t flags, int kind) {
    for (int32_t x = x0; x < x0 + sx; x++)
        for (int32_t y = y0; y < y0 + sy; y++) {
            if (!columnInZone(x, y))
                continue;
            for (int32_t z = z0; z < z0 + sz; z++) {
                if (!isOpen(get(x, y, z)))
                    continue;
                const World w = world(x, y, z);
                Block b;
                if (kind == 1) {
                    b = {0x7f, 0x7f, 0x7f, 0, blocktype::WATER, flags};
                } else if (kind == 2) {
                    const float n = noise2(w.x, w.y, 0.1, 0, 0) + noise2(w.x, w.y, 0.05, 0, 0);
                    b = packed({255.0f, n * 50.0f + 50.0f, 20.0f}, 255, blocktype::LAVA, flags);
                } else {
                    const float n = noise2(w.x, w.y, 0.06, 0, 0) * 20.0f + 120.0f;
                    b = packed({30.0f, n, 40.0f}, 0, blocktype::POISON, flags);
                }
                put(x, y, z, b);
            }
        }
}

void Mason::air(int32_t x0, int32_t y0, int32_t z0, int32_t sx, int32_t sy, int32_t sz, uint8_t flags) {
    const Block b{0, 0, 0, 0, 0, (uint8_t)(flags | 2)};
    for (int32_t x = x0; x < x0 + sx; x++)
        for (int32_t y = y0; y < y0 + sy; y++) {
            if (!columnInZone(x, y))
                continue;
            for (int32_t z = z0; z < z0 + sz; z++)
                put(x, y, z, b);
        }
}

void Mason::lightDisc(int32_t x0, int32_t y0, int32_t z, int32_t r, int32_t h, uint8_t flags) {
    for (int32_t x = x0 - r; x <= x0 + r; x++)
        for (int32_t y = y0 - r; y <= y0 + r; y++) {
            if (!columnInZone(x, y) || !(roundDistance(x, y, x0, y0) < (float)(r * r)))
                continue;
            const World w = world(x, y, 0);
            const float n = maxOf((noise2(w.x, w.y, 0.05, 0, 0) * 1.5f + 0.5f) * 127.5f, 0.0f);
            const Block b{0, 0, 0, (uint8_t)trunc32(n), 0, flags};
            for (int32_t bz = z; bz < z + h; bz++)
                put(x, y, bz, b);
        }
}

void Mason::tube(bool alongX, int32_t x0, int32_t y0, int32_t z0, int32_t z1, int32_t len, int32_t r) {
    const int32_t start = alongX ? x0 : y0, c = alongX ? y0 : x0;
    int32_t rise = 0;
    for (int32_t a = start; a < start + len; a++, rise += z1 - z0) {
        const int32_t zc = z0 + rise / len;
        for (int32_t b = c - r; b <= c + r; b++) {
            const int32_t x = alongX ? a : b, y = alongX ? b : a;
            if (!columnInZone(x, y))
                continue;
            for (int32_t z = zc - r; z <= zc + r; z++) {
                const float dz = (float)(z - zc) - 0.5f, db = (float)(b - c) - 0.5f;
                if (!(dz * dz + db * db <= (float)(r * r)))
                    continue;
                const float f = (float)(a % 2) * 0.1f + 0.9f;
                put(x, y, z, packed(scaled(colourAt(x, y, z), f), 0, type, 0));
            }
        }
    }
}

void Mason::hangingCone(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h) {
    for (int32_t bz = z - h; bz <= z + 1; bz++) {
        const float rr = (float)(bz - z + h) / (float)h * (float)r;
        for (int32_t bx = trunc32((float)x - rr - 1.5f); (float)bx <= (float)x + rr + 1.5f; bx++)
            for (int32_t by = trunc32((float)y - rr - 1.5f); (float)by <= (float)y + rr + 1.5f; by++) {
                if (!columnInZone(bx, by))
                    continue;
                const World w = world(bx, by, bz);
                const float a = simplex((double)w.y * 0.1, (double)bz * 0.1) * 1.5f + (float)(bx - x);
                const float b = simplex((double)w.x * 0.1, (double)bz * 0.1) * 1.5f + (float)(by - y);
                if (a * a + b * b <= rr * rr)
                    put(bx, by, bz, block(bx, by, bz, blocktype::ROCK, 0x22));
            }
    }
}

void Mason::stripedWall(int32_t x, int32_t y, int32_t z, int32_t w, int32_t h) {
    for (int32_t j = 0; j < w; j++) {
        const int32_t bx = x - w / 2 + j;
        for (int32_t bz = z; bz < z + h; bz++) {
            const float f = (float)(bz % 2) * 0.1f + 0.9f;
            put(bx, y, bz, packed(scaled(colourAt(bx, y, bz), f), 0, 16, 0));
        }
    }
}

void Mason::squarePost(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, bool grounded) {
    const int32_t x0 = x - r, y0 = y - r, s = 2 * r + 2;
    if (grounded)
        foundation(x0 - 1, y0 - 1, z, s, s, 2, 0, 0);
    else
        box(x0 - 1, y0 - 1, z, s, s, 2);
    box(x0, y0, z + 2, 2 * r, 2 * r, h - 2);
    box(x0 - 1, y0 - 1, z + h - 2, s, s, 2);
}

void Mason::cappedPost(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, bool grounded) {
    const int32_t x0 = x - r, y0 = y - r, s = 2 * r + 2;
    if (grounded)
        foundation(x0 - 1, y0 - 1, z, s, s, 2, 0, 0);
    else
        box(x0 - 1, y0 - 1, z, s, s, 2);
    box(x0, y0, z + 2, 2 * r, 2 * r, h - 2);
    box(x0 - 1, y0 - 1, z + h - 3, s, s, 3);
    box(x0 - 2, y0 - 2, z + h - 1, 2 * r + 4, 2 * r + 4, 1);
}

void Mason::roundPost(int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, bool grounded) {
    if (grounded)
        pillar(x, y, z, r + 1, 2, 0, 0);
    else
        disc(x, y, z, r + 1, 2, 0, 0);
    disc(x, y, z + 2, r, h - 2, 0, 0);
    disc(x, y, z + h - 3, r + 1, 3, 0, 0);
    disc(x, y, z + h - 1, r + 2, 1, 0, 0);
}

void Mason::battlements(bool alongX, int32_t x0, int32_t y0, int32_t z, int32_t len, int32_t w, int32_t h) {
    for (int32_t i = 0; i < len; i++) {
        if ((i / w) % 2 == 1)
            continue;
        for (int32_t k = 0; k < w; k++) {
            const int32_t x = alongX ? x0 + i : x0 + k, y = alongX ? y0 + k : y0 + i;
            if (!columnInZone(x, y))
                continue;
            for (int32_t bz = z; bz < z + h; bz++)
                put(x, y, bz, block(x, y, bz, blocktype::ROCK, 0));
        }
    }
}

void Mason::ramp(bool alongX, int32_t x0, int32_t y0, int32_t z, int32_t sx, int32_t sy, int32_t a, int32_t b, int capKind,
                 int capMode) {
    for (int32_t x = x0; x < x0 + sx; x++)
        for (int32_t y = y0; y < y0 + sy; y++) {
            if (!columnInZone(x, y))
                continue;
            int32_t g = z - 1;
            while (isOpen(get(x, y, g)))
                g--;
            g += 1;
            const int32_t t = alongX ? x - x0 : y - y0, n = alongX ? sx : sy;
            const int32_t top = z + t * (b - a) / n + a;
            const bool capped = caps(x, y, top - 1, capMode);
            const int32_t end = capped ? top : top - 1;
            for (int32_t bz = g; bz < end; bz++)
                if (capped && bz == end - 1)
                    putCap(x, y, bz, capKind);
                else
                    put(x, y, bz, block(x, y, bz, type, 2));
        }
}

void Mason::spiralRing(int32_t x0, int32_t y0, int32_t z, int32_t rIn, int32_t rOut, int32_t h) {
    for (int32_t x = x0 - rOut; x <= x0 + rOut; x++)
        for (int32_t y = y0 - rOut; y <= y0 + rOut; y++) {
            if (!columnInZone(x, y))
                continue;
            const float fx = (float)(x - x0) - 0.5f, fy = (float)(y - y0) - 0.5f;
            const float d2 = fy * fy + fx * fx;
            if (d2 >= (float)(rOut * rOut) || (float)(rIn * rIn) > d2)
                continue;
            const float a = std::acos(fx / std::sqrt(d2));
            int32_t g = z;
            for (;; g--) {
                const Block k = get(x, y, g);
                if (!isOpen(k) && !(k.flags & 1))
                    break;
            }
            const int32_t top = trunc32((double)((float)h * a) / 3.141592653589793 + (double)z);
            for (int32_t bz = g; bz < top; bz++)
                put(x, y, bz, block(x, y, bz, type, 0));
        }
}

} // namespace worldgen
