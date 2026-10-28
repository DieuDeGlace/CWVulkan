// What a place gets from its region role (0x11a820), written from
// docs/specs/worldgen-creatures-roles.md (sections cited as §n). Roles 6-13 lay down an item,
// which part B does (ZoneContents.cpp, roleItem); everything else is here.
#include "worldgen/CreatureBuilder.h"
#include "worldgen/Height.h"
#include "worldgen/ZoneCreatures.h"

#include <algorithm>
#include <functional>

namespace worldgen {
namespace {

using namespace looksflag;

constexpr uint32_t CAMP_SEED = 0x9a11;
constexpr uint32_t PICK_SALT = 0x4192;
constexpr uint32_t TURN_SALT = 0x326a0;
constexpr uint32_t GUARDS_SALT = 0x5d14e;
constexpr uint32_t CELL_SEED = 0x4f2f90;
constexpr uint8_t CHEST_ITEM = 0x16;

// The place the zone belongs to and what every builder reads of it (§1 step 2).
struct RolePlace {
    Int2 P;          // the place cell (a big place's even corner)
    bool big = false;
    int kind = 0;
    int role = 0;
    int lvl = 0;
    Int2 centre;     // the landmark point
    uint32_t ph = 0; // H(P)
    Item treasure;
    Int2 centreCell() const { return {floorDiv(centre.x, 4), floorDiv(centre.y, 4)}; }
};

class Roles {
public:
    Roles(CreatureBuilder &b) : b(b), plan(*b.plan()) {
        const ZonePlace &zp = b.zone.place;
        r.P = zp.cell;
        r.big = bigCell(r.P.x, r.P.y);
        r.kind = placeKind(r.P.x, r.P.y, r.big);
        r.role = plan.roleAt(r.P.x, r.P.y);
        r.lvl = plan.levelAt(r.P.x, r.P.y);
        r.centre = cellPoint(r.P.x, r.P.y, r.big);
        r.ph = cellKey(r.P.x, r.P.y);
        r.treasure = plan.roleTreasure(r.P.x, r.P.y);
    }

    void run();

private:
    CreatureBuilder &b;
    const RegionPlan &plan;
    RolePlace r;

    uint32_t R(uint32_t salt) const { return random(r.ph + salt); }
    ZoneSpawns &spawns() { return b.zone.spawns; }

    // --- §1.1 Shared helpers ---
    float weightAt(int32_t x, int32_t y) const {
        return placeWeight(r.kind, r.big, warpedDist2(r.centre, 16 * x + 8, 16 * y + 8));
    }
    std::vector<Int2> placeCells(float limit, const std::function<bool(int32_t, int32_t)> &fits) const {
        const int32_t E = r.big ? 0x800 : 0x400;
        const int32_t x0 = floorDiv(wrapMul(1024, r.P.x), 4), x1 = floorDiv(wrapMul(1024, r.P.x) + E, 4);
        const int32_t y0 = floorDiv(wrapMul(1024, r.P.y), 4), y1 = floorDiv(wrapMul(1024, r.P.y) + E, 4);
        std::vector<Int2> out;
        for (int32_t x = x0; x < x1; x++)
            for (int32_t y = y0; y < y1; y++)
                if (weightAt(x, y) > limit && fits(x, y))
                    out.push_back({x, y});
        if (out.empty())
            out.push_back(r.centreCell());
        return out;
    }
    static bool plain(int t) { return (t >= 0 && t <= 12 && ((0x1c3f >> t) & 1)) || t == 99; }
    enum GroundTest { LAVA, RIVER, OPEN };
    static bool ground(int32_t x, int32_t y, GroundTest test) {
        const int32_t mx = 16 * x + 8, my = 16 * y + 8;
        const Place p = placeAt(mx, my);
        if (p.variant != 0 && !(p.weight >= 0.8f))
            return false;
        const ColumnValues c = columnValues(mx, my, p, 0);
        switch (test) {
        case LAVA: return c.lava > 0.5f;
        case RIVER: return c.river > 0.5f;
        default: return !(c.river > 0.5f) && !(c.lava > 0.5f) && !(c.swamp > 0.5f) && groundHeight(mx, my, 0) >= 1.0f;
        }
    }
    int type(int32_t x, int32_t y) { return b.rules.type(x, y); }
    bool fitsKind(int32_t x, int32_t y) { // 0x128aa0
        const int t = type(x, y);
        switch (r.kind) {
        case 1: case 2: case 3: return t >= 0x16 && t <= 0x18;
        case 4: return t >= 0x23 && t <= 0x26 && t != 0x25;
        case 7: return t == 0x34;
        case 0x15: return t == 0xc;
        case 0x16: return t >= 0x57 && t <= 0x59;
        case 0x1d: return t >= 10 && t <= 12;
        case 0x22: case 0x23: return t == 99;
        case 0x25: return t == 0x39;
        case 0x26: return t == 0x45 || t == 0x46;
        default: return plain(t) && ground(x, y, OPEN);
        }
    }
    bool fitsGroup(int32_t x, int32_t y) { // 0x128c10
        const int t = type(x, y);
        switch (r.kind) {
        case 0x15: case 0x1d: return t >= 10 && t <= 12;
        case 0x22: case 0x23: return t == 0x62;
        default: return plain(t) && ground(x, y, OPEN);
        }
    }
    std::vector<Int2> campCells() {
        return placeCells(0.5f, [&](int32_t x, int32_t y) { return plain(type(x, y)) && ground(x, y, OPEN); });
    }
    std::vector<Int2> kindCells() {
        return placeCells(0.5f, [&](int32_t x, int32_t y) { return fitsKind(x, y); });
    }
    // 0x115b80.
    void shuffle(std::vector<Int2> &v) {
        for (size_t i = 1; i < v.size(); i++) {
            const int m = (int)i + 1;
            int x;
            do
                x = b.rand();
            while (!(x / m < 32767 / m || 32767 % m == m - 1));
            std::swap(v[i], v[(size_t)(x % m)]);
        }
    }
    void carry(SpawnPoint &p, const Item &item) {
        if (item.category != 0)
            p.gear.inventory.add(item, 1);
    }
    // The chest item of a camp (§3.1 step 2, §3.2 step 2).
    Item chestItem() const {
        Item it;
        it.category = CHEST_ITEM;
        it.id = (posMod(r.P.y, 64) * 64 + posMod(r.P.x, 64)) % 20;
        it.rarity = (uint8_t)std::min(r.lvl + 1, 4);
        return it;
    }
    int64_t addMission(const SpawnPoint &p, bool mission) {
        const int64_t id = b.add(p);
        if (mission)
            spawns().mission.push_back(id);
        return id;
    }
    // The ruler of the place (0x2ba810), -1 for none.
    int32_t ruler() const {
        static constexpr int32_t PEOPLE[16] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        const uint32_t v = random(r.ph + 0x8e736);
        if (r.kind >= 1 && r.kind <= 3)
            return PEOPLE[v & 0xf];
        if (r.kind == 7)
            return (v & 1) ? 0xa7 : 0x2b;
        if (r.kind == 22)
            return 0x2d;
        return -1;
    }

    // --- §3 Encounters ---
    void encounter();
    void questCamp(int32_t giver, int32_t G, int32_t Gp);
    void guardedChest();
    SpawnPoint campGuard(const std::vector<int32_t> &races, int32_t people, Int2 c);
    void campTraveller(int32_t people, Int2 c);
    void bossCamp(int32_t boss, const std::vector<int32_t> &races, int32_t people);
    void groupCamp(int32_t leader, uint32_t leaderFlags, const std::vector<int32_t> &races, int32_t people);
    void lonelyCreature();
    void bossWithGuards(int32_t boss, int32_t guard, int32_t people, bool leader);
    void arenaTeams();
    // --- §4 Set pieces ---
    void towerPeople();
    void witchHut();
    void brazierKeepers();
    void spiderNests();
};

void Roles::run() {
    spawns().mission.clear();
    switch (r.role) {
    case 0: return;
    case role::TOWER: towerPeople(); return;
    case role::WITCH: witchHut(); return;
    case role::DEMON_PORTAL: bossWithGuards(0xaf, 0xe3, 4, false); return;
    case role::BRAZIERS: brazierKeepers(); return;
    case role::SPIDERS: spiderNests(); return;
    default:
        if (r.role >= 6 && r.role <= 13)
            return; // part B's role item
        encounter();
        return;
    }
}

// --- §3 ------------------------------------------------------------------------------------------

void Roles::encounter() {
    b.seed(R(0x21559a6e));
    const int e = plan.encounter(r.P.x, r.P.y, b.day);
    // The quest camps' giver: the gnome's supplier for role 1, else a people pick, drawn only for
    // them (the traces show no pick before the other encounters).
    auto giver = [&] { return r.role == role::GNOME ? 0x131 : b.pick(Picker::People); };
    switch (e) {
    case 0: guardedChest(); break;
    case 1: lonelyCreature(); break;
    case 2: {
        const Int2 u{floorDiv(64 * b.zone.zx, PLACE_SHIFT), floorDiv(64 * b.zone.zy, PLACE_SHIFT)};
        bossCamp((int32_t)(random(cellKey(u.x, u.y) + 0x1c52c9) % 3) + 0xef, {0xec}, 6);
        break;
    }
    case 3: bossCamp(0xaf, {227, 85, 226, 225}, 4); break;
    case 4: bossCamp(0xb2, {0x60, 0xe6}, 5); break;
    case 5: bossCamp(0xb3, {17, 173, 166, 165, 0xe6}, 5); break;
    case 6: bossCamp(0xb0, {37, 40, 38, 39}, 0); break;
    case 7: bossCamp(0xb4, {0x57}, 0); break;
    case 8: bossCamp(0xb1, {0x35}, 0); break;
    case 9: {
        const int32_t g = giver();
        questCamp(g, b.pick(Picker::PeopleOrBeasts), 0);
        break;
    }
    case 10: questCamp(giver(), 0x2e, 0); break;
    case 11: questCamp(giver(), 0x2d, 0); break;
    case 12: questCamp(giver(), 0x11, 5); break;
    case 13: questCamp(giver(), 0x10d, 5); break;
    case 14: questCamp(giver(), 0xe6, 5); break;
    case 15: questCamp(giver(), 0xe3, 4); break;
    case 16: questCamp(giver(), 0xe2, 4); break;
    case 17: groupCamp(0x6b, GIANT, {0x6b}, 0); break;
    case 18: {
        const int32_t race = std::max(r.lvl - 1, 0) + 0x25;
        groupCamp(race, GIANT, {race}, 0);
        break;
    }
    case 19: {
        const int32_t a = b.pick(Picker::Undead), c = b.pick(Picker::Undead), leader = b.pick(Picker::UndeadLeader);
        groupCamp(leader, STRONG, {a, c}, 5);
        break;
    }
    case 20: {
        const int32_t a = b.pick(Picker::Beasts), c = b.pick(Picker::Beasts), leader = b.pick(Picker::BeastLeader);
        groupCamp(leader, STRONG, {a, c}, 4);
        break;
    }
    case 21: groupCamp(b.variant(0xec), STRONG, {0xec}, 6); break;
    case 22: {
        const int32_t a = b.pick(Picker::Lizards), c = b.pick(Picker::Lizards), leader = b.pick(Picker::Lizards);
        groupCamp(leader, GIANT, {a, c}, 0);
        break;
    }
    case 23: {
        const int32_t a = b.pick(Picker::Golems), c = b.pick(Picker::Golems), leader = b.pick(Picker::Golems);
        groupCamp(leader, GIANT, {a, c}, 0);
        break;
    }
    case 27: arenaTeams(); break;
    case 30: bossWithGuards(b.pick(Picker::Undead), 0xe6, 5, true); break;
    case 31: bossWithGuards((int32_t)(random(r.ph + 0x1d7d7d) & 1) + 0xb2, 0xe6, 5, false); break;
    case 32: bossWithGuards(b.pick(Picker::Beasts), 0xe3, 4, true); break;
    default: break; // 28, 29: nothing; 24, 26 never happen
    }
}

// §3.1.
void Roles::questCamp(int32_t giver, int32_t G, int32_t Gp) {
    // 1.
    const bool bigUnaligned = bigCell(r.P.x, r.P.y);
    std::vector<int> layouts = {0, 3};
    if (Gp == 4 || Gp == 5)
        layouts = {4, 2};
    if (G == 0x2d || G == 0x2e || G == 0x11 || G == 0xa5 || G == 0x60 || G == 0xe2 || G == 0x10d)
        layouts = {1};
    const int layout = layouts[R(0xe2ef) % layouts.size()];
    const bool holds = layout == 0 || layout == 2 || layout == 4;
    // 2.
    b.seed(R(CAMP_SEED));
    Item chest = chestItem();
    chest.region = regionAt(r.centre.x, r.centre.y);
    // 3.
    const std::vector<Int2> kinds = kindCells();
    const Int2 pick = kinds[R(PICK_SALT) % kinds.size()];
    const std::vector<Int2> others =
        placeCells(0.5f, [&](int32_t x, int32_t y) { return !(Int2{x, y} == pick) && fitsGroup(x, y); });
    // 4.
    auto guard = [&](Int2 c, uint32_t flags) {
        SpawnPoint p;
        p.level = r.lvl;
        p.race = b.variant(G);
        p.people = Gp;
        p.roam = 0x30;
        p.addFlags(flags);
        b.looks(p);
        b.gear(p);
        b.place(p, b.cellSpot(c));
        return p;
    };
    // 5.
    if (b.inZone(pick)) {
        static constexpr int32_t OBJECTS[5] = {0, 0x58, 0x5a, 0x5b, 0x59};
        const int32_t type = layout == 0 ? (int32_t)(R(0xeb2a) & 3) + 0x54 : OBJECTS[layout];
        const Spot s = b.cellSpot(pick);
        StaticObject o = makeObject(type, s.x, s.y, s.z, (uint8_t)(R(TURN_SALT) & 3));
        if (holds)
            o.takes = chest;
        b.zone.settle(o, false);
        SpawnPoint g;
        g.hostility = hostility::FRIENDLY;
        g.level = r.lvl;
        g.race = giver;
        g.roam = 0x30;
        g.addFlags(QUEST_GIVER | RESIDENT);
        g.home = r.P;
        b.looks(g);
        b.gear(g);
        g.x = o.x, g.y = o.y, g.z = o.z;
        carry(g, r.treasure); // it carries the treasure from the moment it is added
        b.add(g);
        o.scale = g.appearance.get<float>(Appearance::SCALE) / 0.8f;
        b.zone.keepIfInside(o);
        if (!holds) {
            const int n = (int)(R(GUARDS_SALT) & 3) + 5;
            for (int i = 0; i < n; i++) {
                SpawnPoint p = guard(pick, RESIDENT);
                p.area.push_back(pick);
                p.home = r.P;
                b.add(p);
            }
            return;
        }
    } else if (!holds) {
        return;
    }
    // 7.
    const bool small = !bigUnaligned;
    const Int2 pick2 = others[R(0x19dbe) % others.size()];
    if (b.inZone(pick2)) {
        SpawnPoint holder = guard(pick2, RESIDENT | STRONG);
        carry(holder, chest);
        b.add(holder);
        if (small) {
            const int n = (int)(R(GUARDS_SALT) % 3) + 4;
            for (int i = 0; i < n; i++)
                b.add(guard(pick2, RESIDENT));
        }
    }
    // 8. Big places: guards made for the rest of the camp's cells but never added.
    if (small)
        return;
    b.seed(R(CAMP_SEED));
    std::vector<Int2> rest = others;
    shuffle(rest);
    const size_t n = others.size();
    const size_t keep = std::max(std::min(n, (size_t)32), n / 8);
    for (size_t i = 0; i < keep && i < rest.size(); i++)
        if (b.inZone(rest[i]))
            guard(rest[i], RESIDENT);
}

// §3.2.
void Roles::guardedChest() {
    b.seed(R(CAMP_SEED));
    const int32_t race = b.pick(Picker::PeopleOrBeasts);
    const Item item = chestItem();
    const std::vector<Int2> kinds = kindCells();
    const Int2 pick = kinds[R(PICK_SALT) % kinds.size()];
    if (!b.inZone(pick))
        return;
    const Spot s = b.cellSpot(pick);
    StaticObject o = makeObject(0x15, s.x, s.y, s.z, (uint8_t)(R(TURN_SALT) & 3));
    b.zone.settle(o, false);
    b.zone.keepIfInside(o);
    const int n = (int)(R(GUARDS_SALT) & 3) + 5;
    for (int i = 0; i < n; i++) {
        SpawnPoint p;
        p.level = r.lvl;
        p.race = b.variant(race);
        p.roam = 0x30;
        p.addFlags(RESIDENT);
        p.home = r.P;
        if (i == 0) {
            p.addFlags(STRONG);
            carry(p, item);
        }
        b.looks(p);
        b.gear(p);
        b.place(p, b.cellSpot(pick));
        p.area.push_back(pick);
        b.add(p);
    }
}

// §3.3.
SpawnPoint Roles::campGuard(const std::vector<int32_t> &races, int32_t people, Int2 c) {
    SpawnPoint p;
    p.level = r.lvl;
    p.race = b.variant(races[(size_t)(b.rand() % (int)races.size())]);
    p.people = people;
    p.roam = 0x30;
    p.addFlags(RESIDENT);
    p.home = r.P;
    b.looks(p);
    b.gear(p);
    if ((p.race == 0xec || p.race == 0xed) && b.rand() % 2 != 0) {
        b.place(p, b.take(c.x, c.y, 0));
        p.mode = 0x8a; // it drops from above
    } else {
        b.place(p, b.cellSpot(c));
    }
    return p;
}

void Roles::campTraveller(int32_t people, Int2 c) {
    SpawnPoint p;
    p.hostility = hostility::FRIENDLY;
    p.level = r.lvl;
    if (people == 4 || people == 5) {
        p.race = b.traveller();
        p.gear.cls = 0x8d;
        p.people = 1;
    } else {
        p.race = b.pick(Picker::KindEightPeople);
        p.gear.cls = 0x8f;
        p.people = 8;
    }
    p.roam = 0x2030;
    p.addFlags(RESIDENT);
    b.looks(p);
    b.gear(p);
    b.place(p, b.cellSpot(c));
    const int64_t id = b.add(p);
    const bool pet = p.people == 1 ? b.rand() % 4 == 0 : b.rand() % 8 == 0;
    if (!pet)
        return;
    SpawnPoint q;
    q.hostility = hostility::NEUTRAL;
    q.level = r.lvl;
    q.addFlags(RESIDENT);
    if (p.people == 1) {
        q.race = 0xd1;
        q.roam = 0x90;
        q.owner = id;
    } else {
        q.race = 0x107;
        q.roam = 0x10;
    }
    b.looks(q);
    b.gear(q);
    b.place(q, b.cellSpot(c));
    b.add(q);
}

// §3.4.
void Roles::bossCamp(int32_t boss, const std::vector<int32_t> &races, int32_t people) {
    b.seed(R(CAMP_SEED));
    const std::vector<Int2> kinds = kindCells();
    const Int2 pick = kinds[(size_t)(b.rand() % (int)kinds.size())];
    std::vector<Int2> open = campCells();
    shuffle(open);
    const int n = (int)open.size();
    const int r5 = b.rand() % 5 + 1, r8 = b.rand() % 5 + 8;
    const int count = std::min(r8, std::max(1, std::max(n / 20, std::min(r5, n / 2))));
    spawns().tracked.clear();
    if (b.inZone(pick)) {
        SpawnPoint host;
        host.hostility = hostility::FRIENDLY;
        host.level = r.lvl;
        const int32_t ruled = ruler();
        if (ruled >= 0) {
            host.race = ruled;
            if (r.kind >= 1 && r.kind <= 3)
                host.gear.cls = 0x8a;
        } else if (people == 4 || people == 5) {
            host.race = b.traveller(), host.gear.cls = 0x8d, host.people = 1;
        } else {
            host.race = b.pick(Picker::KindEightPeople), host.gear.cls = 0x8f, host.people = 8;
        }
        host.addFlags(QUEST_GIVER | RESIDENT);
        host.roam = 0x2030;
        b.place(host, b.cellSpot(pick));
        b.looks(host);
        b.gear(host);
        carry(host, r.treasure);
        b.add(host);
    }
    const bool guards = boss == 0xb2 || boss < 0xaf || boss > 0xb4;
    for (int i = 0; i < n; i++) {
        if (i >= count && b.rand() % 12 != 0)
            continue;
        const Int2 c = open[(size_t)i];
        if (!b.inZone(c))
            continue;
        b.seed(random(cellKey(c.x, c.y) + CELL_SEED));
        if (i < count) {
            SpawnPoint p;
            p.hostility = hostility::ANIMAL;
            p.level = r.lvl;
            p.race = boss;
            p.addFlags(RESIDENT);
            p.home = r.P;
            b.place(p, b.cellSpot(c));
            p.yaw = b.quarterYaw();
            b.looks(p);
            b.gear(p);
            spawns().tracked.push_back(b.add(p));
        }
        if (guards)
            b.add(campGuard(races, people, c));
    }
}

// §3.5.
void Roles::groupCamp(int32_t leader, uint32_t leaderFlags, const std::vector<int32_t> &races, int32_t people) {
    b.seed(R(CAMP_SEED));
    const std::vector<Int2> kinds = kindCells();
    const Int2 pick = kinds[(size_t)(b.rand() % (int)kinds.size())];
    std::vector<Int2> open = campCells();
    shuffle(open);
    open.resize(std::max<size_t>(1, std::max<size_t>(open.size() / 8, 8)), Int2{0, 0});
    spawns().tracked.clear();
    if (b.inZone(pick)) {
        SpawnPoint p;
        p.level = r.lvl;
        p.race = leader;
        p.people = people;
        p.addFlags(leaderFlags | QUEST_GIVER | RESIDENT);
        p.roam = 0x30;
        p.requiredDefeated = std::max(1, std::min(20, (int)open.size() / 2));
        b.place(p, b.cellSpot(pick));
        if (r.treasure.category != 0) {
            carry(p, r.treasure);
            p.addFlags(BIG);
        }
        b.looks(p);
        b.gear(p);
        addMission(p, r.treasure.category == 0);
    }
    for (const Int2 &c : open) {
        if (!b.inZone(c))
            continue;
        b.seed(random(cellKey(c.x, c.y) + CELL_SEED));
        spawns().tracked.push_back(b.add(campGuard(races, people, c)));
        if (b.rand() % 10 == 0)
            campTraveller(people, c);
    }
}

// §3.9.
void Roles::lonelyCreature() {
    b.seed(R(CAMP_SEED));
    static const std::vector<int32_t> ONE = {0x6b};
    static const std::vector<int32_t> MANY = {0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x73, 0x74, 0x75, 0x76, 0x77};
    const std::vector<int32_t> &list = r.kind == 0x17 || r.kind == 0x24 ? ONE : MANY;
    const int32_t race = list[random(r.ph + 0x2b3a706) % list.size()];
    const std::vector<Int2> cells = r.kind == 0x18 ? std::vector<Int2>{r.centreCell()} : kindCells();
    const Int2 pick = cells[R(PICK_SALT) % cells.size()];
    if (!b.inZone(pick))
        return;
    SpawnPoint p;
    p.level = r.lvl;
    p.race = b.variant(race);
    p.roam = 0x30;
    p.addFlags(GIANT | RESIDENT);
    p.home = r.P;
    carry(p, r.treasure);
    b.looks(p);
    b.gear(p);
    b.place(p, b.cellSpot(pick));
    addMission(p, r.treasure.category == 0);
}

// §3.10.
void Roles::bossWithGuards(int32_t boss, int32_t guard, int32_t people, bool leader) {
    b.seed(R(CAMP_SEED));
    b.pick(Picker::PeopleOrBeasts); // drawn and dropped
    const std::vector<Int2> kinds = kindCells();
    const Int2 pick = kinds[R(PICK_SALT) % kinds.size()];
    if (!b.inZone(pick))
        return;
    const bool demon = boss == 0xaf && !leader;
    SpawnPoint p;
    p.hostility = leader ? hostility::HOSTILE : hostility::ANIMAL;
    p.level = r.lvl;
    p.race = b.variant(boss);
    if (leader)
        p.people = people, p.roam = 0x10;
    p.addFlags(demon ? (GIANT | RESIDENT) : RESIDENT);
    p.home = r.P;
    b.place(p, demon ? b.nearest(pick, 0) : b.cellSpot(pick));
    if (r.treasure.category != 0) {
        carry(p, r.treasure);
        if (leader)
            p.addFlags(BIG | STRONG);
    }
    b.looks(p);
    b.gear(p);
    const int64_t id = addMission(p, true);
    const int n = b.rand() % 4 + 3;
    for (int i = 0; i < n; i++) {
        SpawnPoint g;
        g.level = r.lvl;
        g.race = b.variant(guard);
        g.people = people;
        g.roam = 0x4000;
        g.addFlags(RESIDENT);
        g.home = r.P;
        b.place(g, b.cellSpot(pick));
        g.owner = id;
        b.looks(g);
        b.gear(g);
        b.add(g);
    }
}

// §3.11.
void Roles::arenaTeams() {
    b.seed(R(CAMP_SEED));
    const Int2 middle = r.centreCell();
    if (!b.inZone(middle))
        return;
    std::set<int64_t> previous;
    const uint32_t sigma = b.zone.place.style;
    for (int t = 0; t < 4; t++) {
        int size = (int)(random(0x452u * (uint32_t)t + 0x2fdd8u + r.ph) % 3) + 1;
        const int drawn = b.rand();
        const int kind = t == 3 ? 0 : drawn % 10;
        uint32_t flags = STRONG, style = 0;
        int32_t people = 0;
        std::vector<int32_t> races;
        switch (kind) {
        case 1: races = {b.pick(Picker::PeopleOrBeasts)}; break;
        case 2: races = {69, 70, 71, 72, 0x11f, 0x122, 0x123}; break;
        case 3: races = {102, 104, 105, 103}; break;
        case 4: races = {0x28}; break;
        case 5: races = {97, 230, 15, 165, 17, 269, 96, 41, 0xe6, 0xad, 0xa6}, people = 5; break;
        case 6: races = {226, 85, 73, 229, 0xe1, 0xe3}, people = 4; break;
        case 7: races = {0x104}, people = 3; break;
        case 8:
            style = sigma;
            races = {styleBoss(sigma, (uint32_t)b.rand())};
            people = styleRace(sigma);
            if (!styleEven(sigma))
                flags = FOREIGN_STYLE | STRONG;
            break;
        case 9: races = {0xec, 0xed, 0x106}, people = 3; break;
        default: races = {108, 109, 114, 115, 119, 117, 118, 111, 0x71, 0x70, 0x74}, flags = GIANT, size = 1; break;
        }
        int keep = size;
        if (size > 1 && b.rand() % 2 != 0)
            size *= 2, keep = 1;
        std::set<int64_t> team;
        for (int i = 0; i < size; i++) {
            if (i >= keep)
                flags &= ~STRONG;
            SpawnPoint p;
            p.level = t + 1;
            p.race = b.variant(races[(size_t)(b.rand() % (int)races.size())]);
            p.roam = 0x10;
            p.addFlags(flags | RESIDENT);
            p.home = r.P;
            p.prerequisites = previous;
            p.people = people;
            p.style = style;
            p.requiredStage = 2 * t + 1;
            b.looks(p);
            b.gear(p);
            b.place(p, b.cellSpot(middle));
            const bool last = t == 3 && i == 0;
            if (last)
                carry(p, r.treasure);
            const int64_t id = b.add(p);
            if (last)
                spawns().mission.push_back(id);
            team.insert(id);
        }
        previous = team;
    }
}

// --- §4 ------------------------------------------------------------------------------------------

// §4.1.
void Roles::towerPeople() {
    constexpr int FLOORS = 4; // 0x2d9da0
    Int2 tower;
    bool found = false;
    for (int32_t x = 4 * b.zone.zx; x < 4 * b.zone.zx + 4; x++)
        for (int32_t y = 4 * b.zone.zy; y < 4 * b.zone.zy + 4; y++) {
            if (type(x, y) != 0x34)
                continue;
            tower = {x, y}, found = true;
            SpawnPoint p;
            p.race = ruler();
            if (r.kind >= 1 && r.kind <= 3)
                p.gear.cls = 0x8a;
            p.yaw = b.quarterYaw();
            p.level = r.lvl;
            p.addFlags(RESIDENT | STRONG);
            p.home = r.P;
            b.looks(p);
            b.gear(p);
            b.place(p, b.take(x, y, FLOORS + 6));
            const int64_t ruler = b.add(p);
            SpawnPoint pet;
            pet.hostility = hostility::PET;
            pet.race = 0x121;
            pet.yaw = b.quarterYaw();
            pet.level = r.lvl;
            pet.addFlags(RESIDENT);
            pet.owner = ruler;
            pet.home = r.P;
            b.looks(pet);
            b.gear(pet);
            b.place(pet, b.take(x, y, FLOORS + 6));
            spawns().mission.push_back(b.add(pet));
        }
    if (!found)
        return;
    b.seed(random(cellKey(r.P.x + 0x2317, r.P.y + 0x2ab6)));
    struct Dweller {
        int32_t race;
        uint32_t flags;
    };
    Dweller floors[FLOORS];
    for (Dweller &d : floors) {
        d.flags = 0;
        switch (b.rand() % 8) {
        case 1: d.race = b.pick(Picker::Beasts); break;
        case 2: d.race = b.pick(Picker::Undead); break;
        case 3: d.race = b.pick(Picker::Golems); break;
        case 4: d.race = b.rand() % 4 + 0x25; break;
        case 5: d.race = b.pick(Picker::TowerFolk); break;
        case 6: d.race = b.pick(Picker::Lizards); break;
        case 7: d.race = b.pick(Picker::PeopleOrBeasts), d.flags = FOREIGN_STYLE; break;
        default: d.race = b.pick(Picker::PeopleOrBeasts); break;
        }
    }
    const int low = std::min(r.lvl, 1);
    for (int32_t cx = tower.x - 1; cx <= tower.x + 1; cx++)
        for (int32_t cy = tower.y - 1; cy <= tower.y + 1; cy++)
            for (int i = 0; i < FLOORS; i++) {
                if (cx == tower.x && cy == tower.y && i == FLOORS - 1)
                    continue;
                if (b.rand() % 2 == 0 || b.count(cx, cy, i + 7) == 0)
                    continue;
                SpawnPoint p;
                p.race = floors[i].race;
                p.yaw = b.quarterYaw();
                p.level = (r.lvl - low) * i / (FLOORS - 1) + low;
                p.addFlags(floors[i].flags | RESIDENT);
                p.roam = 0x30;
                b.looks(p);
                b.gear(p);
                b.place(p, b.take(cx, cy, i + 7));
                b.add(p);
            }
}

// §4.2.
void Roles::witchHut() {
    if (r.kind == 0x16) {
        const Int2 m = r.centreCell();
        std::vector<Int2> cells;
        for (int32_t x = m.x - 4; x <= m.x + 3; x++)
            for (int32_t y = m.y - 4; y <= m.y + 3; y++)
                if (type(x, y) == 0x58)
                    cells.push_back({x, y});
        if (!cells.empty()) {
            const Int2 pick = cells[(size_t)(b.rand() % (int)cells.size())];
            if (b.inZone(pick)) {
                const Spot s = b.nearest(pick, 2);
                StaticObject o = makeObject(0x58, s.x, s.y, s.z, (uint8_t)(R(TURN_SALT) & 3));
                b.zone.settle(o, false);
                b.zone.keepIfInside(o);
            }
        }
    }
    b.seed(R(CAMP_SEED));
    const std::vector<Int2> kinds = kindCells();
    const Int2 pick = kinds[R(PICK_SALT) % kinds.size()];
    if (!b.inZone(pick))
        return;
    SpawnPoint p;
    p.level = r.lvl;
    p.race = b.variant(0x2d);
    p.roam = 0x30;
    p.addFlags(RESIDENT | STRONG);
    p.home = r.P;
    carry(p, r.treasure);
    b.looks(p);
    b.gear(p);
    b.place(p, b.cellSpot(pick));
    p.area.push_back(pick);
    addMission(p, r.treasure.category == 0);
}

// §4.3.
void Roles::brazierKeepers() {
    for (int32_t x = 4 * b.zone.zx; x < 4 * b.zone.zx + 4; x++)
        for (int32_t y = 4 * b.zone.zy; y < 4 * b.zone.zy + 4; y++) {
            if (type(x, y) != 0x38)
                continue;
            Item flame;
            flame.category = CHEST_ITEM, flame.id = 0x49, flame.rarity = (uint8_t)r.lvl;
            SpawnPoint p;
            p.race = b.pick(Picker::PeopleOrBeasts);
            p.gear.cls = 1;
            p.people = 0xb;
            p.yaw = b.quarterYaw();
            p.level = r.lvl;
            p.area.push_back({x, y});
            p.addFlags(FOREIGN_STYLE | RESIDENT | STRONG);
            p.roam = 0x30;
            carry(p, flame);
            b.looks(p);
            b.gear(p);
            b.place(p, b.cellSpot({x, y}));
            b.add(p);
            StaticObject o = makeObject(0x7e, 0, 0, 0, (uint8_t)(b.rand() % 4));
            o.takes = flame;
            const Spot s = b.cellSpot({x, y});
            o.x = s.x, o.y = s.y, o.z = s.z;
            b.zone.settleAndKeep(o);
        }
}

// §4.4.
void Roles::spiderNests() {
    std::vector<Int2> cisterns, towers, open;
    const int32_t E = r.big ? 0x800 : 0x400;
    const int32_t x0 = floorDiv(wrapMul(1024, r.P.x), 4), x1 = floorDiv(wrapMul(1024, r.P.x) + E, 4);
    const int32_t y0 = floorDiv(wrapMul(1024, r.P.y), 4), y1 = floorDiv(wrapMul(1024, r.P.y) + E, 4);
    for (int32_t x = x0; x < x1; x++)
        for (int32_t y = y0; y < y1; y++) {
            const int t = type(x, y);
            if (t == 0x44)
                cisterns.push_back({x, y});
            else if (t == 0x42)
                towers.push_back({x, y});
            else if (weightAt(x, y) > 0.5f)
                open.push_back({x, y});
        }
    shuffle(open);
    open.resize(std::max<size_t>(1, std::max<size_t>(open.size() / 8, 8)), Int2{0, 0});
    // A spider of the lower level draws its level first, each its own (the traces show one draw
    // per spider, not one for the place as the spec reads).
    constexpr int LOWER = -1;
    auto spider = [&](Int2 c, int level, uint32_t flags) {
        SpawnPoint p;
        p.level = level == LOWER ? std::max(r.lvl - b.rand() % 2, 0) : level;
        p.race = b.variant(0xec);
        p.people = 6;
        p.addFlags(flags);
        p.home = r.P;
        b.place(p, b.cellSpot(c));
        p.roam = 0x10;
        b.looks(p);
        b.gear(p);
        return p;
    };
    int64_t last = 0;
    auto pet = [&](int32_t race, Int2 c) {
        SpawnPoint p;
        p.hostility = hostility::PET;
        p.level = r.lvl;
        p.race = race;
        p.addFlags(GIANT | RESIDENT);
        p.home = r.P;
        b.place(p, b.nearest(c, 0));
        p.yaw = b.quarterYaw();
        p.owner = last;
        b.looks(p);
        b.gear(p);
        const int64_t id = b.add(p);
        (race == 0xef ? spawns().mission : spawns().tracked).push_back(id);
        last = id;
    };
    for (const Int2 &c : cisterns)
        if (b.inZone(c)) {
            last = b.add(spider(c, LOWER, RESIDENT | STRONG));
            pet(0xf0, c);
        }
    for (const Int2 &c : open)
        if (b.inZone(c))
            last = b.add(spider(c, LOWER, RESIDENT));
    for (const Int2 &c : towers)
        if (!b.inZone(c))
            continue;
        else {
            SpawnPoint s = spider(c, r.lvl, RESIDENT | STRONG);
            s.requiredDefeated = (int32_t)cisterns.size();
            const int64_t spiderId = b.add(s);
            last = spiderId;
            pet(0xef, c);
            if (b.rand() % 2 != 0) {
                SpawnPoint p;
                p.level = r.lvl;
                p.race = 0x106;
                p.people = 6;
                p.addFlags(GIANT | RESIDENT);
                p.home = r.P;
                b.place(p, b.cellSpot(c));
                p.roam = 0x10;
                p.requiredDefeated = (int32_t)cisterns.size();
                b.looks(p);
                b.gear(p);
                const int64_t id = b.add(p);
                last = id;
                b.added(spiderId).owner = id;
            }
        }
}

} // namespace

void roleThings(CreatureBuilder &b, int role) {
    (void)role; // the role is read from the plan, as 0x11a820 does
    if (!b.plan())
        return;
    Roles(b).run();
}

} // namespace worldgen
