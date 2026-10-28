// Shared creature helpers, written from docs/specs/worldgen-creatures.md §1, §4.1, §5.5 and
// worldgen-creatures-zone.md §2.5 (the castle race).
#include "worldgen/CreatureBuilder.h"

namespace worldgen {

int32_t styleBoss(uint32_t s, uint32_t n) {
    static constexpr int32_t PEOPLE[8] = {0, 2, 4, 7, 9, 11, 13, 15}; // the playable peoples, one of each pair
    const uint32_t k = xorshift(s + 0x1337c) % 11;
    const int32_t sex = (int32_t)(xorshift(n) & 1);
    switch (k) {
    case 0: return sex;
    case 8: return 48;
    case 9: return 51;
    case 10: return 76;
    default: return PEOPLE[k] + sex;
    }
}

Spot CreatureBuilder::nearest(Int2 c, int kind) {
    const auto it = zone.working.find({c.x, c.y, kind});
    if (it == zone.working.end() || it->second.empty())
        return take(c.x, c.y, kind);
    const int64_t mx = dotsOf(c.x * 16 + 8), my = dotsOf(c.y * 16 + 8);
    const Spot *best = nullptr;
    float bestD = 0.0f;
    for (const Spot &s : it->second) {
        const float dx = (float)(mx - s.x) * (1.0f / 65536.0f);
        const float dy = (float)(my - s.y) * (1.0f / 65536.0f);
        const float d = dy * dy + dx * dx;
        if (!best || d < bestD)
            best = &s, bestD = d;
    }
    return *best;
}

void CreatureBuilder::castleRace(SpawnPoint &p, int structure, uint32_t s) {
    const std::vector<int32_t> &list = castleRaces(structure);
    if (styleOdd(s) && rand() % 8 == 0) {
        p.race = styleBoss(s, (uint32_t)rand());
        p.people = styleRace(s);
        p.style = s;
        if (!styleEven(s))
            p.addFlags(looksflag::FOREIGN_STYLE);
    } else {
        p.race = list[(size_t)(rand() % (int)list.size())];
    }
    p.race = variant(p.race);
}

// --- Spawns.h --------------------------------------------------------------------------------

int64_t firstSpawnId(int32_t zx, int32_t zy) {
    const uint64_t hi = (uint64_t)(int64_t)(int32_t)((uint32_t)zy + 0x1ffffffu);
    const uint64_t lo = (uint64_t)(int64_t)(int32_t)((uint32_t)zx + 0x1ffffffu);
    return (int64_t)(((hi << 32) | lo) << 6);
}

int64_t ZoneSpawns::add(SpawnPoint p, int32_t zx, int32_t zy) {
    if (p.id == 0) {
        p.id = firstSpawnId(zx, zy);
        while (points.count(p.id))
            p.id++;
    }
    const int64_t id = p.id;
    points[id] = std::move(p);
    return id;
}

SpawnPoint *ZoneSpawns::find(int64_t id) {
    const auto it = points.find(id);
    return it == points.end() ? nullptr : &it->second;
}

} // namespace worldgen
