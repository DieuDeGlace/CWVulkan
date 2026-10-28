#include "Missions.h"

#include "worldgen/Residents.h"

#include <cstring>
#include <map>

namespace missions {
namespace {
template <class T> T get(const uint8_t *r, size_t at) {
    T v;
    std::memcpy(&v, r + at, sizeof v);
    return v;
}
// A place cell from a position in dots (0x36160 rounds a big place's down to even).
Cell placeOf(int64_t x, int64_t y) {
    auto block = [](int64_t v) { return (int32_t)(v / 0x10000); }; // (a Fixed's int)
    Cell c{worldgen::floorDiv(block(x), 10), worldgen::floorDiv(block(y), 10)};
    if (worldgen::bigCell(c[0], c[1]))
        c = {worldgen::floorDiv(c[0], 1) * 2, worldgen::floorDiv(c[1], 1) * 2};
    return c;
}
// A position's zone, flooring (blocks of 64).
int zoneOf(int64_t dots) {
    const int64_t block = dots >= 0 ? dots >> 16 : -((-dots + 0xffff) >> 16);
    return (int)(block >= 0 ? block / 64 : (block + 1) / 64 - 1);
}
float within2(const uint8_t *a, int64_t x, int64_t y, int64_t z) {
    const float k = 1.0f / 65536.0f;
    const float dx = (float)(x - get<int64_t>(a, 0x10)) * k, dy = (float)(y - get<int64_t>(a, 0x18)) * k,
                dz = (float)(z - get<int64_t>(a, 0x20)) * k;
    return dx * dx + dy * dy + dz * dz;
}
constexpr float RANGE2 = 400.0f; // 20 blocks
} // namespace

bool completed(const std::set<Cell> &finished, const std::set<Cell> &solved, Cell place) {
    return solved.count(place) || finished.count(place);
}

bool update(World &w, Effects &out) {
    const uint8_t *me = w.player;
    if (!me || me[worldgen::creature::ABILITY] == 0x9c)
        return false;
    // The planner of the player's region (+0x150), its roles by place.
    static Cell region{INT32_MIN, INT32_MIN};
    static std::map<Cell, int> kinds;
    const Cell now{get<int32_t>(me, worldgen::creature::REGION), get<int32_t>(me, worldgen::creature::REGION + 4)};
    if (now != region) {
        region = now, kinds.clear();
        for (const auto &r : worldgen::regionRoles({now[0], now[1]}).roles)
            kinds[{r[0], r[1]}] = r[2];
    }
    auto kindAt = [&](Cell c) { // 0x93480
        auto it = kinds.find(c);
        return it == kinds.end() ? 0 : it->second;
    };
    auto finish = [&](Cell c) { w.finished->insert(c); };
    auto solve = [&](Cell c) { w.finished->insert(c), w.solved->insert(c); };

    // The residents whose task is done (flags 0x2000, toggle 0x100, an interaction) near the player.
    w.creatures([&](const uint8_t *c) {
        if (!(get<uint32_t>(c, worldgen::creature::FLAGS) & 0x2000) || !(get<uint16_t>(c, 0x128) & 0x100) || !c[0x19c])
            return;
        if (!(within2(me, get<int64_t>(c, 0x10), get<int64_t>(c, 0x18), get<int64_t>(c, 0x20)) < RANGE2))
            return;
        const Cell place = placeOf(get<int64_t>(c, worldgen::creature::HOME), get<int64_t>(c, worldgen::creature::HOME + 8));
        const int kind = kindAt(place);
        if (kind == 15)
            finish(place);
        else if (kind == 1)
            solve(place);
    });
    // The zones' objects within 20 blocks: a chest (0x15, 0x16) with an item that was opened
    // near the player finishes kind 15; a brazier (0x7e) no longer active solves kind 22.
    const int64_t px = get<int64_t>(me, worldgen::creature::POSITION), py = get<int64_t>(me, worldgen::creature::POSITION + 8);
    const int64_t reach = (int64_t)20 * 0x10000;
    for (int zx = zoneOf(px - reach); zx <= zoneOf(px + reach); zx++)
        for (int zy = zoneOf(py - reach); zy <= zoneOf(py + reach); zy++)
            for (const worldgen::ZoneObject &o : w.objects(zx, zy)) {
                const Cell place = placeOf(o.x, o.y);
                if ((o.type == 0x15 || o.type == 0x16) && o.takes.category && !w.active(o) && within2(me, o.x, o.y, o.z) < RANGE2 &&
                    kindAt(place) == 15)
                    finish(place);
                if (o.type == 0x7e && !w.active(o) && kindAt(place) == 22)
                    solve(place);
            }
    // A zone within three of the player's whose mission creatures are all dead: the player's
    // place is finished (and solved but for kinds 15-18).
    const int pzx = zoneOf(px), pzy = zoneOf(py);
    for (int dx = -3; dx < 4; dx++)
        for (int dy = -3; dy < 4; dy++) {
            const std::vector<int64_t> ids = w.missionCreatures(pzx + dx, pzy + dy);
            if (ids.empty())
                continue;
            bool all = true;
            for (int64_t id : ids)
                all = all && w.dead(id);
            if (!all)
                continue;
            const Cell place = placeOf(px, py);
            const int kind = kindAt(place);
            if (kind >= 15 && kind <= 18)
                finish(place);
            else
                solve(place);
        }
    // The region's missions: the items that complete them, and the completed ones counted.
    static const int placements[][2] = {{6, 1}, {7, 2}, {8, 0x12}, {9, 3}, {0xd, 9}, {0xc, 8}, {0xb, 7}, {10, 5}};
    int barriers = 0, barriersDone = 0, pumps = 0, pumpsDone = 0, flames = 0, witches = 0, portals = 0;
    for (const auto &[place, kind] : kinds) {
        // (Kind 18, an exchange partner's ten items: the planner never gives it.)
        // An artifact's place (role 14) is solved once the player carries the artifact hidden
        // there. (spec: open question: the lore spec leaves the mission update out; this
        // is the simplest rule consistent with its "carried" test, 0x5e780.)
        if (kind == 14 && w.carries) {
            const worldgen::Item hidden = worldgen::placeArtifact(place[0], place[1]);
            if (hidden.category != 0 && w.carries(hidden))
                solve(place);
        }
        if (kind >= 2 && kind <= 5 && w.hasSpecial(kind + 0xc)) // the crafting books
            solve(place);
        for (const auto &p : placements)
            if (kind == p[0] && w.hasSpecial(p[1]))
                solve(place);
        const bool done = completed(*w.finished, *w.solved, place);
        if (kind == 0x15)
            barriers++, barriersDone += done;
        if (kind == 0x17)
            pumps++, pumpsDone += done;
        if (kind == 0x16)
            flames += done;
        if (kind == 0x13)
            witches += done;
        if (kind == 0x14)
            portals += done;
    }
    out.witch = witches > 0;
    out.demon = portals > 0;
    out.barriers = barriers != 0 && barriersDone == barriers;
    out.flames = flames;
    out.pumpsLeft = pumps - pumpsDone;
    return true;
}

} // namespace missions
