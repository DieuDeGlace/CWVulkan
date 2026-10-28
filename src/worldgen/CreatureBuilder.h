#pragma once
// What every creature pass of a zone shares (docs/specs/worldgen-creatures.md §0.4, §4.1, §5):
// the zone's rand() stream, the tables, the region's plan, making a spawn point's looks and gear,
// and taking spots. Internal to the world generator: ZoneCreatures.cpp (the cell pass, the
// marking pass and the refresh) and RoleThings.cpp (the region roles) use it.
#include "worldgen/BuildingRules.h"
#include "worldgen/CreatureTables.h"
#include "worldgen/Gear.h"
#include "worldgen/Looks.h"
#include "worldgen/Planner.h"
#include "worldgen/ZoneBuild.h"

namespace worldgen {

// The style helpers of a place's realm s (§4.1).
inline bool styleOdd(uint32_t s) { return (xorshift(s + 0xe6cb) & 1) != 0; }   // 0x10d310
inline bool styleEven(uint32_t s) { return (xorshift(s + 0xbd8b9) & 1) == 0; } // 0x10d2e0
inline int32_t styleRace(uint32_t s) { return 12 + (int32_t)(xorshift(s + 0x7b2650) % 11); } // 0x10cee0
inline int32_t stylePeople(uint32_t s) { return (int32_t)(xorshift(s + 0x18365) & 7) + 1; } // 0x10cd60
// What the realm's nation is made of, picked with n (0x10eb10; conversation-script.md §5.4.1).
int32_t styleBoss(uint32_t s, uint32_t n);

class CreatureBuilder {
public:
    CreatureBuilder(ZoneBuild &zone, const CreatureTables &tables, int32_t day)
        : zone(zone), tables(tables), day(day), rules(zone.cells), draws_{&zone}, rand(draws_) {}

    ZoneBuild &zone;
    const CreatureTables &tables;
    int32_t day;
    BuildingRules rules;

private:
    struct ZoneDraws {
        ZoneBuild *zone;
        int operator()() { return zone->draw(); }
    } draws_;

public:
    Draw rand;

    const RegionPlan *plan() const { return zone.planner.plan.get(); }
    void seed(uint32_t v) { zone.seed(v); }

    // --- Races ---
    int32_t variant(int32_t race) { return tables.variant(race, rand); }
    int32_t pick(Picker p, int32_t culture = -1) { return tables.pick(p, rand, culture); }
    int32_t traveller() { return tables.traveller(rand); }

    // --- Making a point (§0.4) ---
    void looks(SpawnPoint &p, bool wild = false) { makeLooks(tables, p.race, wild, rand, p.appearance); }
    void gear(SpawnPoint &p) { gear(p, p.level); }
    void gear(SpawnPoint &p, int32_t level) { makeGear({p.hostility, p.race, level, p.flags(), p.people}, rand, p.gear); }
    static void place(SpawnPoint &p, const Spot &s) { p.x = s.x, p.y = s.y, p.z = s.z; }
    int64_t add(const SpawnPoint &p) { return zone.spawns.add(p, zone.zx, zone.zy); }
    SpawnPoint &added(int64_t id) { return *zone.spawns.find(id); }

    // --- Yaws (§0.1) ---
    float yawA() { return (float)rand() / 32767.0f * 360.0f; }
    float yawB() { return (float)rand() * 360.0f / 32767.0f; }
    float quarterYaw() { return (float)(rand() % 4) * 90.0f; }

    // --- Spots (§5) ---
    int count(int32_t cx, int32_t cy, int kind) const { return zone.spotCount(cx, cy, kind); }
    Spot take(int32_t cx, int32_t cy, int kind) { return zone.takeSpot(cx, cy, kind); }
    Spot cellSpot(Int2 c) { return zone.spotInCell(c.x, c.y); }
    Spot nearest(Int2 c, int kind); // 0x2fe700
    bool inZone(Int2 c) const { return floorDiv(c.x, 2) == zone.zx && floorDiv(c.y, 2) == zone.zy; }

    // The castle race (0x31a800) of structure T for a point in the realm s.
    void castleRace(SpawnPoint &p, int structure, uint32_t s);
};

} // namespace worldgen
