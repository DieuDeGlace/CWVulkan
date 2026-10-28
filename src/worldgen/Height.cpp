// The height of a point, written from docs/specs/worldgen-terrain-height.md (sections cited as
// §n). Single precision, one rounded operation at a time, in the spec's order; "in double"
// where the spec says so. None of it draws random numbers.
#include "worldgen/Height.h"

#include "NativeCrt.h"
#include "worldgen/Noise.h"

#include <cmath>

namespace worldgen {
namespace {

constexpr double TWO_PI = 6.283185307179586;
constexpr double PI = 3.141592653589793;
constexpr float DOT = 1.0f / 65536.0f;
constexpr float TINY_WEIGHT = 9.999999747378752e-05f; // 0.0001 as a float
constexpr float REGION_SCALE = 0.0004f;

// Kind sets (tables §6).
bool flatKind(int kind) { return kind == 17 || kind == 21 || kind == 28 || kind == 36; } // mask 0x1010220000
bool blendedKind(int kind) { return kind == 8 || kind == 13 || kind == 14 || kind == 24; } // mask 0x1006100
bool roadClearedKind(int kind) { // mask 0x3c02e6400
    switch (kind) {
    case 10: case 13: case 14: case 17: case 18: case 19: case 21: case 30: case 31: case 32: case 33: return true;
    default: return false;
    }
}

// --- Region sets (§3) ---
constexpr int OCEAN[] = {biomes::OCEAN};
constexpr int LAKE[] = {biomes::LAKE};
constexpr int WET[] = {biomes::WET};
constexpr int DRY[] = {9, 13};
constexpr int LAVA[] = {biomes::LAVA};
constexpr int FOREST[] = {5, 12};
constexpr int DESERT[] = {10};
constexpr int WARM_WET[] = {4, 8};

// --- Channels (§5.1) ---
struct Layer {
    double scale, ox, oy;
};
struct Channel {
    Layer coarse, broad, fine; // weighted 0.5, 1 and 0.1
};
constexpr Channel RIVER = {{0.0025, 23123, 42432}, {0.0005, 23123, 42432}, {0.01, 23123, 42432}};
constexpr Channel RIVER_WIDTH = {{0.0025, 53455, 6547}, {0.0005, 5553445, 123321}, {0.01, 6455445, 33223}};
constexpr Channel LAVA_FLOW = {{0.0023, 330003, 323322}, {0.00045, 88989, 1001}, {0.01, 558587, 198743}};
constexpr Channel SWAMP = {{0.0025, 5253, 809834}, {0.0005, 6664, 7567567}, {0.01, 80084, 5664}};

float layer(int32_t x, int32_t y, const Layer &l) { return noiseAt(x, y, l.scale, l.ox, l.scale, l.oy); }

float channel(int32_t x, int32_t y, const Channel &c) {
    float v = layer(x, y, c.coarse) * 0.5f;
    v = v + layer(x, y, c.broad);
    v = v + layer(x, y, c.fine) * 0.1f;
    return absOf(v);
}

// How full a channel is from its noise: 1 in the middle, 0 away from it.
float fill(float v) {
    v = v * 5.0f;
    if (v > 0.5f) {
        const float t = (v - 0.5f) * 0.5f;
        v = t * t * t + 0.5f;
    }
    const float f = maxOf(1.0f - v, 0.0f);
    return f * f;
}

// A place dries channels toward its centre. Written as the game computes it (the value is e).
float dries(float w) {
    const float q = clamp01((w - 0.1f) * 10.0f);
    const float e = (1.0f - q) * (1.0f - q);
    return 1.0f - (1.0f - e);
}

// A water or lava ring around a variant-4 place.
float ring(float r, const Place &p) {
    if (!((float)(p.radius * p.radius) > p.dist2))
        return r;
    const float radius = (float)p.radius;
    float t = absOf((std::sqrt(p.dist2) - radius * 0.75f) / (radius * 0.25f));
    const float keep = 1.0f - r;
    t = minOf(t, 1.0f);
    const float f = 1.0f - t * t;
    return r + f * f * keep;
}

// A place filled with the channel at its centre (kinds 13, 14).
float centreFill(float r, float w) {
    const float t = (w - 0.4f) / 0.6f;
    return r + (1.0f - r) * (t > 0.0f ? t : 0.0f);
}

// Roads dry channels.
float roadsDry(float r, float road) {
    const float c = clamp01((road - 0.4f) * 10.0f);
    return (1.0f - c) * r;
}

int centreBiome(const Place &p) { return biomeAt(p.point.x, p.point.y); }

// --- Roads (§4) ---

// The road's nearness along the link from landmark a to landmark b, for the point (dots).
float segment(int64_t px, int64_t py, Int2 a, Int2 b) {
    const float vx = (float)(px - (int64_t)a.x * 65536) * DOT;
    const float vy = (float)(py - (int64_t)a.y * 65536) * DOT;
    const float ex = (float)(b.x - a.x), ey = (float)(b.y - a.y);
    const float dot = ex * vx + ey * vy;
    float d2;
    if (!(0.0f < dot)) {
        d2 = vy * vy + vx * vx;
    } else {
        const float len2 = ey * ey + ex * ex;
        if (dot >= len2) {
            const float wx = (float)(px - (int64_t)b.x * 65536) * DOT;
            const float wy = (float)(py - (int64_t)b.y * 65536) * DOT;
            d2 = wy * wy + wx * wx;
        } else {
            const float t = dot / len2;
            const float ay = vy - ey * t;
            const float ax = vx - ex * t;
            d2 = ax * ax + ay * ay;
        }
    }
    float f = 1.0f - minOf(1.0f, d2 / (250.0f * 250.0f));
    f = f * f;
    f = f * f;
    f = f * f;
    f = f * f;
    return 1.0f - f;
}

constexpr uint32_t ROAD_EAST_WEST_SALT = 0x81d65;
constexpr uint32_t ROAD_NORTH_SOUTH_SALT = 0x286926;

// Whether a road links from a cell (hashed at its even corner when big).
bool linked(int32_t cx, int32_t cy, bool big, uint32_t salt) {
    if (big)
        cx = 2 * floorDiv(cx, 1), cy = 2 * floorDiv(cy, 1);
    return oddHash((uint32_t)cy * 57u + salt + (uint32_t)cx);
}

Int2 landmarkOf(int32_t cx, int32_t cy) { return cellPoint(cx, cy, bigCell(cx, cy)); }

// --- Final height helpers (§8) ---

// Fine grain on highland peaks (0x2b5670), in double.
float grain(int32_t x, int32_t y) {
    double v = (double)noiseAt(x, y, 0.08, 432, 0.08, 432) * 0.05;
    v = v + (double)noiseAt(x, y, 0.04, 432, 0.04, 432);
    const float r = (float)(1.0 - std::fabs(v));
    const float e = 1.0f - r;
    return e * e + 0.0f;
}

// The spiral ramp (variant 2) or pit (variant 3) of a place (§8.5).
float spiral(int32_t x, int32_t y, const Place &p, float h) {
    const float a = (float)p.radius * 0.6f;
    if (!(a * a > p.weight)) // the game tests the weight here, not the distance
        return h;
    const LandmarkWarp w = landmarkWarp(x, y);
    const float dx = (float)(((int64_t)x - p.point.x) * 65536 + trunc64(w.x * 65536.0f)) * DOT;
    const float dy = (float)(((int64_t)y - p.point.y) * 65536 + trunc64(w.y * 65536.0f)) * DOT;
    const float length = std::sqrt(dy * dy + dx * dx);
    const float ux = dx / length, uy = dy / length;
    float angle = nativecrt::acosf(ux);
    if (0.0f > uy)
        angle = (float)(TWO_PI - (double)angle);
    // Where the spiral starts, which way it turns and how many blocks one turn is, by cell.
    int32_t cx = floorDiv(x, PLACE_SHIFT), cy = floorDiv(y, PLACE_SHIFT);
    if (p.big)
        cx = 2 * floorDiv(cx, 1), cy = 2 * floorDiv(cy, 1);
    const uint32_t k = (uint32_t)cy * 57u;
    float start = (float)(random((uint32_t)cx + 0x1966ab2cu + k) % 1000000u) / 1000000.0f;
    start = start + start;
    float theta = (float)((double)start * PI + (double)angle);
    if ((double)theta > TWO_PI)
        theta = (float)((double)theta - TWO_PI);
    if (oddHash((uint32_t)cx + 0x711bec9fu + k))
        theta = (float)(TWO_PI - (double)theta);
    const float perTurn = (float)(random((uint32_t)cx + 0x4e3e1cu + k) % 20u + 25u);
    const float cap = minOf(a / perTurn * 15.0f, 150.0f);
    const float scale = a / 6.0f;
    float turns = (float)((double)a - (double)theta / TWO_PI * (double)perTurn);
    turns = (turns - length) / perTurn;
    if (turns < -0.25f)
        return h;
    const float whole = (float)floorToInt(turns);
    const float part = turns - whole - 0.75f;
    float s = part > 0.0f ? smooth(minOf(1.0f, part * 4.0f)) : 0.0f;
    s = s + whole;
    if (s > 0.0f)
        theta = (float)((double)s * TWO_PI + (double)theta);
    if (0.0f > turns)
        theta = theta * (turns * 4.0f + 1.0f);
    if (1.0f > theta)
        theta = theta * theta;
    float r = std::sqrt(theta) * scale;
    r = minOf(cap, r);
    return p.variant == 3 ? h - r : h + r;
}

// A walled pit (variant 5, §8.5).
float walledPit(int32_t x, int32_t y, const Place &p, float h) {
    const float radius = (float)p.radius;
    const int32_t inner = trunc32(radius * 0.3f), outer = trunc32(radius * 0.75f);
    if (!((float)(outer * outer) > p.dist2))
        return h;
    auto wallAt = [&] { return noiseAt(x, y, 0.02, 98453466, 0.02, 8743) * 25.0f + 30.0f; };
    const float wall = wallAt();
    const float o = (float)outer;
    const float gap = o - wall;
    if (p.dist2 > gap * gap) {
        const float t = (o - std::sqrt(p.dist2)) / wall;
        return h - t * t * 100.0f;
    }
    if (p.dist2 > (float)(inner * inner)) {
        float t = std::sqrt(p.dist2) - (float)inner;
        t = t / wallAt();
        t = minOf(t, 1.0f);
        return h - t * t * 100.0f;
    }
    return h;
}

// Cliff steps (§8.8): the ground stepped down toward the height before the place shapes.
float cliffSteps(int32_t x, int32_t y, float h, float before, float g) {
    const float top = (noiseAt(x, y, 0.01, 7474, 0.01, 98984) + 1.0f) * 20.0f + before;
    const float slope = noiseAt(x, y, 0.02, 8744, 0.02, 904) * 0.025f + 0.06f;
    float at = h;
    if (h > before) {
        for (;;) {
            at = (float)floorToInt(at);
            const float m = minOf(absOf((at - top) * slope), 1.0f);
            float s = smooth(1.0f - (1.0f - m * m) * g);
            s = s * ((h - at) * 0.1f + 0.5f);
            if (s > 0.5f)
                break;
            at = at + -1.0f;
            if (!(at > before))
                break;
        }
    }
    return at;
}

} // namespace

// --- §2 ---

float baseHeight(const Neighbourhood &n, int kind, float w) {
    const float ocean = nearestOf(n, OCEAN, 1);
    const float lakeD = nearestOf(n, LAKE, 1);
    float h = 100.0f;
    if (ocean >= 0.0f && ocean >= n.nearestDist2) {
        const float t = (ocean - n.nearestDist2) * 1.4901161193847656e-08f;
        const float a = maxOf(1.0f - t, 0.0f) * 4.0f;
        const float s = minOf(3.0f - a, 1.0f);
        h = 0.0f > s ? -smooth(-s) * 100.0f : smooth(s) * 100.0f;
    }
    if (lakeD >= 0.0f && lakeD >= n.nearestDist2) {
        const float t = ((lakeD - n.nearestDist2) * 0.0003f) * 0.0003f;
        const float u = maxOf(1.0f - t, 0.0f);
        const float nn = noiseAt(n.x, n.y, 0.0015, 6667, 0.0015, 33322) + 1.0f;
        h = h + nn * (smooth(u) * 100.0f);
    }
    h = h + noiseAt(n.x, n.y, 0.001, 247412, 0.001, 8128321) * 20.0f;
    if (kind == 28) {
        h = h - w * 50.0f;
    } else if (kind == 31) {
        h = h + w * 50.0f;
    } else if (kind != 0 && w > 0.0f && 1.0f > h) {
        const float ww = minOf(w + w, 1.0f);
        h = h + smooth(ww) * (1.0f - h);
    }
    return h;
}

float plainBaseHeight(int32_t x, int32_t y) { return baseHeight(neighbourhood(x, y), 0, 0.0f); }

float placeHeight(const Place &p) { return baseHeight(neighbourhood(p.point.x, p.point.y), p.kind, 1.0f); }

float baseHeightWithPlace(int32_t x, int32_t y) {
    const Neighbourhood n = neighbourhood(x, y);
    const Place p = placeAt(x, y);
    return baseHeight(n, p.kind, p.weight);
}

// --- §3 ---

float lakeRegion(const Neighbourhood &n) { return regionWeight(n, LAKE, 1, REGION_SCALE); }
float wetRegion(const Neighbourhood &n) { return regionWeight(n, WET, 1, REGION_SCALE); }
float dryRegion(const Neighbourhood &n) { return regionWeight(n, DRY, 2, 0.0003f); }

float lavaRegion(const Neighbourhood &n, int kind, float w) {
    const float v = regionWeight(n, LAVA, 1, REGION_SCALE);
    if (v >= 1.0f)
        return v;
    if (kind == 18)
        return minOf(1.0f, (w + w) + v);
    if (kind == 19)
        return minOf(1.0f, v + w);
    return v;
}

float forestRegion(const Neighbourhood &n, int kind, float w) {
    const float v = regionWeight(n, FOREST, 2, REGION_SCALE);
    if (v >= 1.0f)
        return v;
    if (kind == 26)
        return minOf(1.0f, v + w);
    return v;
}

float dryness(const Neighbourhood &n) {
    const float a = regionWeight(n, DRY, 2, REGION_SCALE);
    const float b = regionWeight(n, DESERT, 1, REGION_SCALE);
    const float c = regionWeight(n, WARM_WET, 2, REGION_SCALE);
    float v = noiseAt(n.x, n.y, 0.0002, 6654, 0.0002, 98883) * 0.35f;
    v = v + (c * 1.5f + 0.5f);
    v = v - b * 0.4f;
    v = v - (a + a);
    if (!(1.0f > v))
        v = 1.0f;
    return 0.0f < v ? v : 0.0f;
}

// --- §4 ---

float roads(int32_t x, int32_t y, int kind, float w) {
    // 1. The point moved along the roads' own warp (less inside a place).
    const float a = noiseAt(x, y, 0.0025, 2346, 0.005, 64565) * 128.0f;
    const float a2 = noiseAt(x, y, 0.005, 4234, 0.01, 5434545) * 32.0f;
    const float b = noiseAt(x, y, 0.005, 23432, 0.0025, 2342344) * 128.0f;
    const float b2 = noiseAt(x, y, 0.01, 54356, 0.005, 7677);
    const float keep = 1.0f - w;
    const int64_t px = trunc64((a + a2) * keep * 65536.0f) + (int64_t)x * 65536;
    const int64_t py = trunc64((b + b2 * 32.0f) * keep * 65536.0f) + (int64_t)y * 65536;
    // 2. Its place cell (the dots divided truncating toward zero).
    const int32_t cx0 = floorDiv((int32_t)(px / 65536), PLACE_SHIFT);
    const int32_t cy0 = floorDiv((int32_t)(py / 65536), PLACE_SHIFT);
    // 3. Every road between the landmarks of the 3 x 3 cells around it.
    float road = 1.0f;
    for (int32_t cx = cx0 - 1; cx <= cx0 + 1; cx++)
        for (int32_t cy = cy0 - 1; cy <= cy0 + 1; cy++) {
            const bool big = bigCell(cx, cy);
            const Int2 here = cellPoint(cx, cy, big);
            float cell = 1.0f;
            struct Link {
                int32_t hx, hy;
                bool hashBig;
                uint32_t salt;
                int32_t ox, oy;
            };
            const Link links[4] = {
                {cx, cy, big, ROAD_EAST_WEST_SALT, cx - 1, cy},
                {cx + 1, cy, bigCell(cx + 1, cy), ROAD_EAST_WEST_SALT, cx + 1, cy},
                {cx, cy, big, ROAD_NORTH_SOUTH_SALT, cx, cy - 1},
                {cx, cy + 1, bigCell(cx, cy + 1), ROAD_NORTH_SOUTH_SALT, cx, cy + 1},
            };
            for (const Link &l : links) {
                if (!linked(l.hx, l.hy, l.hashBig, l.salt))
                    continue;
                const Int2 there = landmarkOf(l.ox, l.oy);
                if (there != here)
                    cell = cell * segment(px, py, here, there);
            }
            road = road * cell;
        }
    // 5. Places wear or clear the road.
    if (kind == 5) {
        if (w > TINY_WEIGHT) {
            const float c = minOf(1.0f, maxOf(0.0f, (w - 0.4f) * 10.0f));
            float e = 1.0f - c;
            const float n = noiseAt(x, y, 0.01, 0, 0.01, 0);
            e = e * e;
            const float inner = 1.0f - e;
            float m = minOf(absOf(n) + absOf(n), 1.0f);
            m = m * m;
            m = m * m;
            const float v = (1.0f - m) * inner;
            road = road * (1.0f - v);
        }
    } else if (roadClearedKind(kind)) {
        road = 1.0f - (1.0f - road) * keep;
    }
    // 6. Sharpened: 1 on the road, 0 away from it.
    road = road * 120.0f;
    if (road > 0.5f)
        road = (road - 0.5f) / 119.5f * 0.5f + 0.5f;
    return maxOf(1.0f - road, 0.0f);
}

// --- §5 ---

float river(int32_t x, int32_t y, float roadsV, float lavaR, float forestR, float wet, const Place &p) {
    float r = 0.0f;
    if (0.5f > lavaR && 0.5f > forestR) {
        float v = channel(x, y, RIVER);
        if (wet > 9.999999747378752e-06f)
            v = v * (1.0f - (1.0f - channel(x, y, RIVER_WIDTH)) * wet);
        r = (1.0f - minOf(lavaR + lavaR, 1.0f)) * fill(v);
        r = r * (1.0f - minOf(forestR + forestR, 1.0f));
    }
    if (p.kind != 0) {
        if (p.kind == 13) {
            r = centreFill(r, p.weight);
        } else {
            r = r * dries(p.weight);
            if (p.variant == 4 && centreBiome(p) != biomes::LAVA)
                r = ring(r, p);
        }
    }
    return roadsDry(r, roadsV);
}

float lava(int32_t x, int32_t y, float roadsV, float lavaR, float lake, const Place &p) {
    float r = 0.0f;
    if (lavaR > 0.5f) {
        const float f = fill(channel(x, y, LAVA_FLOW));
        r = f * ((lavaR - 0.5f) + (lavaR - 0.5f));
        r = r * (1.0f - lake);
    }
    if (p.kind != 0) {
        if (p.kind == 14) {
            r = centreFill(r, p.weight);
        } else if (p.kind == 19) {
            float k = (float)p.radius * 0.3f;
            k = k * k;
            if (k > p.dist2)
                r = r + (1.0f - p.dist2 / k);
        } else {
            r = r * dries(p.weight);
            if (p.variant == 4 && centreBiome(p) == biomes::LAVA)
                r = ring(r, p);
        }
    }
    return roadsDry(r, roadsV);
}

float swamp(int32_t x, int32_t y, float roadsV, float forestR, float lake, const Place &p) {
    float r = 0.0f;
    if (forestR > 0.5f) {
        const float f = fill(channel(x, y, SWAMP));
        r = f * ((forestR - 0.5f) + (forestR - 0.5f));
        r = r * (1.0f - lake);
    }
    if (p.kind != 0)
        r = r * dries(p.weight);
    return roadsDry(r, roadsV);
}

// --- §6 ---

float highlands(int32_t x, int32_t y, float base, float lake, float riverV, float lavaV, float swampV, float roadsV,
                const Place &p) {
    const float n1 = noiseAt(x, y, 0.0015, 565, 0.0015, 8877);
    float amount = maxOf(0.0f, (n1 + n1) + (lake + 1.0f));
    const float m1 = noiseAt(x, y, 0.001, 34234, 0.001, 3245);
    const float m = m1 * noiseAt(x, y, 0.0015, 545, 0.0015, 654665);
    const float c = noiseAt(x, y, 0.006, 55545, 0.006, 6566) * 0.2f + (m + m);
    float ridge = maxOf(1.0f - absOf(c), 0.0f);
    float flat = 1.0f;
    const float w = p.weight;
    if (p.kind != 0 && w > 0.0f) {
        if (p.kind == 10 || p.kind == 19) {
            const float n5 = noiseAt(x, y, 0.025, 84333, 0.025, 98111) + 1.0f;
            const float add = n5 * (w * 0.5f) * (1.0f - ridge);
            amount = amount + (w + w);
            ridge = ridge + add;
        } else if (!flatKind(p.kind)) {
            const float s = minOf(1.0f, maxOf(0.0f, w + w));
            flat = 1.0f - smooth(s);
        }
    }
    float hb = (base + 80.0f) * 0.05f;
    if (!(hb > 0.0f))
        hb = 0.0f;
    if (!(0.0f < amount))
        return 0.0f;
    const float a = minOf(1.0f, amount);
    float keep = (1.0f - minOf(lavaV + lavaV, 1.0f)) * (1.0f - minOf(riverV + riverV, 1.0f));
    keep = keep * (1.0f - minOf(swampV + swampV, 1.0f));
    keep = keep * (1.0f - minOf(roadsV + roadsV, 1.0f));
    keep = keep * flat;
    const float q = 1.0f - keep;
    if (!(hb < 1.0f))
        hb = 1.0f;
    float v = (1.0f - smooth(q)) * smooth(a);
    v = v * (hb * ridge); // hb and ridge first: the spec's "v × hb × ridge" misses 8 oracle points by an ulp
    if (v > 0.5f) {
        const float t = v - 0.5f;
        float u = t * 4.0f;
        if (!(1.0f > u))
            u = 1.0f;
        if (0.0f >= u)
            u = 0.0f;
        const float g = cliffs(x, y, p.kind, p.weight);
        u = u * u;
        float e = 1.0f - smooth(g);
        e = e * e;
        const float k = (1.0f - e) * (1.0f - u);
        v = (1.0f - k) * t + 0.5f;
    }
    return v;
}

// --- §7 ---

float vegetation(int32_t x, int32_t y, float dry, float dryReg) {
    const float n1 = noiseAt(x, y, 0.01, 647646, 0.01, 84373);
    double v = ((double)n1 + (1.5 - (double)dry)) * 0.30000001192092896;
    v = v + (double)dryReg;
    v = v + (double)noiseAt(x, y, 0.02, 43234, 0.02, 5345345) * 0.2;
    v = v + (double)noiseAt(x, y, 0.05, 77878, 0.05, 9888) * 0.1;
    return (float)v;
}

// --- §8 ---

float cliffs(int32_t x, int32_t y, int kind, float w) {
    const bool peak = kind == 10 || kind == 19;
    if (peak && w > 0.5f)
        return 0.0f;
    float a = noiseAt(x, y, 0.1, 7007, 0.1, 6000) * 0.1f;
    a = a + noiseAt(x, y, 0.01, 1838, 0.01, 88233);
    a = absOf(a) * 4.0f;
    const float b = noiseAt(x, y, 0.01, 43255, 0.01, 6678) * 0.25f;
    a = minOf(a, 1.0f);
    const float c = minOf(1.0f, maxOf(0.0f, b + (1.0f - a)));
    const float e = 1.0f - c;
    float r = 1.0f - e * e * e;
    if (peak && w > TINY_WEIGHT)
        r = r * maxOf(1.0f - (w + w), 0.0f);
    return r;
}

bool lowShore(int32_t x, int32_t y, float h) {
    if (!(h < 20.0f))
        return false;
    if (!(0.0f < h))
        return true;
    return (noiseAt(x, y, 0.02, 7743, 0.02, 8977) + 1.0f) * 10.0f > h;
}

float finalHeight(int32_t x, int32_t y, const HeightInputs &in, const Place &p, int mode) {
    const float w = p.weight;
    float base = in.base;
    // 8.1 Rolling ground and the place's shape.
    float rolling = (noiseAt(x, y, 0.005, 3363, 0.005, 7874) + 1.0f) * 25.0f - 10.0f;
    bool lowPlace = false;
    float shapeKeep = 1.0f;  // F: how much of the terraces the place keeps
    float rollKeep = 1.0f;   // G: how much of the rolling ground
    if (p.kind != 0 && w > TINY_WEIGHT) {
        float own = placeHeight(p);
        if (10.0f > own)
            own = 10.0f, lowPlace = true;
        const float s = minOf(1.0f, w + w);
        const float sm = smooth(s);
        if (p.kind == 10) { // a mountain
            if (!(mode & heightmode::LOW))
                base = w * w * 180.0f + base;
        } else if (p.kind == 18) { // a crater
            const float radius = (float)p.radius;
            const float t = minOf(w + w, 1.0f);
            const float inner = maxOf((w - 0.5f) + (w - 0.5f), 0.0f);
            shapeKeep = 1.0f - sm;
            const float rim = t * t * 0.1f * radius + base;
            const float e = (1.0f - inner) * (1.0f - inner);
            base = rim - (1.0f - e) * radius * 0.25f;
        } else if (p.kind == 19) { // a volcano
            const float a = minOf(w / 0.85f, 1.0f);
            const float b = maxOf((w - 0.85f) / 0.15f, 0.0f);
            shapeKeep = 1.0f - sm;
            const float cone = a * a * a * 180.0f + base;
            const float e = (1.0f - b) * (1.0f - b);
            base = cone - (1.0f - e) * 20.0f;
        } else if (!flatKind(p.kind)) {
            shapeKeep = 1.0f - sm;
            if (rolling > 10.0f)
                rolling = rolling + (10.0f - rolling) * sm;
            if (blendedKind(p.kind)) {
                rollKeep = shapeKeep;
                base = (own - base) * sm + base;
            } else {
                base = (own - base) * w + base;
            }
        }
    }
    // 8.2 Rolling ground.
    float h = maxOf(-256.0f, base);
    const float r2 = minOf(in.river + in.river, 1.0f), l2 = minOf(in.lava + in.lava, 1.0f);
    const float s2 = minOf(in.swamp + in.swamp, 1.0f), d2 = minOf(in.roads + in.roads, 1.0f);
    float wt = (1.0f - l2) * (1.0f - r2);
    wt = wt * (1.0f - s2);
    const float open = (1.0f - d2 * 0.5f) * wt * rollKeep;
    {
        float hk = h * 0.05f;
        if (!(hk > 0.0f))
            hk = 0.0f;
        float e = (1.0f - open) * (1.0f - open);
        const float n = noiseAt(x, y, 0.015, 4447657, 0.015, 87878);
        e = e * e;
        if (!(hk < 1.0f))
            hk = 1.0f;
        const float k = (1.0f - e) * hk * rolling;
        h = h + (n + 1.0f) * 0.5f * k;
    }
    // 8.3 Terraces.
    {
        float q = h * 0.1f;
        if (!(1.0f > q))
            q = 1.0f;
        float d = noiseAt(x, y, 0.015, 234234, 0.015, 12233);
        d = d + noiseAt(x, y, 0.01, 10943, 0.01, 98433);
        const float v = (1.0f - d2) * wt * shapeKeep;
        const float v2 = 1.0f - (1.0f - v);
        if (!(0.0f < q))
            q = 0.0f;
        d = d * v2 * q;
        if (d > 0.0f && 0.5f > in.highlands) {
            const float cmod = p.kind == 28 ? minOf(1.0f, w + w) : 0.0f;
            const float hm = maxOf(1.0f - (in.highlands + in.highlands), 0.0f);
            auto step = [&](float t, double ox, double oy) {
                const float a = 1.0f - minOf(t * 20.0f, 1.0f);
                float s = noiseAt(x, y, 0.005, ox, 0.005, oy) + cmod;
                s = s > 0.0f ? s : 0.0f;
                if (!(s < 1.0f))
                    s = 1.0f;
                h = h + (1.0f - a * a) * hm * 20.0f * s * s;
            };
            step(d, 7743, 98834);
            if (d > 0.5f)
                step(d - 0.5f, 6777, 8887);
            if (d > 0.75f)
                step(d - 0.75f, 2133, 444);
        }
    }
    const float beforeShapes = h;
    // 8.5 The variant's shape.
    if (!(mode & (heightmode::LOW | heightmode::NO_VARIANT_SHAPES)) && p.kind != 0) {
        if (p.variant == 1) { // a raised plateau
            const float radius = (float)p.radius;
            const float a = radius * 0.7f, b = radius * 0.5f;
            if (a * a > p.dist2) {
                const float span = a - b;
                const float t = (std::sqrt(p.dist2) - b) / span;
                const float u = minOf(1.0f, 1.0f - t);
                h = h + smooth(u) * span * 0.75f;
            }
        } else if (p.variant == 5) {
            if (!lowPlace)
                h = walledPit(x, y, p, h);
        } else if (p.variant == 2 || (p.variant == 3 && !lowPlace)) {
            h = spiral(x, y, p, h);
        }
    }
    // 8.6 Highland peaks.
    if (!(mode & heightmode::LOW) && in.highlands > 0.5f) {
        const float t = minOf(1.0f, maxOf(0.0f, (in.highlands - 0.75f) * 4.0f));
        const float u = (in.highlands - 0.5f) + (in.highlands - 0.5f);
        float g = (1.0f - t) * (t * 4.0f);
        if (!(1.0f > g))
            g = 1.0f;
        const float n9 = noiseAt(x, y, 0.01, 8757, 0.01, 9087);
        h = (n9 * 0.25f + 0.75f) * (u * u * 100.0f) + h;
        if (!(0.0f < g))
            g = 0.0f;
        h = h + grain(x, y) * g * 16.0f;
    }
    // 8.7 River beds and shores.
    if (!(mode & heightmode::NO_BEDS)) {
        if (in.river > 0.5f) {
            const float s = 1.0f - ((in.river - 0.5f) + (in.river - 0.5f));
            float qq = (h + 5.0f) * 0.1f;
            if (!(qq > 0.0f))
                qq = 0.0f;
            const float nn = (noiseAt(x, y, 0.025, 35, 0.025, 8933) + 1.0f) * 0.5f;
            const float s3 = s * s * s;
            if (!(qq < 1.0f))
                qq = 1.0f;
            h = h - (nn * (1.0f - s3) * 9.5f * qq + 2.0f);
        } else if (in.lava > 0.5f || in.swamp > 0.5f) {
            h = h + -2.0f;
        } else if (in.vegetation > 0.5f || in.roads > 0.5f || lowShore(x, y, h)) {
            h = h + -1.0f;
            if (in.highlands > 0.5f)
                h = h + -1.0f;
        }
    }
    // 8.8 Cliff steps.
    if ((mode & heightmode::CLIFF_STEPS) && in.highlands > 0.5f) {
        const float g = cliffs(x, y, p.kind, w);
        if (g > 0.0f)
            h = cliffSteps(x, y, h, beforeShapes, g);
    }
    return h;
}

// --- §1 ---

ColumnValues columnValues(int32_t x, int32_t y, const Place &place, int mode) {
    ColumnValues c;
    c.regions = neighbourhood(x, y);
    c.place = place;
    const int kind = place.kind;
    const float w = place.weight;
    c.lake = lakeRegion(c.regions);
    c.base = baseHeight(c.regions, kind, w);
    c.roads = roads(x, y, kind, w);
    c.lavaRegion = lavaRegion(c.regions, kind, w);
    c.forestRegion = forestRegion(c.regions, kind, w);
    c.wet = wetRegion(c.regions);
    c.river = river(x, y, c.roads, c.lavaRegion, c.forestRegion, c.wet, place);
    c.lava = lava(x, y, c.roads, c.lavaRegion, c.lake, place);
    c.swamp = swamp(x, y, c.roads, c.forestRegion, c.lake, place);
    c.highlands = highlands(x, y, c.base, c.lake, c.river, c.lava, c.swamp, c.roads, place);
    c.dryRegion = dryRegion(c.regions);
    c.dryness = dryness(c.regions);
    c.vegetation = vegetation(x, y, c.dryness, c.dryRegion);
    c.height = finalHeight(x, y, c.heightInputs(), place, mode);
    return c;
}

float groundHeight(int32_t x, int32_t y, int mode) { return columnValues(x, y, mode).height; }

} // namespace worldgen
