// Adapted from AlphaVulkan's original GPL clean-room plant pass.
// Arithmetic/draw order follow alpha-worldgen-vegetation.md; record consumption
// is a separate bridge (alpha-plant-record-consumption.md).
#include "alphagen/Terrain.h"
#include <algorithm>
#include <cmath>
namespace alphagen {
VegetationStage Terrain::vegetation(const Zone &zone, Random random) const {
    VegetationStage result;
    const int32_t x0 = zone.originX(), y0 = zone.originY();
    auto draw = [&] { return random.draw(); };
    auto spin = [&] { return (float)draw() * 360.f / 32767.f; };
    auto floatArgNoise = [](float a, float b) { return noise((double)a, (double)b); };

    for (int32_t x = x0; x < x0 + Zone::SIZE; x++)
        for (int32_t y = y0; y < y0 + Zone::SIZE; y++) {
            const Column &column = zone.column(x, y);
            const int32_t count = (int32_t)column.blocks.size();
            const Climate cl = climate(x, y, &zone);
            for (int32_t i = 0; i < count; i++) {
                const int32_t zt = column.bottom + i + 1;
                const uint8_t above = zone.read(x, y, zt).type();
                if (i < count - 1 && !clear(above))
                    continue;
                const uint8_t here = zone.read(x, y, zt - 1).type();

                if (here == TOWN_GROUND && above == AIR) { // §3.1: street lights
                    if (townness(x, y) > 0.75f && (90 * y + x) % 470 == 0) {
                        const int r = draw();
                        bool room = true;
                        for (int32_t z = zt; z < zt + 7; z++)
                            room = room && clear(zone.read(x, y, z).type());
                        if ((r & 15) == 0 && room) {
                            StaticObject light;
                            const bool hot = cl.warmth > 0.8f;
                            light.kind = hot ? 51 : 50;
                            light.x = (int64_t)x * 65536;
                            light.y = (int64_t)y * 65536;
                            light.z = (int64_t)zt * 65536;
                            light.rotation = r & 3;
                            light.scale[0] = light.scale[1] = hot ? 1.f : 2.f;
                            light.scale[2] = 8;
                            result.statics.push_back(light);
                        }
                    }
                    continue;
                }
                if (here == LAKE_BED && above == AIR) { // §3.2: reeds
                    if (cl.warmth > 0.2f && std::fabs(floatArgNoise(0.05f * (float)x + 9843.f, 0.05f * (float)y + 8437.f)) > 0.5f) {
                        const int r = draw();
                        if ((r & 7) == 0)
                            result.plants.push_back({x, y, zt, 0.09f, (float)r * 360.f / 32767.f, 22, 4});
                    }
                    continue;
                }
                if (here == WATER && above == AIR) { // §3.3: water lilies
                    if (cl.warmth > 0.2f && zt > 0 &&
                        std::fabs(floatArgNoise(0.05f * (float)x + 24234.f, 0.05f * (float)y + 53565.f)) > 0.7f) {
                        const int r = draw();
                        if (r % 10 == 0)
                            result.plants.push_back({x, y, zt, 0.09f, (float)r * 360.f / 32767.f, 31 + (r & 1), 0});
                    }
                    continue;
                }
                const bool ground = here == GRASS || here == SAND || here == SNOW || here == FIRE_GROUND;
                if (!ground || !clear(above))
                    continue;
                // §3.4: ground cover.
                if (std::fabs(floatArgNoise(0.05f * (float)x + 9843.f, 0.05f * (float)y + 8437.f)) <= 0.6f)
                    continue;
                if ((draw() & 7) != 0)
                    continue;
                const float nA = noise(0.01 * x + 9843.0, 0.01 * y + 8437.0);
                if (zt <= -5) {
                    Plant p{x, y, zt, 0.1f, (float)((draw() & 3) * 90)};
                    if (nA <= 0) {
                        p.kind = 7;
                        p.flags = 4;
                    } else {
                        p.kind = 5 + (draw() & 1);
                        if (p.kind == 5) {
                            p.scale = 0.075f;
                            p.flags = 4;
                        }
                    }
                    p.scale *= (float)draw() / 32767.f + 1.f;
                    result.plants.push_back(p);
                    continue;
                }
                if (zt < 1)
                    continue;
                if (here == FIRE_GROUND) {
                    Plant p{x, y, zt, 0.075f, (float)((draw() & 3) * 90)};
                    if (nA > 0) {
                        p.kind = 9;
                        p.flags = 4;
                    } else {
                        p.kind = 10;
                        p.scale = (float)(draw() * 0.02 / 32767.0 + 0.03);
                    }
                    result.plants.push_back(p);
                    continue;
                }
                if (here == GRASS || here == SAND) {
                    const float fineFloat = floatArgNoise(0.01f * (float)x + 34234.f, 0.01f * (float)y + 234234.f);
                    const float fineDouble = noise(0.01 * x + 34234.0, 0.01 * y + 234234.0);
                    if (cl.warmth > 0.75f) {
                        if (cl.moisture <= 0.25f) {
                            const int r = draw();
                            if (r % 100 == 0) {
                                Plant p{x, y, zt, 0.075f, (float)((draw() & 3) * 90)};
                                p.kind = 28 - ((draw() & 1) != 0 ? 1 : 0);
                                result.plants.push_back(p);
                            }
                        } else {
                            Plant p{x, y, zt, 0.075f, spin()};
                            if (nA > 0)
                                p.kind = fineFloat > 0 ? 4 : 3;
                            else
                                p.kind = fineDouble <= 0.5f ? 12 : 11;
                            if (p.kind == 2 || p.kind == 3)
                                p.flags = 4;
                            else if (p.kind == 11) {
                                p.flags = 4;
                                p.scale = (float)draw() * 0.05f / 32767.f + 0.05f;
                            } else if (p.kind == 12) {
                                p.flags = 4;
                                p.scale = (float)draw() * 0.02f / 32767.f + 0.1f;
                            }
                            if (here == GRASS || p.kind == 2 || p.kind == 3)
                                result.plants.push_back(p);
                        }
                    } else {
                        Plant p{x, y, zt, 0.075f, (float)((draw() & 3) * 90)};
                        if (nA > 0)
                            p.kind = cl.moisture > 0.5f ? (fineFloat <= 0 ? 2 : 3) : (fineFloat > 0 ? 12 : 4);
                        else
                            p.kind = fineDouble <= 0 ? 1 : 0;
                        if (p.kind == 2 || p.kind == 3 || p.kind == 4 || p.kind == 12)
                            p.flags = 4;
                        if (p.kind == 12)
                            p.scale = (float)draw() * 0.02f / 32767.f + 0.1f;
                        if (here == GRASS || p.kind == 2 || p.kind == 3 || p.kind == 4)
                            result.plants.push_back(p);
                    }
                }
                // §4: ground items.
                if (here == GRASS) {
                    const int r = draw();
                    if (r % 150 == 0) {
                        GroundItem item{x, y, zt, 11, 15, (float)draw() / 32767.f * 360.f, 1};
                        if (draw() & 1) {
                            item.subtype = 22;
                            item.scale = 0.1f;
                        }
                        result.items.push_back(item);
                    }
                } else if (here == SNOW) {
                    if (draw() % 300 == 0) {
                        GroundItem item{x, y, zt, 11, 24, (float)draw(), 0.1f};
                        draw(); // (its value always selects subtype 24)
                        result.items.push_back(item);
                    }
                }
            }
        }
    result.random = random;
    return result;
}
} // namespace alphagen
