// Terrain-only hybrid continuation. Alpha structure builders are intentionally
// absent: Steam layout integration is a separate stage, not a hidden Alpha town.
// Reuses original AlphaVulkan GPL terrain code; see docs/alpha-generation.md.
#include "alphagen/Terrain.h"
#include "alphagen/Trees.h"
#include <algorithm>
namespace alphagen {
namespace {
constexpr int64_t ONE = 65536;
constexpr float TWO_POW_MINUS_16 = 1.f / 65536.f;
float f(double value) {
    return float(value);
}
Block toBlock(glm::vec3 color, uint8_t type) {
    return {uint8_t(int32_t(color.r)), uint8_t(int32_t(color.g)), uint8_t(int32_t(color.b)), type};
}
} // namespace
void Terrain::looseBoulders(Zone &zone, Random &random) const {
    const int32_t zx = zone.zx, zy = zone.zy;
    const auto *entry = w.zoneEntry(zx, zy);
    const ZoneEntry E = entry ? *entry : ZoneEntry{};
    const auto &middle = zone.column(zone.originX() + 128, zone.originY() + 128);
    const float mc = middle.moisture, wc = middle.warmth;
    const bool climateBonus = (wc > 0.6f || wc < 0.3f) && (mc > 0.7f || (mc < 0.4f && (mc > 0.2f || wc < 0.8f)));
    if (E.kind != 1 && E.kind != 3 && E.kind != 4) {
        int32_t n = random.draw() % 10;
        if (climateBonus)
            n += 10;
        for (int32_t b = 0; b < n; b++) {
            const int32_t ax = (random.draw() & 7) + 3;
            const int32_t ay = (random.draw() & 7) + 3;
            const int32_t az = (random.draw() & 7) + 3;
            const int32_t x = 2 * (128 * zx + ax) + random.draw() % (4 * (64 - ax));
            const int32_t y = 2 * (128 * zy + ay) + random.draw() % (4 * (64 - ay));
            const float hy = (float)y - (float)HOME, hx = (float)x - (float)HOME;
            if (f(f(hy * hy) + f(hx * hx)) < 400.f)
                continue;
            bool nearObject = false;
            for (const ObjectMark &o : zone.objectList) {
                const float ey = f((float)((int64_t)y * ONE - o.y) * (1.f / 65536.f));
                const float ex = f((float)((int64_t)x * ONE - o.x) * (1.f / 65536.f));
                nearObject = nearObject || ey * ey + ex * ex < 1600.f;
            }
            if (nearObject || townness(x, y) > 0.25f)
                continue;
            // The ground: just above the highest stored block that is neither air nor water.
            const Column &column = zone.column(x, y);
            int32_t G = column.bottom;
            for (int32_t i = (int32_t)column.blocks.size() - 1; i >= 0; i--)
                if (!clear(column.blocks[i].type())) {
                    G = column.bottom + i + 1;
                    break;
                }
            const Block ground = zone.read(x, y, G - 1);
            const uint8_t gt = ground.type();
            if (gt == TOWN_GROUND || gt == LEAVES_TYPE || gt == WOOD_TYPE || (ground.typeByte & DRY_AIR))
                continue;
            for (int32_t X = x - ax; X <= x + ax; X++) {
                const float u = f((float)(X - x) / (float)ax);
                for (int32_t Y = y - ay; Y <= y + ay; Y++) {
                    if (!zone.contains(X, Y))
                        continue;
                    const float v = f((float)(Y - y) / (float)ay);
                    const float s2 = f(v * v + u * u);
                    const float k = f(noise(0.05 * X, 0.05 * Y) * 0.8f);
                    for (int32_t Z = G + 2 * az; Z >= G - 2 * az; Z--) {
                        const float wz = f(f((float)(Z - G) / (float)az) + k);
                        if (f(wz * wz + s2) <= 1.f && !(zone.read(X, Y, Z).typeByte & DRY_AIR))
                            zone.place(X, Y, Z, toBlock(rock(X, Y, Z, &zone), ROCK | SURFACE_COLOURED),
                                       [&](int32_t gz) { return rockBlock(X, Y, gz, zone); });
                    }
                }
            }
        }
    }
}
void Terrain::treeGrid(Zone &zone, Random &random) const {
    const int32_t x0 = zone.originX(), y0 = zone.originY();
    const Location *Lz = w.location(x0, y0);
    for (int32_t i = 0; i < 14; i++)
        for (int32_t j = 0; j < 14; j++) {
            const int32_t gx = x0 + 18 * i, gy = y0 + 8 + 18 * j;
            const float bonus = Lz && Lz->type == 3 ? bump(w.locationDistance(*Lz, gx + 8, gy)) : 0.f;
            const float mo = climate(gx + 8, gy, &zone).moisture;
            int32_t s = (int32_t)((float)(random.draw() % 5) + 2.f * mo + 6.f + 4.f * bonus);
            int32_t t = (int32_t)(((float)random.draw() * 8.f / 32767.f + 6.f * bonus + 8.f) * (0.5f * mo + 1.f));
            int32_t x = gx + s, y = gy - 8 + s; // (the sample y less 8: 256 zy + 18 j + s)
            const float hx = (float)(x - HOME), hy = (float)(y - HOME);
            if (hx * hx + hy * hy < 400.f)
                continue; // keep the home point clear
            x = std::min(x, x0 + 256 - s / 2);
            y = std::min(y, y0 + 256 - s / 2);
            bool nearObject = false; // §5 step 8: keep clear of camps and stone circles
            for (const ObjectMark &o : zone.objectList) {
                const float ey = (float)((int64_t)y * 65536 - o.y) * TWO_POW_MINUS_16;
                const float ex = (float)((int64_t)x * 65536 - o.x) * TWO_POW_MINUS_16;
                nearObject = nearObject || ey * ey + ex * ex < 1600.f;
            }
            if (nearObject)
                continue;

            const float n = noiseAt(0x294, 0x298, 0.001, x, y);
            int32_t kind = (random.draw() & 1) ? 5 : 0;
            if (n <= 0.3f) {
                if (random.draw() % 10 == 0)
                    kind = 1;
            } else if (random.draw() % 10 != 0)
                kind = 1;
            if (random.draw() % 10 == 0)
                kind = 2;
            const float wm = climate(x, y, &zone).warmth;
            if (wm > 0.8f && mo > 0.7f) {
                kind = 4 + ((random.draw() & 1) != 0);
                if ((random.draw() & 3) == 0)
                    kind = 3;
            }
            if (wm >= 0.3f && wm < 0.7f) {
                if (noise(0.001 * x + 8473.0, 0.001 * y + 9438.0) > 0.8f && random.draw() % 5 != 0)
                    kind = 2;
            } else if (wm < 0.3f) {
                if (wm > 0.2f) {
                    if (noise(0.001 * x + 8473.0, 0.001 * y + 9438.0) > 0.6f && random.draw() % 5 != 0)
                        kind = 2;
                } else
                    kind = 1;
            }
            if (kind == 2)
                t += random.draw() % std::max(t / 2, 1);
            else if (kind == 1)
                s = std::max(std::min(s, (int32_t)(0.5f * (float)t)), 1);
            if (x + s > x0 + 256)
                x = x0 + 256 - s;
            if (y + s > y0 + 256)
                y = y0 + 256 - s;

            const float p = std::min(sat(1.2f * noiseAt(0x1f4, 0x1f8, 0.008, x, y)) + 0.2f * mo * mo + 0.02f, 1.f) + bonus;
            if ((float)random.draw() / 32767.f > p)
                continue;
            if (!zone.contains(x, y))
                continue;
            // The ground: down from the top to the first block that is not air or water.
            const Column &c = zone.column(x, y);
            int32_t ground = c.bottom - 1;
            for (int32_t z = c.top() - 1; z >= c.bottom; z--)
                if (!clear(zone.read(x, y, z).type())) {
                    ground = z;
                    break;
                }
            if (ground < c.bottom)
                continue;
            // Trees spec §1.2: hot, dry ground grows palms.
            const Climate here = climate(x, y, &zone);
            if (here.warmth > 0.8f && here.moisture < 0.2f)
                kind = 3;
            const uint8_t type = zone.read(x, y, ground).type();
            if (ground >= 0 && (type == GRASS || type == SNOW || (type == SAND && ground + 1 < 3))) {
                zone.trees.push_back({x, y, ground + 1, s, t, kind});
                TreeBuilder(*this, zone, random).build(x, y, ground + 1, std::max(s, 1), t, kind);
            }
        }
}

} // namespace alphagen
