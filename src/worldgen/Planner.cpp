// The region planner, written from docs/specs/worldgen-creatures.md §2 (steps numbered as there),
// conversation-script.md §5.5 (the encounter) and lore-and-artifacts.md §5.1 (artifacts).
#include "worldgen/Planner.h"

#include "worldgen/CellObjects.h"
#include "worldgen/Options.h"
#include "worldgen/Places.h"
#include "worldgen/StructureTables.h"
#include "worldgen/ZoneBuild.h"

#include <algorithm>
#include <list>
#include <mutex>
#include <set>
#include <tuple>

namespace worldgen {
namespace {

constexpr uint32_t COUNTER_SALT = 0x46d7c2;
constexpr int ARTIFACT_CATEGORY = 23;
constexpr int BOOK_CATEGORY = 24;
constexpr int32_t NO_ITEM_REGION_ITEM = 8;

int32_t evened(int32_t v) { return 2 * floorDiv(v, 1); }

// A place of the pool: its kind and cell (a big place's even corner), ordered as the game's set.
struct PoolPlace {
    int kind;
    int32_t x, y;
    bool operator<(const PoolPlace &o) const { return std::tie(kind, x, y) < std::tie(o.kind, o.x, o.y); }
    bool operator==(const PoolPlace &o) const { return kind == o.kind && x == o.x && y == o.y; }
};

bool isKind(int kind, std::initializer_list<int> kinds) { return std::find(kinds.begin(), kinds.end(), kind) != kinds.end(); }

class Planner {
public:
    Planner(int32_t rx, int32_t ry) : rx(rx), ry(ry) {
        plan.region = {rx, ry};
        c = random(cellKey(rx, ry) + COUNTER_SALT);
        culture = regionBiome(rx, ry);
        buildPool();
    }

    RegionPlan make() {
        fixedKinds();
        witch();
        demonPortal();
        artifacts();
        roleSix();
        roleSeven();
        gnomes();
        craftingBooks();
        if (culture != 1)
            pick(9, nonTown());
        for (int r : {10, 11, 13, 8, 12})
            pick(r, nonTown());
        monsters();
        everyoneElse();
        lists();
        return std::move(plan);
    }

private:
    int32_t rx, ry;
    RegionPlan plan;
    uint32_t c;
    int culture;
    std::set<PoolPlace> pool;

    // "Take X(c)": use it and move the counter on.
    uint32_t take() { return xorshift(c++); }

    void setRole(const PoolPlace &p, int r) { plan.roles[{p.x, p.y}] = r; }
    void setLevel(const PoolPlace &p, int l) { plan.levels[{p.x, p.y}] = l; }

    // §2.1: every place whose landmark lies in the region.
    void buildPool() {
        const int32_t x0 = floorDiv(wrapMul(rx, 8192) - 8192, PLACE_SHIFT), x1 = floorDiv(wrapMul(rx + 2, 8192), PLACE_SHIFT);
        const int32_t y0 = floorDiv(wrapMul(ry, 8192) - 8192, PLACE_SHIFT), y1 = floorDiv(wrapMul(ry + 2, 8192), PLACE_SHIFT);
        for (int32_t x = x0; x <= x1; x++)
            for (int32_t y = y0; y <= y1; y++) {
                if (x == 0 && y == 0)
                    continue;
                const bool big = bigCell(x, y);
                const int kind = placeKind(x, y, big);
                if (kind == 0)
                    continue;
                const int32_t px = big ? evened(x) : x, py = big ? evened(y) : y;
                const Int2 point = cellPoint(px, py, big);
                if (regionAt(point.x, point.y) == Int2{rx, ry})
                    pool.insert({kind, px, py});
            }
    }

    std::vector<PoolPlace> ofKinds(std::initializer_list<int> kinds) const {
        std::vector<PoolPlace> out;
        for (const PoolPlace &p : pool)
            if (isKind(p.kind, kinds))
                out.push_back(p);
        return out;
    }
    std::vector<PoolPlace> nonTown() const {
        std::vector<PoolPlace> out;
        for (const PoolPlace &p : pool)
            if (p.kind != 5)
                out.push_back(p);
        return out;
    }
    // "Pick r from L".
    bool pick(int r, const std::vector<PoolPlace> &list, int level = -1) {
        if (list.empty())
            return false;
        const PoolPlace p = list[take() % list.size()];
        pool.erase(p);
        setRole(p, r);
        if (level >= 0)
            setLevel(p, level);
        return true;
    }
    void giveArtifact(const PoolPlace &p) {
        pool.erase(p);
        setRole(p, role::ARTIFACT);
        setLevel(p, 4);
        plan.artifacts[{p.x, p.y}] = makeArtifact(p.x, p.y, c);
        c += 2;
    }

    // Step 1.
    void fixedKinds() {
        for (const PoolPlace &p : ofKinds({7, 16, 17, 24})) {
            pool.erase(p);
            switch (p.kind) {
            case 7: setRole(p, role::TOWER), setLevel(p, (int)(take() % 3 + 2)); break;
            case 16: setRole(p, role::BRAZIERS), setLevel(p, (int)(take() & 3) + 1); break;
            case 17: setRole(p, role::SPIDERS), setLevel(p, (int)(take() & 3) + 1); break;
            default: setRole(p, role::ARENA), setLevel(p, 4); break;
            }
        }
    }
    // Step 2.
    void witch() {
        const std::vector<PoolPlace> towns = ofKinds({5});
        if (towns.size() < 2)
            return;
        const std::vector<PoolPlace> huts = ofKinds({22});
        if (huts.empty())
            return;
        const PoolPlace p = huts[xorshift(c) % huts.size()];
        pool.erase(p);
        setRole(p, role::WITCH);
        setLevel(p, (int)(xorshift(c + 1) % (uint32_t)std::min<size_t>(towns.size(), 5)));
        const PoolPlace &t = towns[xorshift(c + 2) % towns.size()];
        plan.specialTown = {t.x, t.y};
        c += 3;
    }
    // Step 3.
    void demonPortal() {
        const std::vector<PoolPlace> portals = ofKinds({37});
        if (portals.empty())
            return;
        const PoolPlace p = portals[xorshift(c) % portals.size()];
        pool.erase(p);
        setRole(p, role::DEMON_PORTAL);
        setLevel(p, (int)(xorshift(c + 1) % 5));
        plan.marked = true;
        c += 2;
    }
    // Steps 4 and 5.
    void artifacts() {
        std::vector<PoolPlace> found;
        for (const PoolPlace &p : pool)
            if (p.kind == 8 || bigStructure(p.x, p.y, p.kind, bigCell(p.x, p.y)) != 0)
                found.push_back(p);
        for (const PoolPlace &p : found)
            giveArtifact(p);
        if (!found.empty())
            return;
        std::vector<PoolPlace> list = ofKinds({24});
        if (list.empty())
            list = ofKinds({18, 23, 28, 30, 31, 36});
        if (list.empty())
            list = ofKinds({7, 9, 12, 15, 18, 20, 23, 25, 28, 30, 31});
        if (list.empty())
            list = nonTown();
        if (list.empty())
            return;
        giveArtifact(list[take() % list.size()]);
    }
    // Step 6.
    void roleSix() {
        std::vector<PoolPlace> list = ofKinds({10, 19, 21, 29});
        pick(6, list.empty() ? nonTown() : list);
    }
    // Step 7: not in cultures 1, 5, 12 and 13.
    void roleSeven() {
        if (culture == 1 || culture == 5 || culture == 12 || culture == 13)
            return;
        std::vector<PoolPlace> list = ofKinds({13, 23});
        pick(7, list.empty() ? nonTown() : list);
    }
    // Step 8: four gnomes drawn from one list; a place drawn twice keeps the later level.
    void gnomes() {
        const std::vector<PoolPlace> list = nonTown();
        for (int b = 0; b < 4 && !list.empty(); b++) {
            const PoolPlace p = list[take() % list.size()];
            pool.erase(p);
            setRole(p, role::GNOME);
            setLevel(p, b);
        }
    }
    // Step 9: roles 2-5, the crafting books.
    void craftingBooks() {
        for (int b = 0; b < 4; b++) {
            const std::vector<PoolPlace> list = nonTown();
            if (list.size() < 8 && (take() & 1))
                continue;
            pick(b + 2, list, b);
        }
    }
    // Step 12.
    void monsters() {
        std::vector<PoolPlace> kept;
        for (const PoolPlace &p : pool)
            if (isKind(p.kind, {18, 23, 28, 30, 31, 36}) && (take() & 1))
                kept.push_back(p);
        for (const PoolPlace &p : kept) {
            pool.erase(p);
            setRole(p, role::MONSTER);
            setLevel(p, (int)(take() & 3) + 1);
        }
    }
    // Step 13 (step 14 never happens).
    void everyoneElse() {
        for (const PoolPlace &p : pool)
            if (p.kind != 5) {
                setRole(p, 15);
                setLevel(p, (int32_t)c % 4);
                c++;
            }
    }
    // Step 15 and conversation-script.md §1.5.
    void lists() {
        for (const auto &[cell, r] : plan.roles) {
            switch (r) {
            case 10: plan.itemBits |= 2, plan.items.push_back(16); break;
            case 11: plan.itemBits |= 4; break;
            case 12: plan.itemBits |= 1, plan.items.push_back(15); break;
            case 21: plan.itemBits |= 8, plan.items.push_back(14); break;
            default: break;
            }
            if (r != 0 && !isKind(r, {13, 14, 15, 17, 21, 22, 23}))
                plan.helpTargets.push_back(cell);
            if (r == role::ARTIFACT)
                plan.artifactPlaces.push_back(cell);
        }
    }
};

} // namespace

Item makeArtifact(int32_t x, int32_t y, uint32_t c) {
    Item it;
    it.category = ARTIFACT_CATEGORY;
    it.id = (int32_t)(xorshift(c) % 7);
    it.modifier = xorshift(c + 1);
    it.region = {x, y};
    it.realm = placeStyle(x, y);
    return it;
}

int RegionPlan::roleAt(int32_t x, int32_t y) const {
    const auto it = roles.find({x, y});
    return hookedRole(x, y, it == roles.end() ? 0 : it->second);
}

int RegionPlan::levelAt(int32_t x, int32_t y) const {
    const auto it = levels.find({x, y});
    return it == levels.end() ? 0 : it->second;
}

Item RegionPlan::roleTreasure(int32_t x, int32_t y) const {
    const int r = roleAt(x, y);
    Item it;
    if (r >= 2 && r <= 5) {
        it.category = BOOK_CATEGORY;
        it.id = 0xe + r - 2;
        it.region = region;
    } else if (r == role::ARTIFACT) {
        const auto a = artifacts.find({x, y});
        if (a != artifacts.end())
            it = a->second;
    }
    return it;
}

int32_t RegionPlan::regionItem(int32_t px, int32_t py) const {
    if (items.empty() || (random(cellKey(px, py) + 0x29fa4) & 3) == 0)
        return NO_ITEM_REGION_ITEM;
    return items[random(cellKey(px, py) + 0x89d85) % items.size()];
}

int32_t RegionPlan::doorItem(int32_t cx, int32_t cy) const {
    if (items.empty())
        return -1;
    return items[random(cellKey(cx, cy) + 0x89d85) % items.size()];
}

int RegionPlan::encounter(int32_t x, int32_t y, int32_t day) const {
    const bool big = bigCell(x, y);
    if (big)
        x = evened(x), y = evened(y);
    const int kind = placeKind(x, y, big);
    const int r = roleAt(x, y);
    if (r == 18)
        return 26;
    if (r == role::MONSTER)
        return 1;
    if (r == role::ARENA || kind == 24)
        return 27;
    const uint32_t key = (uint32_t)(x + 0x3d7) + 57u * (uint32_t)(y + 0x15948);
    if (r == role::GNOME) {
        std::vector<int> list;
        if (kind == 4 || kind == 9)
            list = {12, 13, 14};
        else if (kind == 22)
            list = {11};
        else if (kind == 37)
            list = {15, 16};
        else
            list = {9, 10, 11, 12, 13, 14, 15, 16};
        return list[random(key) % list.size()];
    }
    const bool treasure = (r >= 2 && r <= 5) || (r == role::ARTIFACT && artifacts.count({x, y}));
    if (bigStructure(x, y, kind, big) != 0)
        return 28;
    std::vector<int> list;
    switch (kind) {
    case 2: list = {8, 7}; break;
    case 4: case 9: list = {4, 5, 19, 12, 13, 14, 30, 31}; break;
    case 8: return 29;
    case 13: case 17: case 20: case 25: case 28: case 29: case 31: case 32: case 33:
        if (!big)
            list.push_back(2);
        list.insert(list.end(), {18, 19, 20, 21, 22, 23});
        break;
    case 22: list = {11}; break;
    case 24: list = {27}; break;
    case 36: list = {17}; break;
    case 37: list = {15, 16, 20, 3, 32}; break;
    default:
        if (!big)
            list = {3, 6};
        list.insert(list.end(), {18, 19, 20, 21, 22, 23, 0, 9, 10, 11, 12, 13, 14, 15, 16, 30, 31, 32});
        break;
    }
    uint32_t h = random(key);
    if (!treasure)
        h += (uint32_t)day;
    return list[xorshift(h) % list.size()];
}

std::shared_ptr<const RegionPlan> placePlan(int32_t px, int32_t py) {
    const bool big = bigCell(px, py);
    if (big)
        px = evened(px), py = evened(py);
    const Int2 point = cellPoint(px, py, big);
    const Int2 region = regionAt(point.x, point.y);
    return regionPlan(region.x, region.y);
}

Item placeArtifact(int32_t px, int32_t py) {
    if (bigCell(px, py))
        px = evened(px), py = evened(py);
    const std::shared_ptr<const RegionPlan> plan = placePlan(px, py);
    const auto it = plan->artifacts.find({px, py});
    return it == plan->artifacts.end() ? Item() : it->second;
}

RegionPlan makeRegionPlan(int32_t rx, int32_t ry) { return Planner(rx, ry).make(); }

std::shared_ptr<const RegionPlan> regionPlan(int32_t rx, int32_t ry) {
    // A few regions per thread: zones are made near each other. A plan made with the mods' place
    // kinds is kept apart from the game's own.
    struct Entry {
        Int2 region;
        bool original;
        std::shared_ptr<const RegionPlan> plan;
    };
    thread_local std::list<Entry> cache;
    thread_local uint32_t version = 0;
    if (version != optionsVersion())
        cache.clear(), version = optionsVersion();
    const bool original = generatingOriginal();
    for (auto it = cache.begin(); it != cache.end(); ++it)
        if (it->region == Int2{rx, ry} && it->original == original) {
            cache.splice(cache.begin(), cache, it);
            return cache.front().plan;
        }
    cache.push_front({{rx, ry}, original, std::make_shared<const RegionPlan>(makeRegionPlan(rx, ry))});
    if (cache.size() > 8)
        cache.pop_back();
    return cache.front().plan;
}

} // namespace worldgen
