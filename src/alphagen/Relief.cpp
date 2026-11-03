// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
// The zone post passes between the surface pass and the tree grid
// (docs/specs/alpha-worldgen-caves.md): rock mounds and mesas, the great mound and the giant
// tree, roads, terraces with their pools, and the boulders. Section numbers point at that spec.
// The structure passes that follow (§8) are in Structures.cpp, the dungeon builder in
// Dungeon.cpp.
#include "alphagen/Terrain.h"
#include <cmath>

namespace alphagen {
namespace {

float f(double v) {
    return (float)v;
}

Block toBlock(glm::vec3 c, uint8_t typeByte) {
    return {(uint8_t)(int32_t)c.r, (uint8_t)(int32_t)c.g, (uint8_t)(int32_t)c.b, typeByte};
}

constexpr Block AIR_WRITE{255, 255, 255, AIR};

// §0: within d blocks of the home point.
bool nearHome(int32_t x, int32_t y, float d) {
    const float dx = (float)x - (float)HOME, dy = (float)y - (float)HOME;
    return dx * dx + dy * dy < d * d;
}

// §3: whether location L's centre lies in zone (zx, zy).
bool centreIn(const Location &L, int32_t zx, int32_t zy) {
    return floorDiv((int32_t)(L.x >> 16), ZONE) == zx && floorDiv((int32_t)(L.y >> 16), ZONE) == zy;
}

} // namespace

// §1, §2: the noisy top of a formation's column.
int32_t formationTop(int32_t X, int32_t Y, int32_t z, int32_t rz, bool mesa) {
    const float n = noise(0.01 * X + 4394.0, 0.01 * Y + 8974.0);
    return (int32_t)f(f(n * (mesa ? (float)rz : 20.f)) + (float)(z + rz));
}

// §1, §2: the (u, v) of a cell against a formation centred on (x, y); inside when
// v^2 + u^2 <= 1.
glm::vec2 formationUV(FormationTest test, int32_t X, int32_t Y, int32_t Z, int32_t x, int32_t y, int32_t rx, int32_t ry) {
    const double zk = test == FormationTest::MesaFill ? 0.02 : 0.05;
    const float nu = noise(0.05 * Y, zk * Z), nv = noise(0.05 * X + 4374.0, zk * Z + 9898.0);
    float su, sv;
    switch (test) {
    case FormationTest::Mound:
        su = f(10.f / (float)rx);
        sv = f(10.f / (float)ry);
        break;
    case FormationTest::MesaGuard:
        su = sv = 0.4f;
        break;
    default:
        su = sv = 0.5f;
        break;
    }
    return {f(nu * su) + f((float)(X - x) / (float)rx), f(nv * sv) + f((float)(Y - y) / (float)ry)};
}

// §5.1 step 5: the road shade before the town mix and the region style.
float roadShade(int32_t x, int32_t y) {
    float C = 140.f + 40.f * noise(0.05 * x + 843.0, 0.05 * y + 984.0);
    const int32_t a = 234 * (x / 2), b = 234 * (y / 2);
    if (noise((double)a + 4834.0, (double)b + 49434.0) > 0.5f)
        C += 20.f * noise((double)a, (double)b + 49.0);
    return C;
}

// §5.2 steps 2-4.
Terrace terraceAt(float h) {
    Terrace T;
    T.zb = 5 * ((int32_t)h / 5);
    T.t = f(h - (float)T.zb) / 5.f;
    if (T.t < 0.5f) {
        T.a = 2.f * T.t;
    } else {
        T.a = 1.f - 4.f * (T.t - 0.5f);
        if (T.a < 0)
            T.a = (T.a + 1.f) * (T.a + 1.f) - 1.f;
    }
    const float raised = f((float)T.zb - 5.f * T.a);
    T.zw = T.a >= 0 ? T.zb : (int32_t)raised;
    T.zlow = (int32_t)f(raised + 2.f);
    return T;
}

// §1 and §2: a rock mound or a layered mesa on top of the ground.
void Terrain::formation(Zone &zone, int32_t x, int32_t y, int32_t z, int32_t rx, int32_t ry, int32_t rz, bool mesa) const {
    const FormationTest guard = mesa ? FormationTest::MesaGuard : FormationTest::Mound;
    const FormationTest fill = mesa ? FormationTest::MesaFill : FormationTest::Mound;
    auto inside = [&](FormationTest test, int32_t X, int32_t Y, int32_t Z) {
        const glm::vec2 uv = formationUV(test, X, Y, Z, x, y, rx, ry);
        return f(uv.y * uv.y + uv.x * uv.x) <= 1.f;
    };
    // Phase 1: never build over structure blocks (bit 0x80, water aside). The block is read
    // first since it is cheap and the test has no side effects.
    for (int32_t X = x - 2 * rx; X <= x + 2 * rx; X++)
        for (int32_t Y = y - 2 * ry; Y <= y + 2 * ry; Y++) {
            if (!zone.contains(X, Y))
                continue;
            const int32_t bottom = zone.column(X, Y).bottom;
            for (int32_t Z = formationTop(X, Y, z, rz, mesa); Z >= bottom; Z--) {
                const Block b = zone.read(X, Y, Z);
                if (b.type() != WATER && (b.typeByte & KEEP) && inside(guard, X, Y, Z))
                    return;
            }
        }
    // Phase 2: the fill.
    for (int32_t X = x - 2 * rx; X <= x + 2 * rx; X++)
        for (int32_t Y = y - 2 * ry; Y <= y + 2 * ry; Y++) {
            if (!zone.contains(X, Y))
                continue;
            const int32_t b0 = zone.column(X, Y).bottom;
            const int32_t zt = formationTop(X, Y, z, rz, mesa);
            for (int32_t Z = zt; Z >= b0; Z--) {
                const Block b = zone.read(X, Y, Z);
                const uint8_t t = b.type();
                const bool open = t == AIR || t == WATER || t == GRASS || t == SAND || t == 5;
                if (!open || (b.typeByte & DRY_AIR) || !inside(fill, X, Y, Z))
                    continue;
                if (Z == zt) {
                    zone.place(X, Y, Z, surface(X, Y, Z, &zone));
                } else if (mesa) {
                    const float s = sat(f((float)(Z - b0 - 8) * 0.05f));
                    const glm::vec3 c = cliff(X, Y, Z, &zone) * (1.f - s) + rock(X, Y, Z, &zone) * s;
                    zone.place(X, Y, Z, toBlock(c, 5));
                } else {
                    zone.place(X, Y, Z, rockBlock(X, Y, Z, zone));
                }
            }
        }
}

// §6.2, §6.3: a small noisy ellipsoid of rock round a seed.
void Terrain::boulder(Zone &zone, const BoulderSeed &seed, int32_t ax, int32_t ay, int32_t az, bool fromRavine) const {
    for (int32_t X = seed.x - ax; X <= seed.x + ax; X++)
        for (int32_t Y = seed.y - ay; Y <= seed.y + ay; Y++) {
            if (!zone.contains(X, Y))
                continue;
            const float n = f(noise(0.05 * X, 0.05 * Y) * 0.8f);
            const float ux = f((float)(X - seed.x) / (float)ax), uy = f((float)(Y - seed.y) / (float)ay);
            const float s2 = f(uy * uy + ux * ux);
            for (int32_t Z = seed.z + 2 * az; Z >= seed.z - 2 * az; Z--) {
                const float wz = f(f((float)(Z - seed.z) / (float)az) + n);
                if (f(wz * wz + s2) > 1.f)
                    continue;
                const Block b = zone.read(X, Y, Z);
                if ((b.typeByte & KEEP) || (!fromRavine && (b.typeByte & DRY_AIR)))
                    continue;
                zone.place(X, Y, Z, toBlock(rock(X, Y, Z, &zone), fromRavine ? (uint8_t)(ROCK | SURFACE_COLOURED) : ROCK),
                           [&](int32_t gz) { return rockBlock(X, Y, gz, zone); });
            }
        }
}

bool Terrain::reliefPasses(Zone &zone, const std::vector<float> &heights, Random &random, std::vector<BoulderSeed> &terraceBoulders,
                           std::string &error) const {
    const int32_t zx = zone.zx, zy = zone.zy;
    const int32_t x0 = zone.originX(), y0 = zone.originY();
    const Location *Lz = w.location(x0, y0);
    const ZoneEntry *entry = w.zoneEntry(zx, zy);
    const ZoneEntry E = entry ? *entry : ZoneEntry{};
    auto H = [&](int32_t x, int32_t y) { return heights[(x - x0) * 257 + (y - y0)]; };
    auto top = [&](int32_t x, int32_t y) { return zone.column(x, y).top(); };
    auto put = [&](int32_t x, int32_t y, int32_t z, Block b) {
        zone.place(x, y, z, b, [&, x, y](int32_t gz) { return rockBlock(x, y, gz, zone); });
    };

    // This boundary stops before Alpha dungeons. Until the tree builder is ported,
    // refuse its special location instead of publishing incomplete terrain.
    if (Lz && Lz->type == 12 && centreIn(*Lz, zx, zy)) {
        error = "Alpha giant-tree terrain requires the voxel-tree builder";
        return false;
    }

    // §3.1: the zone-centre climate.
    const Column &middle = zone.column(x0 + 128, y0 + 128);
    const float mc = middle.moisture, wc = middle.warmth;
    const bool climateBonus = (wc > 0.6f || wc < 0.3f) && (mc > 0.7f || (mc < 0.4f && (mc > 0.2f || wc < 0.8f)));

    // §3.2: type 6, the 3 x 3 mound grid round the valley centre.
    if (Lz && Lz->type == 6)
        for (int32_t a = 0; a < 768; a += 256)
            for (int32_t b = 0; b < 768; b += 256) {
                if ((random.draw() & 3) == 0)
                    continue;
                const int32_t X = x0 + a / 3 + 42, Y = y0 + b / 3 + 42;
                const float v = 1.f - w.locationDistance(*Lz, X, Y);
                if (v <= 0 || v * v < 0.5f)
                    continue;
                const int32_t rx = random.draw() % 10 + 20;
                const int32_t ry = random.draw() % 10 + 20;
                const int32_t rz = (random.draw() & 15) + 20;
                if (nearHome(X, Y, 60.f))
                    continue;
                formation(zone, X, Y, top(X, Y), rx, ry, rz, false);
            }
    // (§3.3, types 13 and 4: a search whose result is unused.)

    // §4: the great mound, the giant tree, or the rock pass.
    const int32_t cx = x0 + 128, cy = y0 + 128;
    if (Lz && Lz->type == 11 && centreIn(*Lz, zx, zy)) {
        const int32_t rz = 100 + random.draw() % 100;
        formation(zone, cx, cy, top(cx, cy), 100, 100, rz, false);
    } else if (E.kind != 1 && E.kind != 3 && E.kind != 4) {
        int32_t n = random.draw() & 3;
        if (climateBonus)
            n += 2;
        for (int32_t i = 0; i < n; i++) {
            const int32_t rx = random.draw() % 40 + 10;
            const int32_t ry = random.draw() % 40 + 10;
            const int32_t rz = random.draw() % 25 + 10;
            const int32_t x = 2 * (128 * zx + rx) + random.draw() % (4 * (64 - rx));
            const int32_t y = 2 * (128 * zy + ry) + random.draw() % (4 * (64 - ry));
            if (nearHome(x, y, 60.f) || townness(x, y) > 0.6f || !(1.f - 50.f * roughness(x, y) < 0))
                continue;
            const float m = mountains(x, y);
            const int32_t zt = top(x, y);
            if ((random.draw() & 1) != 0 && m < 0.25f)
                formation(zone, x, y, zt, rx, ry, rz / 2, true);
            else
                formation(zone, x, y, zt, rx, ry, rz, false);
        }
    }

    // §5.1: roads along the link channels and the town streets.
    for (int32_t x = x0; x < x0 + Zone::SIZE; x++)
        for (int32_t y = y0; y < y0 + Zone::SIZE; y++) {
            const float s = 1.f - 50.f * flatness(x, y);
            if (s < 0)
                continue;
            const float q = townness(x, y);
            const float h = std::max(H(x, y), 0.f);
            const int32_t z1 = (int32_t)(h + 1.f);
            const float e = 1.f - (1.f - s) * (1.f - s) * (1.f - s);
            const float clearance = noise(0.02 * x + 55432.0, 0.02 * y + 974.0);
            int32_t zc = (int32_t)f(f((clearance + 1.f) * 4.f) + f(f(e * 5.f) + h));
            const Block ground = zone.read(x, y, z1);
            if (ground.type() == AIR || ground.type() == WATER || (ground.typeByte & DRY_AIR))
                continue;
            const glm::vec3 cl = cliff(x, y, z1, &zone);
            glm::vec3 C(roadShade(x, y));
            const Region *region = w.region(floorDiv(x, REGION), floorDiv(y, REGION));
            if (region && (region->style == 1 || region->style == 4 || region->style == 5))
                C = cl;
            const glm::vec3 colour = glm::clamp(cl * (1.f - q) + C * q, glm::vec3(0), glm::vec3(255));
            put(x, y, z1 - 1, toBlock(colour, TOWN_GROUND));
            if (q > 0.92f)
                zc = top(x, y);
            for (int32_t z = z1; z < zc; z++)
                if (!(zone.read(x, y, z).typeByte & DRY_AIR))
                    put(x, y, z, AIR_WRITE);
        }

    // §5.2: terraces with pools on smooth ground.
    for (int32_t x = x0; x < x0 + Zone::SIZE; x++)
        for (int32_t y = y0; y < y0 + Zone::SIZE; y++) {
            const float s = 1.f - 50.f * roughness(x, y);
            if (s < 0 || townness(x, y) > 0.95f)
                continue;
            const float h = std::max(H(x, y), 0.f);
            const Terrace T = terraceAt(h);
            for (int32_t z = T.zlow; z <= T.zb; z++)
                if (!(zone.read(x, y, z).typeByte & DRY_AIR))
                    put(x, y, z, Block{0, 0, (uint8_t)(int32_t)(255.f * (1.f - T.a) + 0.f * T.a), WATER});
            const Block floor = zone.read(x, y, T.zw);
            if (floor.type() != WATER && !(floor.typeByte & DRY_AIR)) {
                put(x, y, T.zw, toBlock(rock(x, y, T.zw, &zone), LAKE_BED));
                if (random.draw() % 200 == 0)
                    terraceBoulders.push_back({x, y, T.zw});
            }
            const float e = 1.f - (1.f - s) * (1.f - s) * (1.f - s);
            const float clearance = noise(0.02 * x + 55432.0, 0.02 * y + 974.0);
            const int32_t zc = (int32_t)f(f((clearance + 1.f) * 2.f) + f(f(e * 5.f) + h));
            for (int32_t z = T.zw + 1; z < zc; z++)
                if (!(zone.read(x, y, z).typeByte & DRY_AIR))
                    put(x, y, z, AIR_WRITE);
        }

    return true;
}

} // namespace alphagen
