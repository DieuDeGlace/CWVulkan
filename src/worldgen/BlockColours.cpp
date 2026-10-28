// The terrain's block colours, written from docs/specs/worldgen-terrain-zone.md §4 (sections
// cited as §n). Single precision, one rounded operation at a time, in the spec's order.
#include "worldgen/BlockColours.h"

#include "worldgen/Noise.h"

#include <cmath>

namespace worldgen {
namespace {

constexpr float TINT = 0.0010000000474974513f; // 0.001: below this a region's tint is skipped
constexpr int WARM[] = {4, 9, 10, 13};
constexpr int COLD[] = {6, 11};

// The two tint noises the forest and kind-27 tints share.
float tintP(int32_t x, int32_t y) { return noiseAt(x, y, 0.01, 0, 0.01, 0) + 1.0f; }
float tintQ(int32_t x, int32_t y) { return noiseAt(x, y, 0.006, 4343, 0.006, 983) + 1.0f; }

} // namespace

Block toBlock(const BlockProps &p) {
    auto byte = [](float c) -> uint8_t {
        c = c * 0.5f;
        if (0.0f > c)
            c = 0.0f;
        else if (c > 255.0f)
            c = 255.0f;
        return (uint8_t)trunc64(c);
    };
    return {byte(p.r), byte(p.g), byte(p.b), p.glow, p.type, p.flags};
}

// --- §4.1 ---

float warmth(const Neighbourhood &n, int kind, float w) {
    float warm = regionWeight(n, WARM, 4, 0.0004f);
    float cold = regionWeight(n, COLD, 2, 0.0004f);
    if (kind == 19) {
        warm = warm + w;
        cold = cold * (1.0f - w);
    } else if (kind == 14 || kind == 18) {
        warm = warm + w * 0.25f;
        cold = cold * (1.0f - w);
    }
    float v = noiseAt(n.x, n.y, 0.0002, 4234234, 0.0002, 55543) * 0.35f;
    v = v + ((warm + warm) + 0.5f);
    v = v - cold;
    if (!(1.0f > v))
        v = 1.0f;
    return 0.0f < v ? v : 0.0f;
}

ColourInputs colourInputs(const ColumnValues &c) {
    ColourInputs in;
    in.highlands = c.highlands;
    in.warmth = warmth(c.regions, c.place.kind, c.place.weight);
    in.dryness = c.dryness;
    in.vegetation = c.vegetation;
    in.roads = c.roads;
    in.lavaRegion = c.lavaRegion;
    in.forestRegion = c.forestRegion;
    in.kind27 = kind27Weight(c.place.kind, c.place.weight);
    in.lava = c.lava;
    in.swamp = c.swamp;
    return in;
}

// --- §4.5 ---

float sandShade(int32_t x, int32_t y, const SandOffsets &sand) {
    return (simplex(sand.x + (double)x * 0.03, sand.y + (double)y * 0.03) + 1.0f) * 0.5f * 70.0f;
}

BlockProps groundColour(int32_t x, int32_t y, int32_t z, const ColourInputs &c, const SandOffsets &sand) {
    // 1-2. Earth: a broad shade and a finer one.
    const float t = (noiseAt(x, y, 0.015, 1883, 0.015, 98984) + 1.0f) * 0.5f;
    float b = t * 0.0f + 10.0f;
    float g = t * 150.0f + 160.0f;
    float r = t * 150.0f + 260.0f;
    const float k = noiseAt(x, y, 0.01, 431, 0.01, 873) * 30.0f + 30.0f;
    r = k + r, g = k + g, b = k + b;
    // 4. Sand near sea level.
    if (z < 100) {
        float s = maxOf(0.0f, (float)z / 100.0f);
        s = s * s;
        const float q = sandShade(x, y, sand);
        const float inv = 1.0f - s;
        r = inv * ((q + 340.0f) - r) + r;
        g = inv * ((q + 250.0f) - g) + g;
        b = inv * ((q + 100.0f) - b) + b;
    }
    // 5. Sand where it is warm and dry.
    const float m = minOf(1.0f - c.warmth, c.dryness);
    if (0.3f > m) {
        const float w = minOf(1.0f, m / 0.3f);
        const float r1 = r * w, g1 = g * w, b1 = b * w;
        const float q = sandShade(x, y, sand);
        const float iw = 1.0f - w;
        g = iw * (q + 250.0f) + g1;
        r = iw * (q + 340.0f) + r1;
        b = iw * (q + 100.0f) + b1;
    }
    // 6. Frozen ground where it is cold.
    if (0.2f > c.warmth) {
        float cw = maxOf(0.0f, 1.0f - c.warmth * 5.0f);
        const float i = 1.0f - cw;
        const float r1 = r * i, g1 = g * i, b1 = b * i;
        const BlockProps rock = rockColour(x, y, z, c.warmth, c.lavaRegion, c.forestRegion);
        cw = cw * 0.5f;
        r = (rock.r + 200.0f) * cw + r1;
        g = (rock.g + 280.0f) * cw + g1;
        b = (rock.b + 400.0f) * cw + b1;
    }
    // 7. Forest soil.
    if (c.forestRegion > TINT) {
        const float f = c.forestRegion;
        const float p = tintP(x, y) * 20.0f, q = tintQ(x, y) * 20.0f;
        const float inv = 1.0f - f;
        const float g1 = inv * g, r1 = inv * r, b1 = inv * b;
        b = ((p + 128.0f) + q) * f + b1;
        r = ((p * 0.5f + 128.0f) - q * 0.5f) * f + r1;
        g = (128.0f - q * 0.5f) * f + g1;
    }
    // 8. Kind-27 places.
    if (c.kind27 > TINT) {
        const float kk = c.kind27;
        const float p = tintP(x, y) * 100.0f, q = tintQ(x, y) * 50.0f;
        const float inv = 1.0f - kk;
        const float r1 = r * inv, g1 = g * inv, b1 = b * inv;
        g = kk * 128.0f + g1;
        r = ((p + 128.0f) - q * 0.5f) * kk + r1;
        b = ((p * 0.5f + 128.0f) + q) * kk + b1;
    }
    // 9. Lava regions: toward their glowing rock.
    if (c.lavaRegion > TINT) {
        const float l = c.lavaRegion;
        const float il = 1.0f - l;
        const float r1 = r * il, g1 = g * il, b1 = b * il;
        const BlockProps rock = rockColour(x, y, z, c.warmth, c.lavaRegion, c.forestRegion);
        r = rock.r * l + r1;
        g = rock.g * l + g1;
        b = rock.b * l + b1;
    }
    return {r, g, b, 0, blocktype::GROUND, 0};
}

BlockProps rockColour(int32_t x, int32_t y, int32_t z, float warm, float lavaR, float forestR) {
    const double zd = (double)z * 0.4, xd = (double)x * 0.1, yd = (double)y * 0.1;
    // 1. Layers by height.
    float v = simplex(yd + 98984.0, zd);
    v = v + simplex(xd, zd);
    v = (v * 0.5f + 1.0f) * 0.5f;
    // 2. Grey, reddened where it is hot.
    float r = 210.0f, g = 210.0f, b = 210.0f;
    if (!(0.5f > warm) && warm > 0.6f) {
        const float t = minOf(1.0f, (warm - 0.6f) / 0.3f);
        r = t * 110.0f + 210.0f;
        g = t * 10.0f + 210.0f;
        b = t * -130.0f + 210.0f;
    }
    // 3. The layers' shade.
    v = v * 0.3f;
    r = v * r + r, g = v * g + g, b = v * b + b;
    // 4. Darker in forests (green twice).
    if (forestR > TINT) {
        const float k = 1.0f - forestR * 0.25f;
        const float m = forestR * 0.15f;
        r = r * k, g = g * k, b = b * k;
        r = (1.0f - m) * r;
        g = k * g;
    }
    // 5. Glowing veins in lava regions.
    if (lavaR > TINT) {
        float s = noiseAt(x, y, 0.04, 8773, 0.04, 99808) * 0.5f;
        s = s + noiseAt(x, y, 0.02, 0, 0.02, 0);
        const float u = 1.0f - absOf(minOf(1.0f, s));
        const float il = 1.0f - lavaR;
        const float u5 = u * (((u * u) * u) * u);
        r = r * il, g = g * il, b = b * il;
        const float glowR = u5 * 300.0f + 30.0f;
        const float n = simplex(xd, yd) + 1.0f;
        r = glowR * lavaR + r;
        g = ((n * u5) * 50.0f + 30.0f) * lavaR + g;
        b = lavaR * 30.0f + b;
    }
    return {r, g, b, 0, blocktype::ROCK, 0};
}

// --- §4.3, §4.4 ---

BlockProps buriedColour(int32_t x, int32_t y, int32_t z, const ColourInputs &c, const SandOffsets &sand) {
    if (c.highlands > 0.5f)
        return rockColour(x, y, z, c.warmth, c.lavaRegion, c.forestRegion);
    return groundColour(x, y, z, c, sand);
}

BlockProps surfaceColour(int32_t x, int32_t y, int32_t z, const ColourInputs &c) {
    // 1. Grass.
    float a = (noiseAt(x, y, 0.005, 343, 0.005, 9233) + 1.0f) * 0.5f;
    a = a * a;
    const float bb = (noiseAt(x, y, 0.02, 6654, 0.02, 778) + 1.0f) * 0.5f;
    const float ib = 1.0f - bb;
    float g = bb * 260.0f + ib * (200.0f - a * 50.0f);
    float r = (220.0f - a * 100.0f) * bb + ib * (80.0f - a * 60.0f);
    float b = bb * 30.0f + ib * (a * 80.0f + 40.0f);
    r = r + 0.0f, g = g + 0.0f, b = b + 0.0f;
    uint8_t type = blocktype::GRASS;
    // 2. Yellower where it is dry.
    const float d = (0.5f - c.dryness) * 80.0f;
    r = r + d;
    if (c.dryness > 0.5f)
        b = b - d;
    // 3. Forest.
    if (c.forestRegion > TINT) {
        const float f = c.forestRegion;
        const float p = tintP(x, y) * 30.0f, q = tintQ(x, y) * 30.0f;
        const float inv = 1.0f - f;
        const float newR = ((p + 128.0f) - q) * f + inv * r;
        const float newB = (maxOf(q, p) + 128.0f) * f + inv * b;
        g = inv * g + f * 128.0f;
        r = newR, b = newB;
    }
    // 4. Kind-27 places.
    if (c.kind27 > TINT) {
        const float k = c.kind27;
        const float p = tintP(x, y) * 20.0f, q = tintQ(x, y) * 100.0f;
        const float inv = 1.0f - k;
        const float r1 = inv * r, g1 = inv * g, b1 = inv * b;
        g = g1 + k * 60.0f;
        r = r1 + ((p + 128.0f) - q * 0.5f) * k;
        b = b1 + ((p * 0.5f + 128.0f) + q) * k;
    }
    // 5. Lava regions: their rock, and lava ground in the middle.
    if (c.lavaRegion > TINT) {
        const float l = c.lavaRegion;
        const float il = 1.0f - l;
        const float r1 = il * r, g1 = il * g, b1 = il * b;
        const BlockProps rock = rockColour(x, y, z, c.warmth, l, c.forestRegion);
        r = rock.r * l + r1;
        g = g1 + rock.g * l;
        b = rock.b * l + b1;
        if (l > 0.5f)
            type = blocktype::LAVA_GROUND;
    }
    return {r, g, b, 0, type, 0};
}

// --- §4.2 ---

BlockProps blockAt(int32_t x, int32_t y, int32_t z, float h, const NeighbourHeights &nb, const ColourInputs &c,
                   const SandOffsets &sand) {
    // 1-2. Lava and poison channels.
    if (c.lava > 0.5f) {
        const float g = (noiseAt(x, y, 0.1, 0, 0.1, 0) + noiseAt(x, y, 0.05, 0, 0.05, 0)) * 50.0f + 50.0f;
        return {255.0f, g, 20.0f, 255, blocktype::LAVA, 0};
    }
    if (c.swamp > 0.5f) {
        const float g = noiseAt(x, y, 0.06, 0, 0.06, 0) * 20.0f + 120.0f;
        return {30.0f, g, 40.0f, 255, blocktype::POISON, 0};
    }
    // 3-4. Under the ground on every side, on roads and vegetation, and at low shores: the
    // buried colour (the unused tint input is always 0).
    const int32_t fz = floorToInt(h);
    const bool buried = z < fz && z < floorToInt(nb.west) && z < floorToInt(nb.east) && z < floorToInt(nb.south) &&
                        z < floorToInt(nb.north);
    if (buried || c.vegetation > 0.5f || c.roads > 0.5f || lowShore(x, y, h)) {
        BlockProps p = buriedColour(x, y, z, c, sand);
        if (c.roads > 0.5f)
            p.type = blocktype::ROAD;
        return p;
    }
    // 5. The surface, blended toward the buried colour on steep sides.
    float lo = minOf(nb.west - h, 0.0f), hi = maxOf(nb.west - h, 0.0f);
    lo = minOf(nb.east - h, lo), hi = maxOf(nb.east - h, hi);
    lo = minOf(nb.south - h, lo), hi = maxOf(nb.south - h, hi);
    lo = minOf(nb.north - h, lo), hi = maxOf(nb.north - h, hi);
    const BlockProps top = surfaceColour(x, y, z, c);
    const bool belowDrop = z < fz && -1.0f > lo;
    if (!(belowDrop || hi > 1.0f))
        return top;
    const float steep = belowDrop ? -lo : hi;
    const float t = maxOf(0.0f, minOf((steep - 1.0f) * 3.0f, 1.0f));
    const BlockProps side = buriedColour(x, y, z, c, sand);
    const float it = 1.0f - t;
    BlockProps p;
    p.r = side.r * t + top.r * it;
    p.g = side.g * t + top.g * it;
    p.b = side.b * t + top.b * it;
    p.glow = top.glow;
    p.type = t > 0.5f ? side.type : top.type;
    p.flags = t > 0.5f ? side.flags : top.flags;
    return p;
}

// --- §4.6, §4.7, §4.9 ---

bool snowAt(int32_t x, int32_t y, float warm, const NeighbourHeights &nb) {
    if (!(0.3f > warm))
        return false;
    if (!(std::fabs((double)(nb.west - nb.east)) < 2.0 && std::fabs((double)(nb.south - nb.north)) < 2.0))
        return false;
    if (0.1f > warm)
        return true;
    return noiseAt(x, y, 0.05, 8411122, 0.05, 884646) * 0.05f + 0.2f > warm;
}

BlockProps riverBedColour(int32_t x, int32_t y, int32_t z, const SandOffsets &sand) {
    const Neighbourhood n = neighbourhood(x, y);
    const Place p = placeForColumn(cellPlace(floorDiv(x, PLACE_SHIFT), floorDiv(y, PLACE_SHIFT)), x, y);
    ColourInputs c;
    c.warmth = warmth(n, p.kind, p.weight);
    c.dryness = dryness(n);
    c.lavaRegion = lavaRegion(n, p.kind, p.weight);
    c.forestRegion = forestRegion(n, p.kind, p.weight);
    c.kind27 = kind27Weight(p.kind, p.weight);
    BlockProps bed = groundColour(x, y, z, c, sand);
    bed.type = blocktype::WET_GROUND;
    return bed;
}

float waterLevel(float base, const Place &p) {
    switch (p.kind) {
    case 0: case 10: case 17: case 19: case 21: case 28: case 36: // mask 0x10102a0400
        return base;
    default: break;
    }
    float w = p.weight;
    if (p.kind == 8 || p.kind == 13 || p.kind == 14)
        w = smooth(minOf(1.0f, maxOf(0.0f, w + w)));
    if (w > 9.999999747378752e-05f) {
        const float own = maxOf(placeHeight(p), 10.0f);
        base = base + (own - base) * w;
    }
    return base;
}

} // namespace worldgen
