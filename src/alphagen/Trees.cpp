// Adapted from AlphaVulkan (DieuDeGlace), GPL-3.0-only; see docs/alpha-generation.md.
#include "alphagen/Trees.h"
#include "CubModel.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace alphagen {
namespace {

constexpr double PI = 3.141592653589793, TWO_PI = 6.283185307179586;
constexpr uint8_t WOOD_BYTE = 0x27, BARE_WOOD_BYTE = 0x07, LEAF_BYTE = 0x28;

float f(double v) {
    return (float)v;
} // f32(...) of the spec
glm::vec3 clamp255(glm::vec3 c) {
    return glm::clamp(c, glm::vec3(0), glm::vec3(255));
}
// Trunc of each channel, as block bytes.
Block bytesOf(glm::vec3 c, uint8_t typeByte) {
    return {(uint8_t)(int32_t)c.r, (uint8_t)(int32_t)c.g, (uint8_t)(int32_t)c.b, typeByte};
}
// f32(a·(1 − u)) + f32(b·u) per channel, the builder's blend.
glm::vec3 mixColour(glm::vec3 a, glm::vec3 b, float u) {
    const float v = 1.f - u;
    return {a.r * v + b.r * u, a.g * v + b.g * u, a.b * v + b.b * u};
}

} // namespace

bool TreeBuilder::dry(int32_t x, int32_t y, int32_t z) const {
    // §2.2: cells outside the zone read as the implicit solid block, which is not dry.
    return zone.contains(x, y) && (zone.read(x, y, z).typeByte & DRY_AIR) != 0;
}

void TreeBuilder::write(int32_t x, int32_t y, int32_t z, Colour c, uint8_t typeByte) {
    if (!zone.contains(x, y))
        return; // §2.3: trees are clipped to their zone
    zone.place(x, y, z, bytesOf(c, typeByte), [&](int32_t gz) { return bytesOf(terrain.rock(x, y, gz, &zone), ROCK); });
}

// §3.1
float TreeBuilder::moss(int32_t i, int32_t j, int32_t k) const {
    const float n1 = noise((double)f(f(0.2f * (float)j) + 534.f), (double)f(f(0.2f * (float)k) + 13.f));
    const float n2 = noise(0.05 * i + 4343.0, 0.1 * k + 84734.0);
    const float share = (n1 * 0.1f + n2) * 0.7f + 0.2f;
    return std::clamp(share * terrain.climate(i, j, &zone).moisture, 0.f, 1.f);
}

TreeBuilder::Colour TreeBuilder::mossy(Colour c, int32_t i, int32_t j, int32_t k) const {
    const float m = moss(i, j, k);
    const glm::vec3 target(50, 120, 60);
    return {(target.r - c.r) * m + c.r, (target.g - c.g) * m + c.g, (target.b - c.b) * m + c.b};
}

// §3.2 (every tree use is centred).
void TreeBuilder::ball(int32_t cx, int32_t cy, int32_t cz, float r, Colour c, uint8_t typeByte, bool withMoss) {
    const double Cx = cx + 0.5, Cy = cy + 0.5, Cz = cz + 0.5;
    const double r2 = f(r * r);
    for (int32_t i = (int32_t)f(f((float)cx - r) - 1.f); (float)i <= f(f((float)cx + r) + 1.f); i++)
        for (int32_t j = (int32_t)f(f((float)cy - r) - 1.f); (float)j <= f(f((float)cy + r) + 1.f); j++)
            for (int32_t k = (int32_t)f(f((float)cz - r) - 1.f); (float)k <= f(f((float)cz + r) + 1.f); k++) {
                if (((j - Cy) * (j - Cy) + (i - Cx) * (i - Cx)) + (k - Cz) * (k - Cz) > r2 || dry(i, j, k))
                    continue;
                // (moss blends from the colour's bytes)
                const Colour base = glm::vec3(bytesOf(c, 0).r, bytesOf(c, 0).g, bytesOf(c, 0).b);
                write(i, j, k, withMoss ? mossy(base, i, j, k) : base, typeByte);
            }
}

// §3.3
void TreeBuilder::disc(int32_t cx, int32_t cy, int32_t z, float r, Colour c, uint8_t typeByte) {
    const double Cx = cx + 0.5, Cy = cy + 0.5;
    const double r2 = f(r * r);
    for (int32_t i = (int32_t)f(f((float)cx - r) - 1.f); (float)i <= f(f((float)cx + r) + 1.f); i++)
        for (int32_t j = (int32_t)f(f((float)cy - r) - 1.f); (float)j <= f(f((float)cy + r) + 1.f); j++)
            if ((j - Cy) * (j - Cy) + (i - Cx) * (i - Cx) <= r2 && !dry(i, j, z))
                write(i, j, z, c, typeByte);
}

// §3.4
void TreeBuilder::blob(int32_t x, int32_t y, int32_t z, float rH, float rV, Colour lower, Colour upper, int32_t kind) {
    const double cx = x + 0.5, cy = y + 0.5, cz = z + 0.5;
    const int d1 = random.draw(), d2 = random.draw();
    auto colourAt = [&](double v) {
        const float tau = std::clamp(f((v + 1.0) * 0.5), 0.f, 1.f);
        return clamp255(
            {f(1.f - tau) * lower.r + upper.r * tau, f(1.f - tau) * lower.g + upper.g * tau, f(1.f - tau) * lower.b + upper.b * tau});
    };
    for (int32_t i = (int32_t)((cx - rH) - 1.0); i <= (rH + cx) + 1.0; i++)
        for (int32_t j = (int32_t)((cy - rH) - 1.0); j <= (rH + cy) + 1.0; j++) {
            const float off = f(0.5 * (double)noise(0.04 * i + d1, 0.04 * j + d2) + (double)f(0.3f * noise(0.1 * i + d1, 0.1 * j + d2)));
            const double H = ((j - cy) / rH) * ((j - cy) / rH) + ((i - cx) / rH) * ((i - cx) / rH);
            for (int32_t k = (int32_t)((rV + cz) + 1.0); k >= (cz - rV) - 1.0; k--) {
                const double v = ((k - cz) / rV) * 1.7999999523162842 + off;
                if (v * v + H > 1.0 || dry(i, j, k))
                    continue;
                write(i, j, k, colourAt(v), LEAF_BYTE);
                if (kind == 4 && random.draw() % 20 == 0) { // vines hang from jungle crowns
                    int32_t n = random.draw() % 10;
                    if ((random.draw() & 3) == 0)
                        n *= 2;
                    for (int32_t below = k - 1; below >= k - n; below--)
                        if (!dry(i, j, below))
                            write(i, j, below, colourAt(((below - cz) / rV) * 1.8 + off), LEAF_BYTE);
                }
            }
        }
}

// Mode0 subset of alpha-worldgen-structures.md §6.2. No markers, crop or fill-down.
void TreeBuilder::palmLeaf(const CubModel &model, int32_t x, int32_t y, int32_t z, int rotation) {
    const int width = rotation & 1 ? model.sy : model.sx, height = rotation & 1 ? model.sx : model.sy;
    if (x + width <= zone.originX() || y + height <= zone.originY() || x >= zone.originX() + Zone::SIZE || y >= zone.originY() + Zone::SIZE)
        return;
    for (int i = 0; i < model.sx; ++i)
        for (int j = 0; j < model.sy; ++j) {
            int X = x + i, Y = y + j;
            switch (rotation) {
            case 1:
                X = x + j;
                Y = y + model.sx - 1 - i;
                break;
            case 2:
                X = x + model.sx - 1 - i;
                Y = y + model.sy - 1 - j;
                break;
            case 3:
                X = x + model.sy - 1 - j;
                Y = y + i;
                break;
            }
            if (!zone.contains(X, Y))
                continue;
            int low = model.sz, high = 0;
            for (int k = 0; k < model.sz; ++k)
                if (model.solid(i, j, k)) {
                    low = std::min(low, k);
                    high = k;
                }
            for (int k = model.sz - 1; k >= 0; --k) {
                const auto existing = zone.read(X, Y, z + k);
                Block block;
                if (!model.solid(i, j, k)) {
                    if (k <= low || k >= high)
                        continue;
                    block = {255, 255, 255, uint8_t(DRY_AIR | KEEP)};
                } else {
                    if ((existing.typeByte & DRY_AIR) && !clear(existing.type()))
                        continue;
                    const auto c = model.color(i, j, k);
                    block = {c.r, c.g, c.b, uint8_t(LEAF_BYTE | DRY_AIR)};
                }
                zone.place(X, Y, z + k, block, [&](int32_t gap) { return bytesOf(terrain.rock(X, Y, gap, &zone), ROCK); });
            }
        }
}

void TreeBuilder::build(int32_t x, int32_t y, int32_t z, int32_t s, int32_t t, int32_t k) {
    if (k == 3 && !terrain.palmLeaves())
        throw std::invalid_argument("Alpha palms require owner-model stamping");
    const double X0 = x + 0.5, Y0 = y + 0.5;
    const int32_t zx = zone.zx, zy = zone.zy;
    float w = terrain.climate(x, y, &zone).warmth;
    auto draw = [&] { return random.draw(); };

    // §4.1, §4.2: trunk radii and bark.
    float rA = f(s * 0.25f), rB = rA;
    const float gB = f(f(w * 50.f) + 120.f), bB = f(f(w * 100.f) + 50.f);
    const int d1 = draw(), d2 = draw(), d3 = draw(), d4 = draw();
    Colour A(240, f((float)(d1 % 80) + gB), f((float)(d2 % 50) + bB));
    Colour B(240, f((float)(d3 % 80) + gB), f((float)(d4 % 50) + bB));

    // §4.3: per-kind changes.
    switch (k) {
    case 5:
        t = (int32_t)f(t * 0.7f);
        s = std::min((int32_t)f(s * 0.2f), 3);
        if (8 * s > t)
            t = 8 * s;
        break;
    case 1:
        rA = f(rA * 0.5f);
        t *= 2;
        rB = 0.1f;
        break;
    case 6:
        rB = f(rA * 0.5f);
        break;
    case 3:
        rA = rB = 2;
        A = {240, 180, 120};
        B = {220, 100, 50};
        break;
    case 2:
        rA = rB = 1.5f;
        A = B = {255, 255, 255};
        break;
    }
    if (k != 2 && k != 3) {
        rA = std::max(rA, 0.3f);
        rB = std::max(rB, 0.3f);
    }

    // §4.4: the leaf palette.
    const float v = f(f(w - 0.5f) * 10.f);
    bool falling = false; // F: autumn and blossom trees shed leaves (kind-60 records)
    w = std::clamp(w, 0.f, 1.f);
    Colour L1, L2;
    if (f((float)draw() / 32767.f) < v || k == 4 || k == 3) {
        const int a = draw(), b = draw(), c = draw(), d = draw() % 100 + 50;
        L1 = {(float)(a % 20), (float)(b % 155 + 50), (float)(c % 100)};
        L2 = clamp255(L1 + glm::vec3(d, d, 0));
    } else if (terrain.climate(x, y, &zone).moisture > 0.5f) { // autumn
        const int a = draw(), b = draw(), c = draw(), d = draw() % 100 + 50;
        L1 = {(float)(a % 55 + 200), (float)(b % 100), (float)(c % 50)};
        L2 = clamp255(L1 + glm::vec3(d, d, 0));
        falling = true;
    } else if ((draw() & 1) == 0) { // blossom
        const int a = draw(), b = draw(), d = draw() % 100 + 100;
        L1 = {(float)(a % 55 + 200), 0, (float)(b % 200)};
        L2 = clamp255(L1 + glm::vec3(d, d, d));
        falling = true;
    } else {
        const int a = draw(), b = draw(), c = draw(), d = draw() % 150 + 100;
        L1 = {(float)(a % 20), (float)(b % 100 + 120), (float)(c % 100)};
        L2 = clamp255(L1 + glm::vec3(d, d, 0));
    }
    if (k == 2) {
        L1 = {150, 255, 0};
        L2 = {244, 255, 0};
        falling = false;
    }

    // §4.5: cold climates frost the leaves and grey the bark.
    if (w < 0.2f) {
        const float c = std::min(1.f - f(f(w - 0.1f) / 0.1f), 1.f), q = 1.f - c;
        const Colour S = terrain.snow(x, y);
        L1 = c * S + q * L1;
        L2 = c * S + q * L2;
        A = {c * 120.f + q * A.r, c * 120.f + q * A.g, c * 80.f + q * A.b};
    }

    // §4.6: palms lean.
    float jx = 0, jy = 0;
    if (k == 3) {
        jx = f(f(0.5f - f((float)draw() / 32767.f)) * (float)t);
        jy = f(f(0.5f - f((float)draw() / 32767.f)) * (float)t);
        if (f(f((float)x + jx) - (float)s) < (float)(256 * zx) || f((float)s + f((float)x + jx)) >= (float)(256 * zx + 256))
            jx = 0;
        if (f(f((float)y + jy) - (float)s) < (float)(256 * zy) || f((float)s + f((float)y + jy)) >= (float)(256 * zy + 256))
            jy = 0;
    }

    // §4.7: the trunk, top down.
    for (int32_t i = (int32_t)f(f((float)x - rA) - 1.f); (float)i <= f(f((float)x + rA) + 1.f); i++)
        for (int32_t j = (int32_t)f(f((float)y - rA) - 1.f); (float)j <= f(f((float)y + rA) + 1.f); j++)
            for (int32_t c = z + t; c >= z - 2; c--) {
                const float u = f((float)(c - z + 2) / f((float)(t + 2)));
                const float u2 = f(u * u);
                const float r = f(f((1.f - u2) * rA) + f(u2 * rB));
                int32_t ti = i, tj = j;
                if (k == 3) {
                    ti = (int32_t)f(f(jx * u2) + (float)i);
                    tj = (int32_t)f(f(jy * u2) + (float)j);
                }
                double ex, ey;
                if (r > 0.8f && k == 6) {
                    ex = (double)f(noise(0.025 * j, 0.025 * c) * f(5.f / r)) + (i - X0) / r;
                    ey = (double)f(noise(0.025 * i, 0.025 * c + 8473.0) * f(5.f / r)) + (j - Y0) / r;
                } else if (r > 0.8f) {
                    ex = (i - X0) / r;
                    ey = (j - Y0) / r;
                } else {
                    ex = (double)(i - x) / r;
                    ey = (double)(j - y) / r;
                }
                if (ey * ey + ex * ex > 1.0 || dry(ti, tj, c))
                    continue;
                Colour col = mixColour(A, B, u);
                if (k == 2 && draw() % 8 == 0)
                    col = {50, 50, 50}; // birch spots
                write(ti, tj, c, mossy(col, ti, tj, c), k == 6 ? BARE_WOOD_BYTE : WOOD_BYTE);
            }

    // §4.8: roots, flare or palm leaves.
    if (k == 6) {
        const int nR = (draw() & 3) + 5;
        const float phi = f(draw() * TWO_PI / 32767.0);
        const float g = f(rA * 0.75f);
        for (int rho = 0; rho < nR; rho++) {
            const float a = f(rho * PI * 2.0 / nR + phi);
            const float sa = f(std::sin((double)a)), ca = f(std::cos((double)a));
            double PX = X0 + f(f(ca * rA) * 0.5f), PY = Y0 + f(f(sa * rA) * 0.5f), PZ = z;
            const float dX = f(f(ca * g) * 0.1f), dY = f(f(sa * g) * 0.1f), e = f(g * 0.05f);
            for (int q = 0; q <= 140; q += 7) {
                ball((int32_t)PX, (int32_t)PY, (int32_t)PZ, (float)(10 - q / 20), A, WOOD_BYTE, true);
                PX += dX;
                PY += dY;
                PZ -= f(f((float)draw() * e) / 32767.f);
            }
        }
    } else if (k == 0 || k == 2 || k == 4 || k == 5) {
        float o = -1;
        for (int32_t zc = z; zc >= z - 1; zc--) {
            const float r = f(f((float)(4 - z + zc) * rA) / 5.f);
            o = f(o + r);
            ball((int32_t)f((float)x - o), y, zc, r, A, WOOD_BYTE, true);
            ball((int32_t)f((float)(x + 1) + o), y, zc, r, A, WOOD_BYTE, true);
            ball(x, (int32_t)f((float)y - o), zc, r, A, WOOD_BYTE, true);
            ball(x, (int32_t)f((float)(y + 1) + o), zc, r, A, WOOD_BYTE, true);
        }
    } else if (k == 3) {
        // Trees §4.8.3; structure placer §6.2, mode0 (no marker draws).
        const auto *models = terrain.palmLeaves();
        const int64_t fx = int64_t(x) * 65536 + 32768 + int64_t(f(jx * 65536.f));
        const int64_t fy = int64_t(y) * 65536 + 32768 + int64_t(f(jy * 65536.f));
        const int32_t xc = int32_t(fx / 65536), yc = int32_t(fy / 65536);
        if (models->cardinal) {
            const auto &m = *models->cardinal;
            const int32_t origins[4][2] = {
                {xc - m.sx / 2, yc + 1}, {xc + 1, yc - m.sx / 2}, {xc - m.sx / 2, yc - m.sy}, {xc - m.sy, yc - m.sx / 2}};
            for (int r = 0; r < 4; ++r) {
                const int32_t zp = z + t - 5 + draw() % 3;
                palmLeaf(m, origins[r][0], origins[r][1], zp, r);
            }
        }
        if (models->diagonal) {
            const auto &m = *models->diagonal;
            const int32_t origins[4][2] = {{xc + 1, yc + 1}, {xc + 1, yc - m.sy}, {xc - m.sy, yc - m.sy}, {xc - m.sy, yc + 1}};
            for (int r = 0; r < 4; ++r) {
                const int32_t zp = z + t - 5 + draw() % 3;
                palmLeaf(m, origins[r][0], origins[r][1], zp, r);
            }
        }
    }

    // §4.9: the marker draw: one crown in ten of a warm enough tree gets a kind-61 record,
    // where birds sing by day and owls call by night (audio §3).
    if (draw() % 10 == 0 && w > 0.2f)
        zone.songPosts.push_back({(int64_t)x * 65536 + 32768, (int64_t)y * 65536 + 32768, (int64_t)(z + t) * 65536});

    // §5: a kind-60 record, in the leaves' colour as the crown is made.
    const auto leaves = [&](int64_t px, int64_t py, int64_t pz) {
        zone.leafSources.push_back({px, py, pz, L1.r / 255.f, L1.g / 255.f, L1.b / 255.f});
    };

    // §4.10: crowns.
    if (k == 0 || k == 2 || k == 4) {
        const int nb = draw() % 5 + 4;
        const float phi = f(draw() * TWO_PI / 32767.0);
        const float sf = f(f((float)draw() * 0.8f) / 32767.f) + 0.6f;
        const float rC = f(((double)f(f((float)draw() * 1.25f) / 32767.f) + 2.0) * rA);
        blob(x, y, z + t, rC, f(rC * sf), L1, L2, k);
        for (int b = 0; b < nb; b++) {
            const float h = f(f(f((float)draw() * 0.5f) / 32767.f) + 0.5f);
            const float zb = f(f(h * (float)t) + (float)z);
            const float a = f(b * PI * 2.0 / nb + phi);
            const float sa = f(std::sin((double)a)), ca = f(std::cos((double)a));
            double PX = X0 + f(f(ca * rA) * 0.5f), PY = Y0 + f(f(sa * rA) * 0.5f), PZ = zb;
            const Colour cb = mixColour(A, B, h);
            const float g = f(rA * 0.75f);
            for (int step = 0; step < 6; step++) {
                if (k == 0 && rA > 1.5f)
                    ball((int32_t)PX, (int32_t)PY, (int32_t)PZ, 2, cb, WOOD_BYTE, true);
                PX += f(f(ca * g) * 0.5f);
                PY += f(f(sa * g) * 0.5f);
                PZ += f(f((float)draw() * f(g * 0.5f)) / 32767.f);
            }
            const float rr = f(((double)f(f((float)draw() * 1.25f) / 32767.f) + 2.0) * rA);
            if ((PX - rr) - 1.0 < 256.0 * zx)
                PX = f(f((float)(256 * zx) + rr) + 1.f);
            if ((PY - rr) - 1.0 < 256.0 * zy)
                PY = f(f((float)(256 * zy) + rr) + 1.f);
            if ((rr + PX) + 1.0 >= 256.0 * zx + 256.0)
                PX = f(f((float)(256 * zx + 256) - rr) - 2.f);
            if ((rr + PY) + 1.0 >= 256.0 * zy + 256.0)
                PY = f(f((float)(256 * zy + 256) - rr) - 2.f);
            if (b > 4)
                PZ += f(f(sf * rr) * 0.5f);
            const float e = f(f(sf * rr) * 0.25f);
            blob((int32_t)PX, (int32_t)PY, (int32_t)(e + PZ), rr, f(sf * rr), L1, L2, k);
            if (falling)
                leaves((int64_t)(PX * 65536.0), (int64_t)(PY * 65536.0), (int64_t)((PZ + e) * 65536.0));
        }
    } else if (k == 5) {
        const int32_t layers = (int32_t)f((float)(t / 4 + 6) + f(s * 0.5f));
        draw();
        draw();
        const float phi0 = f((draw() & 7) * PI / 4.0);
        for (int32_t i = 0; i < layers; i++) {
            const int32_t acc = i * t;
            if ((i & 1) && draw() % 3 != 0)
                continue;
            const float hf = f(f(i * 0.6f) / (float)std::max(layers - 1, 1)) + 0.6f;
            const float zl = f(f((float)t * hf) + (float)z);
            const float a = f(i * PI * 0.25 + phi0);
            const float sa = f(std::sin((double)a)), ca = f(std::cos((double)a));
            const double Qx = X0 + f(ca * rA), Qy = Y0 + f(sa * rA);
            const Colour col = clamp255(mixColour(A, B, hf));
            const int32_t half = std::max(t / 2, 1);
            const int32_t n = t / 2 + draw() % half + (acc / 4) / layers;
            const float fr = f(f(f((float)s * 8.f) * f(1.f - hf)) + (float)s);
            const float rx = f(fr * ca), ry = f(fr * sa);
            for (int32_t j = 0; j <= 2 * n; j++) {
                const float frac = n > 0 ? (float)j / (float)(2 * n) : 1.f;
                const float e = 1.f - (1.f - frac) * (1.f - frac);
                const int64_t PZ = (int64_t)(((double)f(j * 0.5f) + zl) * 65536.0);
                const int64_t PY = (int64_t)(((double)f(e * ry) + Qy) * 65536.0);
                const int64_t PX = (int64_t)(((double)f(e * rx) + Qx) * 65536.0);
                const int32_t bx = (int32_t)(PX / 65536), by = (int32_t)(PY / 65536), bz = (int32_t)(PZ / 65536);
                if (dry(bx, by, bz) || dry(bx, by, bz + 1))
                    continue;
                write(bx, by, bz + 1, col, WOOD_BYTE);
                write(bx, by, bz, col, WOOD_BYTE);
                const bool clump = j == 2 * n || (j % 4 == 0 && draw() % 3 == 0);
                if (!clump)
                    continue;
                const float rH = f(f(f(f((float)draw() * f(e * 0.75f)) / 32767.f) + 0.25f) * f(n * 0.5f)) + 2.f;
                const float rV = f(f(f((float)draw() * 0.5f) / 32767.f) + 0.5f) * rH;
                int64_t CY = PY + (int64_t)f(f(f(rH * sa) * 0.25f) * 65536.f);
                int64_t CX = PX + (int64_t)f(f(f(rH * ca) * 0.25f) * 65536.f);
                const int64_t RH = (int64_t)f(rH * 65536.f);
                const int64_t lowX = (int64_t)zx * 256 * 65536, lowY = (int64_t)zy * 256 * 65536;
                if (CX - RH - 65536 < lowX)
                    CX = (int64_t)f(f(f((float)(256 * zx) + rH) + 1.f) * 65536.f);
                if (CY - RH - 65536 < lowY)
                    CY = (int64_t)f(f(f((float)(256 * zy) + rH) + 1.f) * 65536.f);
                if (CX + RH + 65536 >= lowX + 256 * 65536)
                    CX = (int64_t)f(f(f((float)(256 * zx + 256) - rH) - 2.f) * 65536.f);
                if (CY + RH + 65536 >= lowY + 256 * 65536)
                    CY = (int64_t)f(f(f((float)(256 * zy + 256) - rH) - 2.f) * 65536.f);
                blob((int32_t)(CX / 65536), (int32_t)(CY / 65536), bz + 1, rH, rV, L1, L2, 5);
                if (falling)
                    leaves(CX, CY, PZ + 65536);
            }
        }
    } else if (k == 6) {
        const int nB = draw() % 5 + 8;
        const float phi = f(draw() * TWO_PI / 32767.0);
        draw();
        draw();
        const float g = f(rA * 0.75f);
        for (int b = 0; b < nB; b++) {
            const float h = f(f(f((float)draw() * 0.5f) / 32767.f) + 0.5f);
            const float zb = f(f((float)t * h) + (float)z);
            const float a = f(b * PI * 2.0 / nB + phi);
            const float sa = f(std::sin((double)a)), ca = f(std::cos((double)a));
            const float a2 = f(a + 1.5707963267948966);
            const float s2 = f(std::sin((double)a2)), c2 = f(std::cos((double)a2));
            double PX = X0 + f(f(ca * rA) * 0.5f), PY = Y0 + f(f(sa * rA) * 0.5f), PZ = zb;
            const Colour cb = mixColour(A, B, h);
            for (int q = 0; q <= 200; q += 5) {
                const int n = q / 5;
                const float rho = f(6.5f - f((float)q / 40.f));
                ball((int32_t)PX, (int32_t)PY, (int32_t)PZ, rho, cb, WOOD_BYTE, true);
                if (n > 10) {
                    const int32_t lift = (int32_t)f(rho * -2.f);
                    const int32_t M1 = 2 - lift, M2 = (1 - lift) * 2;
                    for (int round = 0; round < 5; round++)
                        for (int side = 0; side < 2; side++) {
                            if (draw() % 120 >= n - 10)
                                continue;
                            const int v1 = (draw() & 3) + 4, h1 = (draw() & 3) + 4;
                            const int32_t M = side == 0 ? M1 : M2;
                            const float sign = side == 0 ? 1.f : -1.f;
                            const int32_t cz = (int32_t)((double)(draw() % std::max(M, 1)) + (PZ - 3.0));
                            const int32_t cy = (int32_t)(((double)(draw() % 20) + PY - 10.0) + f(sign * f(s2 * 4.f)));
                            const int32_t cx = (int32_t)(((double)(draw() % 20) + PX - 10.0) + f(sign * f(c2 * 4.f)));
                            blob(cx, cy, cz, (float)h1, (float)v1, L1, L2, 6);
                        }
                    if (falling)
                        leaves((int64_t)(PX * 65536.0), (int64_t)(PY * 65536.0), (int64_t)(PZ * 65536.0));
                }
                PX += f(f(ca * g) * 0.075f);
                PY += f(f(sa * g) * 0.075f);
                PZ += f(f(f(g * 0.1f) * (float)n) / 40.f);
            }
        }
    } else if (k == 1) {
        float phi = f(draw() * TWO_PI / 32767.0);
        const int p = draw() % 100, q = draw() % 50 + 100, r = draw() % 100;
        const Colour C1((float)r, (float)q, (float)p);
        const int p2 = draw() % 100, q2 = draw() % 50 + 100, r2 = draw() % 100;
        const Colour C2((float)r2, (float)q2, (float)p2);
        const float R = (float)s, droop = f(-(float)s * 0.3f);
        const int32_t segs = t / 3;
        for (int spoke = 0; spoke < 30; spoke++) {
            phi = f(phi + 0.9424778335276408);
            const float vv = f(f(f(spoke * 0.7f) / 30.f) + 0.3f);
            const float cr = f(f(std::cos((double)phi)) * R), sr = f(f(std::sin((double)phi)) * R);
            const float fV = f(f(1.f - vv) * 2.f);
            const float hz = f(f(vv * (float)t) + (float)z);
            const double Ex = x + (double)f(fV * cr), Ey = y + (double)f(fV * sr), Ez = hz + (double)f(fV * droop);
            for (int32_t m = segs; m >= 1; m--) {
                const float fm = f((float)m / (float)segs), gm = f(1.f - fm);
                // (in double: x and y are past 2^23, where a float has no fractions left)
                const double PX = fm * Ex + gm * (double)x, PY = fm * Ey + gm * (double)y, PZ = fm * Ez + gm * (double)hz;
                if (w < 0.3f)
                    disc((int32_t)PX, (int32_t)PY, (int32_t)(PZ + 1.0), 2, terrain.snow(x, y), LEAF_BYTE);
                disc((int32_t)PX, (int32_t)PY, (int32_t)PZ, 2, mixColour(C1, C2, fm), LEAF_BYTE);
            }
        }
        write(x, y, z + t + 1, C1, LEAF_BYTE);
        write(x, y, z + t, C1, LEAF_BYTE);
    }
}

} // namespace alphagen
