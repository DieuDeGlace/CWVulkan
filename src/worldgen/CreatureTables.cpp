// The creature tables, read from the extracted files (docs/specs/worldgen-creatures.md §3, §8.3
// and worldgen-creatures-tables.md).
#include "worldgen/CreatureTables.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

namespace worldgen {
namespace {

constexpr int32_t FIRST_SELECTOR = -200, LAST_SELECTOR = 400; // races.txt's range
constexpr int32_t SPECIAL_SELECTOR = -99991;                   // the one selector outside it
constexpr int32_t RACE_COUNT = 321;                            // looks.bin's races 0..320
constexpr int32_t LEVEL_COUNT = 306;                           // T1
constexpr int CULTURES = 16;

bool fail(std::string *why, const std::string &text) {
    if (why)
        *why = text;
    return false;
}

// The extraction tool's scripted rand(): xorshift32 of the seed (a zero seed starts from
// 0x9e3779b9), 15 bits (§8.4).
struct ScriptedRand {
    uint32_t state;
    explicit ScriptedRand(uint32_t seed) : state(seed ? seed : 0x9e3779b9u) {}
    int next() {
        state ^= state << 13, state ^= state >> 17, state ^= state << 5;
        return (int)(state & 0x7fff);
    }
};

// The game routine of each picker, as pickers.txt names it.
struct PickerName {
    Picker picker;
    uint32_t rva;
};
constexpr PickerName PICKER_RVAS[] = {
    {Picker::People, 0x141480},     {Picker::PeopleOrBeasts, 0x1408e0}, {Picker::Undead, 0x142190},
    {Picker::UndeadLeader, 0x142760}, {Picker::Beasts, 0x13ffc0},       {Picker::BeastLeader, 0x13feb0},
    {Picker::Lizards, 0x141390},    {Picker::Golems, 0x142000},         {Picker::TowerFolk, 0x13fa50},
    {Picker::Imps, 0x13fda0},       {Picker::KindEightPeople, 0x140490},
};
constexpr int PICKERS = (int)(sizeof PICKER_RVAS / sizeof PICKER_RVAS[0]);

} // namespace

// --- Reading ---------------------------------------------------------------------------------

bool CreatureTables::readSpawnTable(const std::filesystem::path &file, std::string *why) {
    std::ifstream in(file);
    if (!in)
        return fail(why, "no " + file.string());
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream l(line);
        std::string first;
        if (!(l >> first))
            continue;
        if (first == "values") {
            for (int32_t v; l >> v;)
                levels_.push_back(v);
            continue;
        }
        SpawnGroup g;
        if (!(l >> g.kinds >> g.climate >> g.ground))
            return fail(why, "bad spawn group line in " + file.string());
        for (int32_t r; l >> r;)
            g.races.push_back(r);
        groups_.push_back(std::move(g));
    }
    if (levels_.size() != (size_t)LEVEL_COUNT || groups_.empty())
        return fail(why, "incomplete " + file.string());
    return true;
}

bool CreatureTables::readSiblings(const std::filesystem::path &file, std::string *why) {
    std::ifstream in(file);
    if (!in)
        return fail(why, "no " + file.string());
    siblings_.assign((size_t)(LAST_SELECTOR - FIRST_SELECTOR + 1), {});
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream l(line);
        int32_t selector;
        if (!(l >> selector))
            continue;
        std::vector<int32_t> list;
        for (int32_t r; l >> r;)
            list.push_back(r);
        if (selector == SPECIAL_SELECTOR)
            special_ = std::move(list);
        else if (selector >= FIRST_SELECTOR && selector <= LAST_SELECTOR)
            siblings_[(size_t)(selector - FIRST_SELECTOR)] = std::move(list);
    }
    return true;
}

// pickers.txt holds each picker run on scripted rand() (`<rva hex> <seed> <draws> <race>`, and
// optionally the culture argument, -1 when absent). Each run drew the list index first and the
// variant second, so a list of n entries explains every run when, for each index i, some race's
// variant gives each run's result with that run's second draw. The shortest such list is the
// picker's (a longer one would repeat it).
bool CreatureTables::readPickers(const std::filesystem::path &file, std::string *why) {
    std::ifstream in(file);
    if (!in)
        return fail(why, "no " + file.string());
    struct Run {
        int first, second, result;
    };
    std::map<std::pair<uint32_t, int32_t>, std::vector<Run>> runs; // by routine and culture
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream l(line);
        std::string rva;
        uint32_t seed = 0, draws = 0;
        int32_t race = 0, culture = -1;
        if (!(l >> rva >> seed >> draws >> race) || draws < 2)
            continue;
        l >> culture;
        ScriptedRand r(seed);
        const int a = r.next(), b = r.next();
        runs[{(uint32_t)std::stoul(rva, nullptr, 16), culture}].push_back({a, b, race});
    }
    // The races (selectors) whose variant gives `result` with draw `d`.
    std::map<int32_t, std::vector<int32_t>> holders; // a race -> the selectors whose lists hold it
    auto forEachSelector = [&](auto &&f) {
        for (int32_t s = FIRST_SELECTOR; s <= LAST_SELECTOR; s++)
            f(s, siblings(s));
        if (!special_.empty())
            f(SPECIAL_SELECTOR, special_);
    };
    forEachSelector([&](int32_t s, const std::vector<int32_t> &list) {
        for (int32_t r : list)
            if (holders[r].empty() || holders[r].back() != s)
                holders[r].push_back(s);
    });
    auto candidates = [&](const Run &run) {
        std::vector<int32_t> out;
        for (int32_t s : holders[run.result]) {
            const std::vector<int32_t> &list = s == SPECIAL_SELECTOR ? special_ : siblings(s);
            if (list[(size_t)run.second % list.size()] == run.result)
                out.push_back(s);
        }
        return out;
    };
    auto recover = [&](const std::vector<Run> &list) {
        std::vector<std::vector<int32_t>> cands;
        for (const Run &run : list)
            cands.push_back(candidates(run));
        for (size_t n = 1; n <= 64; n++) {
            std::vector<std::vector<int32_t>> slots(n);
            std::vector<bool> seen(n);
            bool ok = true;
            for (size_t k = 0; k < list.size() && ok; k++) {
                const size_t i = (size_t)list[k].first % n;
                if (!seen[i]) {
                    slots[i] = cands[k], seen[i] = true;
                } else {
                    std::vector<int32_t> both;
                    std::set_intersection(slots[i].begin(), slots[i].end(), cands[k].begin(), cands[k].end(), std::back_inserter(both));
                    slots[i] = std::move(both);
                }
                ok = !slots[i].empty();
            }
            if (!ok || std::find(seen.begin(), seen.end(), false) != seen.end())
                continue;
            std::vector<int32_t> picked;
            for (auto &s : slots)
                picked.push_back(s.front());
            return picked;
        }
        return std::vector<int32_t>{};
    };
    pickers_.assign(PICKERS, {});
    cultureLists_.assign(PICKERS, std::vector<std::vector<int32_t>>(CULTURES));
    for (auto &[key, list] : runs)
        for (int p = 0; p < PICKERS; p++)
            if (PICKER_RVAS[p].rva == key.first) {
                std::vector<int32_t> recovered = recover(list);
                if (key.second < 0)
                    pickers_[(size_t)p] = std::move(recovered);
                else if (key.second < CULTURES)
                    cultureLists_[(size_t)p][(size_t)key.second] = std::move(recovered);
            }
    return true;
}

bool CreatureTables::readLooks(const std::filesystem::path &dir, std::string *why) {
    std::ifstream in(dir / "looks.bin", std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    if (b.size() < 4 || std::memcmp(b.data(), "CWL1", 4) != 0)
        return fail(why, "no looks.bin in " + dir.string());
    looks_.assign(RACE_COUNT, {});
    hasLooks_.assign(RACE_COUNT, false);
    constexpr size_t RECORD = 16 + Appearance::SIZE;
    for (size_t at = 4; at + RECORD <= b.size(); at += RECORD) {
        int32_t race;
        std::memcpy(&race, &b[at], 4);
        if (race < 0 || race >= RACE_COUNT || hasLooks_[(size_t)race])
            continue;
        std::memcpy(looks_[(size_t)race].bytes.data(), &b[at + 16], Appearance::SIZE);
        hasLooks_[(size_t)race] = true;
    }
    wild_.assign(RACE_COUNT, 0);
    std::ifstream w(dir / "looks-wild.txt");
    if (!w)
        return fail(why, "no looks-wild.txt in " + dir.string());
    for (int32_t race, draws; w >> race >> draws;) {
        uint32_t flags;
        w >> flags;
        if (race >= 0 && race < RACE_COUNT && hasLooks_[(size_t)race])
            wild_[(size_t)race] = flags & ~looks_[(size_t)race].flags();
    }
    return true;
}

bool CreatureTables::load(const std::filesystem::path &dir, std::string *why) {
    *this = CreatureTables();
    if (!readSpawnTable(dir / "spawn-table.txt", why) || !readSiblings(dir / "races.txt", why) || !readPickers(dir / "pickers.txt", why) ||
        !readLooks(dir, why))
        return false;
    loaded_ = true;
    return true;
}

// --- Lookups ---------------------------------------------------------------------------------

int32_t CreatureTables::raceLevel(int32_t race) const {
    return race >= 0 && (size_t)race < levels_.size() ? levels_[(size_t)race] : 0;
}

const std::vector<int32_t> &CreatureTables::siblings(int32_t race) const {
    thread_local std::vector<int32_t> alone(1);
    if (race == SPECIAL_SELECTOR && !special_.empty())
        return special_;
    if (race >= FIRST_SELECTOR && race <= LAST_SELECTOR && !siblings_.empty()) {
        const std::vector<int32_t> &list = siblings_[(size_t)(race - FIRST_SELECTOR)];
        if (!list.empty())
            return list;
    }
    alone[0] = race;
    return alone;
}

int32_t CreatureTables::variant(int32_t race, Draw rand) const {
    const std::vector<int32_t> &list = siblings(race);
    return list[(size_t)(rand() % (int)list.size())];
}

const std::vector<int32_t> *CreatureTables::pickerList(Picker p, int32_t culture) const {
    const size_t i = (size_t)p;
    if (i >= pickers_.size())
        return nullptr;
    // Only people-or-beasts builds a different list for some cultures (tables T4); a culture's own
    // list is used when the files hold it.
    if (p == Picker::PeopleOrBeasts && culture >= 0 && culture < CULTURES && !cultureLists_[i][(size_t)culture].empty())
        return &cultureLists_[i][(size_t)culture];
    if (p == Picker::PeopleOrBeasts && culture >= 0) {
        // The cultures whose list differs from the culture-less one (tables T4).
        const bool differs = culture == 5 || culture == 6 || culture == 9 || culture == 10 || culture == 11 || culture == 12;
        if (differs)
            return nullptr;
    }
    return pickers_[i].empty() ? nullptr : &pickers_[i];
}

int32_t CreatureTables::pick(Picker p, Draw rand, int32_t culture) const {
    const std::vector<int32_t> *list = pickerList(p, culture);
    const int first = rand();
    if (!list) {
        rand(); // the variant's draw
        return 0;
    }
    return variant((*list)[(size_t)(first % (int)list->size())], rand);
}

int32_t CreatureTables::traveller(Draw rand) const {
    constexpr int32_t TRAVELLER = 2, SECOND_TRAVELLER = 9;
    const bool both = rand() % 4 == 0;
    const int count = both ? 2 : 1;
    const int32_t race = rand() % count == 0 ? TRAVELLER : SECOND_TRAVELLER;
    return variant(race, rand);
}

const Appearance *CreatureTables::baseAppearance(int32_t race) const {
    if (race < 0 || (size_t)race >= hasLooks_.size() || !hasLooks_[(size_t)race])
        return nullptr;
    return &looks_[(size_t)race];
}

uint32_t CreatureTables::wildFlags(int32_t race) const {
    return race >= 0 && (size_t)race < wild_.size() ? wild_[(size_t)race] : 0;
}

// --- Facts (tables T5, T6) -------------------------------------------------------------------

bool carriesConsumables(int32_t race) {
    if (race >= 0 && race <= 16)
        return true;
    static constexpr int32_t RACES[] = {18,  43,  44,  45,  46,  48,  49,  51,  52,  54,  76,  77,  78,  79,  80,  81,  83,  84,
                                        95,  148, 149, 167, 191, 199, 202, 203, 208, 216, 227, 228, 230, 236, 237, 252, 253, 254,
                                        256, 259, 260, 261, 296, 304, 305};
    return std::find(std::begin(RACES), std::end(RACES), race) != std::end(RACES);
}

bool markingExempt(int32_t race) {
    static constexpr int32_t RACES[] = {73, 82, 85, 110, 225, 226, 229};
    return std::find(std::begin(RACES), std::end(RACES), race) != std::end(RACES);
}

const std::vector<int32_t> &castleRaces(int structure) {
    static const std::vector<int32_t> DUNGEON = {0,   2,   9,   4,   11,  15,  17,  165, 85,  43,  45, 167,
                                                 230, 225, 224, 217, 59,  40,  229, 227, 226, 269, 94};
    static const std::vector<int32_t> S29 = {75, 37, 96, 17, 173, 51, 229, 73};
    static const std::vector<int32_t> S2A = {224, 217, 9, 59, 96, 94, 17, 47, 46, 225};
    static const std::vector<int32_t> S2B = {105, 102, 104, 293, 60, 53, 151, 295, 294, 291, 290, 287, 70, 69, 54};
    static const std::vector<int32_t> S2C = {37, 59, 149, 202, 106, 107, 47};
    static const std::vector<int32_t> S2D = {96, 17, 77, 78, 81, 83, 72, 103, 75, 42, 7};
    static const std::vector<int32_t> S2E = {96, 17, 77, 78, 81, 83, 68, 62, 75, 172, 60, 13};
    static const std::vector<int32_t> S2F = {85, 225, 226, 227, 73, 229, 82, 174, 199};
    static const std::vector<int32_t> OTHER = {96, 17, 165, 97, 230, 173, 269, 296, 41, 15};
    switch (structure) {
    case 8: return DUNGEON;
    case 0x29: return S29;
    case 0x2a: return S2A;
    case 0x2b: return S2B;
    case 0x2c: return S2C;
    case 0x2d: return S2D;
    case 0x2e: return S2E;
    case 0x2f: return S2F;
    default: return OTHER;
    }
}

CreatureTables &sharedCreatureTables() {
    static CreatureTables tables;
    return tables;
}

} // namespace worldgen
