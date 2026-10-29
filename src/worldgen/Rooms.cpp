// Room contents of the building builder, written from docs/specs/worldgen-structures-buildings.md
// §6: furniture (§6.1), room objects (§6.2) and the pieces by the room's code (§6.3).
#include "worldgen/BuildingBuilder.h"

#include "worldgen/BlockColours.h"
#include "worldgen/Height.h"

#include <vector>

namespace worldgen {
namespace {

// "Taken" (§6.1): solid, or inside an object (flag 0x40) that is not object 15 (flag 0x80).
bool taken(const Block &b) { return !isOpen(b) || ((b.flags & 0x40) && !(b.flags & 0x80)); }

// The furnisher (0x675c0, 0x69860) and its pieces.
class Furnisher {
public:
    Furnisher(ZoneBuild &zone, Mason &m, Colour c0, Colour c1, Colour light) : zone(zone), m(m), c0(c0), c1(c1), light(light) {}
    ZoneBuild &zone;
    Mason &m;
    Colour c0, c1, light;
    std::vector<int> wall, centre, side, ceiling;

    int pick(const std::vector<int> &list) { return list[(size_t)(zone.draw() % (int)list.size())]; }

    // 0x69860.
    void furnish(int32_t x0, int32_t y0, int32_t z, int32_t sx, int32_t sy, int32_t h) {
        const int32_t top = z + h;
        if (zone.draw() % 2 != 0)
            sideModel(x0 + sx / 2, y0, z, 0);
        else
            wallSpot(x0 + sx / 2, y0, z, top, 0, false);
        if (zone.draw() % 2 != 0)
            sideModel(x0 + sx / 2, y0 + sy - 1, z, 2);
        else
            wallSpot(x0 + sx / 2 - 1, y0 + sy - 1, z, top, 2, true);
        if (zone.draw() % 2 != 0)
            sideModel(x0, y0 + sy / 2, z, 1);
        else
            wallSpot(x0, y0 + sy / 2, z, top, 1, false);
        if (zone.draw() % 2 != 0)
            sideModel(x0 + sx - 1, y0 + sy / 2, z, 3);
        else
            wallSpot(x0 + sx - 1, y0 + sy / 2 - 1, z, top, 3, true);
        for (int32_t x = x0; x < x0 + sx; x++)
            for (int32_t y = y0; y < y0 + sy; y++) {
                if (x % 5 == 3 && y % 5 == 3 && zone.draw() % 5 == 0)
                    ceilingModel(x, y, top);
                else if (x % 5 == 1 && y % 5 == 1 && zone.draw() % 5 == 0)
                    centrePiece(x, y, top, z);
            }
    }

    // 0x6a9a0.
    void sideModel(int32_t x, int32_t y, int32_t z, int sideDir) {
        if (side.empty() || zone.draw() % 2 != 0)
            return;
        const Mason c = m.child(x, y, 0, 1, 1, sideDir, false);
        const int kind = pick(side);
        if (kind == 1 || (kind >= 3 && kind <= 5))
            z += 2 + zone.draw() % 4;
        else if (kind == 2)
            z += 2;
        int32_t py = 0;
        for (int n = 0; n < 10 && isOpen(c.get(0, py - 1, z)); n++)
            py -= 1;
        for (int n = 0; n < 10 && taken(c.get(0, py, z)); n++)
            py += 1;
        for (int32_t dx = -1; dx <= 1; dx++)
            for (int32_t zz = z; zz <= z + 4; zz++)
                if (isOpen(c.get(dx, py - 1, zz)))
                    return;
        const Block back = c.get(0, py - 1, z);
        if (!(!isOpen(back) && (back.flags & 2)) || !isOpen(c.get(0, py, z)) || !isOpen(c.get(0, py, z + 1)))
            return;
        const Mason::World w = c.world(0, py, z);
        Model mo;
        mo.r = mo.g = mo.b = 1.0f, mo.flags = 2;
        mo.x = dotsOf(w.x) + 32768, mo.y = dotsOf(w.y) + 32768, mo.z = dotsOf(w.z) + 655;
        float k = 1.0f, s = 1.0f;
        switch (kind) {
        case 0: mo.type = 0x55 + zone.draw() % 4, k = 0.2f, s = zone.fraction(0.6f) + 1.0f; break;
        case 1: mo.type = 0x43, k = 0.0625f, s = 1.5f; break;
        case 2: mo.type = 0x3d, k = 0.04f, s = 3.0f; break;
        case 3: mo.type = 0x3e, k = 0.0625f, s = 1.5f; break;
        case 4: mo.type = 0x3f, k = 0.0625f, s = 1.5f; break;
        default: mo.type = 0x44 + zone.draw() % 4, k = 1.0f, s = zone.fraction(0.1f) + 0.1f; break;
        }
        if (kind == 1)
            mo.r = 0.6f, mo.g = 0.5f, mo.b = 0.2f, mo.flags = 1;
        mo.yaw = (float)(c.facing(0) * 90);
        const float tt = s - 1.0f;
        mo.x += trunc64(tt * (float)c.matrix()[4] * 0.5f * 65536.0f);
        mo.y += trunc64(tt * (float)c.matrix()[5] * 0.5f * 65536.0f);
        mo.size = k * s;
        zone.models.push_back(mo);
    }

    // 0x6bef0.
    void wallSpot(int32_t x, int32_t y, int32_t z, int32_t top, int sideDir, bool mirror) {
        if (wall.empty())
            return;
        Mason c = m.child(x, y, 0, 1, 1, sideDir, mirror);
        for (int32_t dx = -2; dx <= 2; dx++)
            for (int32_t zz = z + 1; zz <= top - 1; zz++) {
                const Block b = c.get(dx, -1, zz);
                if (isOpen(b) || !(b.flags & 2))
                    return;
            }
        for (;;) {
            const bool t = taken(m.get(x, y, z));
            if (z >= top)
                return;
            if (!t)
                break;
            z += 1;
        }
        wallPiece(c, pick(wall), z, top);
    }

    void wallPiece(Mason &c, int kind, int32_t z, int32_t top) {
        if (kind != 7)
            c.slots(light, 1, (float)(z - 20), c0, 1, (float)z, c1, 1, (float)top);
        switch (kind) {
        case 0:
            c.foundation(-1, 0, z, 4, 2, 1, 0, 0);
            c.box(0, 0, z + 1, 2, 1, top - z - 2);
            c.box(-1, 0, top - 1, 4, 2, 1);
            break;
        case 1:
            c.foundation(-2, 0, z, 6, 2, 1, 0, 0);
            c.foundation(-1, 2, z, 4, 1, 1, 0, 0);
            c.box(-1, 0, z + 1, 4, 1, top - z - 2);
            c.box(0, 1, z + 1, 2, 1, top - z - 2);
            c.box(-2, 0, top - 1, 6, 2, 1);
            c.box(-1, 2, top - 1, 4, 1, 1);
            break;
        case 2:
            c.foundation(-1, 0, z - 1, 4, 2, 1, 0, 0);
            c.box(-2, 0, z, 6, 3, 1);
            c.box(-1, 0, z + 1, 4, 2, 1);
            c.box(0, 0, z + 2, 2, 1, top - z - 2);
            c.box(-1, 0, top - 5, 4, 2, 1);
            c.box(0, 1, top - 3, 2, 1, 1);
            c.box(0, 1, top - 2, 2, 2, 1);
            c.box(0, 1, top - 1, 2, 9, 1);
            break;
        case 3:
            c.foundation(-1, 0, z, 4, 2, 1, 0, 0);
            c.box(0, 0, z + 1, 2, 1, top - z - 4);
            c.box(-1, 0, top - 4, 4, 2, 2);
            c.box(-2, 0, top - 2, 6, 3, 1);
            c.box(-4, 0, top - 1, 10, 4, 1);
            break;
        case 4: bookshelf(c, z, top); break;
        case 5:
            c.foundation(0, 0, z - 1, 2, 2, 1, 0, 0);
            c.box(0, 0, z, 2, 2, top - z - 5);
            c.box(-1, 0, top - 5, 4, 2, 2);
            c.box(-2, 0, top - 3, 6, 2, 1);
            c.box(-3, 0, top - 2, 8, 2, 1);
            c.box(-5, 0, top - 1, 12, 2, 1);
            break;
        case 6:
            c.foundation(-4, 0, z - 1, 9, 3, 1, 0, 0);
            c.box(-4, 0, z, 9, 3, 4);
            c.box(-3, 0, z + 4, 7, 3, 1);
            c.box(-2, 0, z + 5, 5, 3, 1);
            c.air(-3, 0, z, 7, 3, 3, 0);
            c.air(-2, 0, z + 3, 5, 3, 1, 0);
            c.twoColours({80, 120, 80}, 0, (float)z, {80, 80, 80}, 0, (float)(z + 4), false);
            for (int32_t x : {-2, 0, 2})
                c.box(x, 1, z, 1, 1, 4);
            c.oneColour({1, 1, 1}, pattern::PLAIN);
            c.box(-3, 0, z, 7, 1, 4);
            break;
        case 7: windowBox(c, z); break;
        default: break; // 8-10 belong to lists no room fills (open question 6)
        }
    }

    // The bookshelf (0x69ce0) and its books.
    void bookshelf(Mason &c, int32_t z, int32_t top) {
        c.box(-4, 0, z - 1, 9, 1, 1);
        const int32_t h = top - z;
        c.box(-4, 0, z, 9, 1, h);
        c.box(-3, 1, z, 7, 1, h);
        int32_t zz = z + 1;
        for (int32_t n = 0; n < h / 5; n++, zz += 5) {
            if (zone.draw() % 2 != 0)
                continue;
            c.air(-2, 0, zz, 5, 2, 3, 0);
            if (zone.draw() % 3 != 0)
                continue;
            const int32_t a = zone.draw() % 2, b = zone.draw() % 2;
            const Mason::World w = c.world(a, b, zz);
            GroundItem book;
            book.angle = (float)(zone.draw() % 4 * 90);
            book.item.category = itemcategory::BOOK;
            book.item.id = 0;
            book.item.modifier = (uint32_t)zone.draw(); // the raw draw
            book.x = trunc64(((double)w.x + 0.5) * 65536.0);
            book.y = trunc64(((double)w.y + 0.5) * 65536.0);
            book.z = dotsOf(w.z);
            zone.items.push_back(book);
        }
    }

    // The window box (0x6bb20).
    void windowBox(Mason &c, int32_t z) {
        const int32_t e = z + 1 + zone.draw() % 4;
        c.twoColours({80, 120, 40}, 0, (float)e, {80, 120, 130}, 0, (float)(e + 4), false);
        c.box(-1, 0, e, 3, 3, 1);
        c.box(-2, 0, e + 1, 5, 3, 3);
        c.box(-1, 0, e + 4, 3, 3, 1);
        c.air(-1, 0, e + 1, 3, 3, 3, 0);
        c.oneColour({1, 1, 1}, pattern::PLAIN);
        c.box(-1, -1, e + 1, 3, 1, 3);
        c.twoColours(light, 0, 0, {60, 150, 20}, 0, (float)e, false);
        if (zone.draw() % 3 == 0)
            return;
        c.box(-1, 0, e + 1, 3, 2, 1);
        if (zone.draw() % 3 == 0)
            return;
        const int32_t b = zone.draw() % 3 - 1;
        c.box(b, 2, e + 1, 1, 1, 1);
        const int32_t r = zone.draw() % 10;
        c.box(b, 3, e + 2 - r, 1, 1, r);
    }

    // 0x68510.
    void ceilingModel(int32_t x, int32_t y, int32_t t) {
        if (ceiling.empty() || zone.draw() % 2 != 0)
            return;
        for (int n = 0; n < 10 && taken(m.get(x, y, t - 1)); n++)
            t -= 1;
        const int kind = pick(ceiling);
        if (kind == 1 && zone.draw() % 20 != 0)
            return;
        const Mason::World w = m.world(x, y, t);
        Model mo;
        mo.r = mo.g = mo.b = 1.0f, mo.flags = 2;
        mo.x = dotsOf(w.x) + 32768, mo.y = dotsOf(w.y) + 32768, mo.z = dotsOf(w.z) + 655;
        mo.yaw = (float)(zone.draw() % 4 * 90);
        if (kind == 0) {
            const float s = zone.fraction(0.06f) + 0.08f;
            mo.z -= trunc64(s * 20.0f * 65536.0f);
            mo.type = 0x48, mo.size = s;
        } else {
            mo.type = 0x49, mo.z -= 0x40000, mo.size = 0.15f;
            mo.r = 0.55f, mo.g = 0.5f, mo.b = 0.4f, mo.flags = 1;
        }
        zone.models.push_back(mo);
    }

    // 0x696b0.
    void centrePiece(int32_t x, int32_t y, int32_t top, int32_t z0) {
        if (centre.empty())
            return;
        const int kind = pick(centre);
        if (kind == 0)
            hangingCone(x, y, top);
        else if (kind == 4)
            chainLamp(x, y, top, z0);
        // 1-3 belong to lists no room fills (open question 6).
    }

    // 0x68b00: a lamp on a chain.
    void chainLamp(int32_t x, int32_t y, int32_t t, int32_t z0) {
        if (zone.draw() % 8 != 0)
            return;
        x += zone.draw() % 5 - 2;
        y += zone.draw() % 5 - 2;
        for (int n = 0; n < 10 && taken(m.get(x, y, t - 1)); n++)
            t -= 1;
        auto inside = [&](int32_t a, int32_t b) {
            const float p = simplex((double)(x + a) * 0.3, 0.0) + (float)a;
            const float q = simplex((double)(y + b) * 0.3, 0.0) + (float)b;
            return !(q * q + p * p > 1.0f) || (float)b * (float)b + (float)a * (float)a <= 1.0f;
        };
        for (int32_t a = -2; a <= 2; a++)
            for (int32_t b = -2; b <= 2; b++)
                if (inside(a, b) && !taken(m.get(x + a, y + b, t + 2)))
                    return;
        for (int32_t a = -2; a <= 2; a++)
            for (int32_t b = -2; b <= 2; b++) {
                if (!inside(a, b))
                    continue;
                m.put(x + a, y + b, t, Block{0x7f, 0x7f, 0x7f, 255, 0, 0x22});
                const int32_t px = x + a, py = y + b, pz = t + 1;
                if (!isOpen(m.get(px, py, pz)) && !isOpen(m.get(px - 1, py, pz)) && !isOpen(m.get(px + 1, py, pz)) &&
                    !isOpen(m.get(px, py - 1, pz)) && !isOpen(m.get(px, py + 1, pz)))
                    m.put(px, py, pz, Block{0x7f, 0x7f, 0x7f, 255, blocktype::LIGHT, 0x22});
            }
        const Mason::World w = m.world(x, y, t + 2);
        Model chain;
        chain.type = 0x59;
        chain.x = dotsOf(w.x) + 32112, chain.y = dotsOf(w.y) + 32112, chain.z = dotsOf(w.z) - 67502;
        chain.size = (float)(w.z - z0 - 1);
        chain.yaw = 0.7f;
        chain.r = chain.g = chain.b = 1.0f, chain.flags = 2;
        zone.models.push_back(chain);
    }

    // 0x690b0: a cone hanging from the ceiling.
    void hangingCone(int32_t x, int32_t y, int32_t t) {
        float r = (float)zone.draw();
        r = (r + r) / 32767.0f + 3.0f;
        const int32_t h = trunc32(zone.fraction(10.0f) + r);
        auto offset = [&](int32_t X, int32_t Y, int32_t z, float &a, float &b) {
            a = simplex((double)Y * 0.1, (double)z * 0.1) * 1.5f + (float)(X - x);
            b = simplex((double)X * 0.1, (double)z * 0.1 + 843.0) * 1.5f + (float)(Y - y);
        };
        for (int32_t X = trunc32((float)x - r - 1.5f); (float)X <= (float)x + r + 1.5f; X++)
            for (int32_t Y = trunc32((float)y - r - 1.5f); (float)Y <= (float)y + r + 1.5f; Y++) {
                float a, b;
                offset(X, Y, t + 1, a, b);
                if (a * a + b * b <= r * r && !taken(m.get(X, Y, t + 2)))
                    return;
            }
        m.twoColours(c0, 0, (float)(t - h), c1, 1, (float)t, false);
        for (int32_t Z = t - h; Z <= t + 1; Z++) {
            const float rr = (float)(Z - (t - h)) / (float)h * r;
            for (int32_t X = trunc32((float)x - rr - 1.5f); (float)X <= (float)x + rr + 1.5f; X++)
                for (int32_t Y = trunc32((float)y - rr - 1.5f); (float)Y <= (float)y + rr + 1.5f; Y++) {
                    float a, b;
                    offset(X, Y, Z, a, b);
                    if (a * a + b * b <= rr * rr)
                        m.put(X, Y, Z, m.block(X, Y, Z, blocktype::ROCK, 0x22));
                }
        }
    }
};

} // namespace

int BuildingBuilder::roomCode(const Floor &fl) {
    if (fl.level)
        return k.detail;
    if (t == 45)
        return rules.fortressFloorCode(cx, cy, fl.f);
    return 0;
}

void BuildingBuilder::roomContents(Floor &fl) {
    const int structure = zone.place.structure;
    if (!insideZone() || (special && fl.level) || !(rules.floorOffset(cx, cy) > -32))
        return;
    if (fl.level ? (k.detail >= 2 && k.detail <= 5) : !isBuilding(t))
        return;
    const int code = roomCode(fl);
    Colour light = fl.floorColour;
    if (fl.level)
        castleLightColour(k.kind, light);

    // §6.1 Furniture.
    Furnisher fu(zone, m, fl.floorColour, fl.floorColour2, light);
    if (fl.level && k.style == 2) {
        if (k.detail == 0)
            fu.wall = {2, 5};
        if (k.kind == 1)
            fu.side.push_back(0);
        fu.centre = {4};
    } else if (fl.level) {
        const bool plain = k.detail == 0;
        switch (structure) {
        case 40: if (plain) fu.wall = {4, 3, 5}; fu.side = {4, 3}; break;
        case 41: if (plain) fu.wall = {6, 7, 0, 5}; fu.side = {4}; break;
        case 42: fu.side = {4}; break;
        case 43: if (plain) fu.wall = {1}; if (k.kind == 1) fu.side.push_back(0); fu.ceiling = {0}; break;
        case 44: fu.ceiling = {0}; fu.centre = {0}; break;
        case 45: if (plain) fu.wall = {4, 1, 5}; break;
        case 46: if (plain) fu.wall = {4, 2, 5}; fu.ceiling = {0}; break;
        case 47: if (plain) fu.wall = {4, 0, 5}; break;
        default: break;
        }
        if (k.kind == 1)
            fu.side.push_back(1);
        fu.centre.push_back(4);
    } else if (t >= 35 && t <= 38) {
        fu.side = {4, 1}, fu.wall = {2, 4};
    } else if (t == 45) {
        fu.ceiling = {1}, fu.side = {4, 1}, fu.wall = {0};
    }
    const int32_t w = fl.wall;
    fu.furnish(w, w, fl.z0, 2 * (8 - w), 2 * (8 - w), fl.top0 - fl.z0 - 2);

    // §6.2 Room objects.
    roomObjects(fl);

    // §6.3 Pieces by the room's code.
    auto codeSlots = [&](int p) {
        m.slots(light, p, (float)(fl.z0 - fl.depth0), fl.floorColour, p, (float)fl.z0, fl.floorColour2, p, (float)fl.top0);
    };
    auto underCeiling = [&] {
        int32_t z = fl.top0 - 2;
        for (int n = 0; n < 10; n++) {
            const Block b = m.get(8, 8, z - 1);
            if (isOpen(b) && (!(b.flags & 0x40) || (b.flags & 0x80)))
                break;
            z -= 1;
        }
        return z;
    };
    if (code == 8) {
        codeSlots(1);
        m.foundation(4, 4, fl.z0 - 1, 8, 8, 1, 0, 0);
    } else if (code == 1) {
        codeSlots(1);
        const int32_t z = underCeiling();
        if (k.style == 2 || structure == 40) {
            m.roundPost(8, 8, fl.z0, 2, z - fl.z0, true);
        } else {
            m.cappedPost(8, 8, fl.z0, 2, z - fl.z0, true);
            if (structure == 42 || structure == 44)
                m.box(4, 4, z, 8, 8, 4);
        }
    } else if (code == 12) {
        codeSlots(0);
        const int32_t h = zone.draw() % 5 + 8, r = zone.draw() % 4 + 3;
        m.hangingCone(8, 8, fl.top0 - 2, r, h);
    } else if (code == 11) {
        m.oneColour({150, 120, 50}, pattern::PLAIN);
        const int32_t z = underCeiling();
        m.foundation(7, 7, fl.z0, 2, 2, z - fl.z0, 0, 0);
        const bool alongX = zone.draw() % 2 != 0;
        if (alongX)
            m.box(0, 7, z - 2, 16, 2, 2);
        else
            m.box(7, 0, z - 2, 2, 16, 2);
        if (zone.draw() % 4 == 0) {
            const int32_t along = zone.draw() % 2 * 8 + 4;
            Model lamp;
            lamp.type = 0x17;
            lamp.x = alongX ? dotsOf(16 * cx) + dotsOf(along) : dotsOf(16 * cx + 8);
            lamp.y = alongX ? dotsOf(16 * cy + 8) : dotsOf(16 * cy) + dotsOf(along);
            lamp.z = dotsOf(z - 3);
            lamp.size = 0.1f, lamp.yaw = 0.0f, lamp.flags = modelflag::LIGHT;
            zone.keepIfInside(lamp);
        }
    }
}

// §6.2 (0x2fc270).
void BuildingBuilder::roomObjects(Floor &fl) {
    const int f = fl.f;
    const int kindP = f < 0 ? zone.place.structure : zone.place.place.kind;
    const int blk = f < 0 ? 0 : townBlock(zone.world, cx, cy, zone.place.place.kind);
    if (f >= 0 && (roof == 3 || t == 26 || t == 86 || t == 95))
        return;
    const int64_t z0 = dotsOf(fl.z0);
    const int64_t mx = dotsOf(16 * cx + 8), my = dotsOf(16 * cy + 8);
    // The mode.
    int mode = 0;
    if (f >= 0) {
        const bool hall = t == 24 || t == 100 || t == 36 || t == 37 || t == 46 || t == 71 || t == 76 || t == 81 || t == 89 ||
                          t == 91 || t == 92;
        if (hall && ((blk >= 2 && blk <= 6) || blk == 11))
            mode = 1;
        else if ((blk >= 7 && blk <= 9) || blk == 0)
            mode = 0;
        else if (blk == 2)
            mode = 2 + (f != 0 ? 1 : 0);
        else if (blk == 11)
            mode = zone.draw() % 2 != 0 ? 2 : 0;
        else {
            struct Room {
                int32_t x, y;
                int f;
            };
            std::vector<Room> ground, upper;
            const int32_t bx = 3 * floorDivBy(cx, 3), by = 3 * floorDivBy(cy, 3);
            for (int32_t a = 0; a < 3; a++)
                for (int32_t b = 0; b < 3; b++) {
                    const int nt = rules.type(bx + a, by + b);
                    for (int g = 0; g < mostFloors(nt); g++)
                        if (rules.floorExists(bx + a, by + b, g))
                            (g == 0 ? ground : upper).push_back({bx + a, by + b, g});
                }
            const Room *chosen = nullptr;
            if (!upper.empty())
                chosen = &upper[(size_t)(zone.draw() % (int)upper.size())];
            else if (!ground.empty())
                chosen = &ground[(size_t)(zone.draw() % (int)ground.size())];
            if (chosen && chosen->x == cx && chosen->y == cy && chosen->f == f)
                mode = 3;
            else
                mode = zone.draw() % 4 == 0 ? 2 : 0;
        }
    }
    if (mode == 3) {
        StaticObject a = makeObject(0x20, 0, 0, z0, 0);
        a.x = mx + dotsOf(zone.draw() % 5 - 2);
        a.y = my + dotsOf(zone.draw() % 5 - 2);
        a.turn = (uint8_t)(zone.draw() % 4);
        if (zone.settle(a, true))
            zone.keepIfInside(a);
        StaticObject b = a;
        b.type = 0x21;
        const int64_t sway = trunc64((zone.draw() % 2 != 0 ? -1.75 : 1.75) * 65536.0);
        switch (a.turn) {
        case 0: b.x += sway, b.y += DOTS; break;
        case 1: b.x -= DOTS, b.y += sway; break;
        case 2: b.x += sway, b.y -= DOTS; break;
        default: b.x += DOTS, b.y += sway; break;
        }
        zone.settleAndKeep(b);
    } else if (mode == 2) {
        StaticObject bed = makeObject(0x18, 0, 0, z0, 0);
        bed.x = mx + dotsOf(zone.draw() % 3 - 1);
        bed.y = my + dotsOf(zone.draw() % 3 - 1);
        bed.turn = (uint8_t)(zone.draw() % 4);
        if (zone.settle(bed, true))
            zone.keepIfInside(bed);
        constexpr int64_t D = 163840;
        static constexpr int64_t OFFSET[4][2] = {{0, -D}, {0, D}, {-D, 0}, {D, 0}};
        static constexpr uint8_t TURN[4] = {2, 0, 1, 3};
        for (int s = 0; s < 4; s++) {
            if (zone.draw() % 4 == 0)
                continue;
            zone.settleAndKeep(makeObject(0x1c, bed.x + OFFSET[s][0], bed.y + OFFSET[s][1], bed.z, TURN[s]));
        }
    } else if (mode == 1) {
        static constexpr int32_t AT[4][2] = {{8, 5}, {8, 11}, {5, 8}, {11, 8}};
        static constexpr uint8_t TURN[4] = {2, 0, 1, 3};
        for (int s = 0; s < 4; s++)
            zone.settleAndKeep(makeObject(0x2c, dotsOf(16 * cx + AT[s][0]), dotsOf(16 * cy + AT[s][1]), z0, TURN[s]));
    }

    // Along the walls.
    std::vector<int> list = {28, 45, 42, 74, 40, 38, 39, 37, 30, 31};
    bool full = roofed(t);
    int32_t spacing = 3;
    if (f < 0) {
        full = true;
        switch (kindP - 40) {
        case 0: list = {51, 28, 27, 90}; break;
        case 1: list = {28, 90, 31, 38, 39, 37}; break;
        case 3: case 5: case 6: list = {54, 27, 74}; break;
        case 7: list = {48, 27, 31, 90, 89}; break;
        default: list = {48, 28, 37, 38, 40, 39, 90}; break;
        }
    } else {
        if (kindP == 8)
            list = {48, 28, 31, 90, 74};
        else if (roof == 4)
            list = {51, 28, 27, 31};
        if (!full)
            list = {74}, spacing = 5;
    }
    auto takenAt = [&](int64_t x, int64_t y, int64_t z) {
        const Block b = zone.read(blocksOf(x), blocksOf(y), blocksOf(z));
        return !isOpen(b) || (b.flags & 0x40);
    };
    constexpr int64_t EDGE = 98304;
    static constexpr uint8_t WALL_TURNS[4] = {2, 0, 1, 3};
    for (int wall = 0; wall < 4; wall++)
        for (int i3 = -1; i3 <= 1; i3++) {
            const int32_t off = -spacing + (i3 + 1) * spacing;
            if (zone.draw() % 3 != 0 || (!full && i3 == 0))
                continue;
            const bool alongX = wall < 2;
            const int sign = (wall == 0 || wall == 2) ? 1 : -1;
            const int64_t a = dotsOf(16 * (alongX ? cx : cy) + 8 + off);
            int64_t b = sign > 0 ? dotsOf(16 * (alongX ? cy : cx)) + EDGE - (full ? 0 : DOTS)
                                 : dotsOf(16 * (alongX ? cy : cx) + 16) - EDGE + (full ? 0 : DOTS);
            auto at = [&](int64_t pa, int64_t pb, int64_t pz) { return alongX ? takenAt(pa, pb, pz) : takenAt(pb, pa, pz); };
            for (int n = 0; n < 4 && at(a, b, z0); n++)
                b += sign * DOTS;
            int type = list[(size_t)(zone.draw() % (int)list.size())];
            b += sign * trunc64((double)objectSize(type).y * 0.5 * 65536.0);
            if (at(a, b, z0))
                continue;
            bool open = false;
            for (int c = 0; c <= 4 && !open; c++)
                for (int d = -1; d <= 1 && !open; d++)
                    open = !at(a + dotsOf(d), b - sign * EDGE, z0 + dotsOf(c));
            if (open && full)
                continue;
            if (type == 45 || type == 48 || type == 51 || type == 54)
                type += zone.draw() % 3;
            else if (type == 0x4a)
                type += zone.draw() % 9;
            zone.settleAndKeep(makeObject(type, alongX ? a : b, alongX ? b : a, z0, WALL_TURNS[wall]));
        }
}

} // namespace worldgen
