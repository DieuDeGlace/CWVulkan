#pragma once
// The creature tables (docs/specs/worldgen-creatures.md §3, worldgen-creatures-tables.md): the
// race levels, the spawn groups, each race's variants, the race pickers' lists and every race's
// fixed appearance. They are the game's data: CubeVulkan reads them at run time from the files
// the extraction tool writes from the owner's install (Extract/oracle: spawn-table.txt,
// races.txt, pickers.txt, looks.bin, looks-wild.txt). The short lists the specs give in full
// (tables T5, T6) are facts and live here as code.
#include "worldgen/Spawns.h"

#include <filesystem>
#include <string>
#include <vector>

namespace worldgen {

// A spawn group (tables T2): the races it is made of, how it comes and where it lives.
struct SpawnGroup {
    std::vector<int32_t> races;
    uint32_t kinds = 0;   // how the group comes: any of 1, 2, 4, 8, 0x10
    uint32_t climate = 0; // 0: anywhere
    uint32_t ground = 0;  // 0: never for animal draws
};

// The race pickers (§3.3, tables T4), by the game routine.
enum class Picker {
    People,         // 0x141480
    PeopleOrBeasts, // 0x1408e0, with a culture (-1 for none)
    Undead,         // 0x142190
    UndeadLeader,   // 0x142760
    Beasts,         // 0x13ffc0
    BeastLeader,    // 0x13feb0
    Lizards,        // 0x141390
    Golems,         // 0x142000
    TowerFolk,      // 0x13fa50
    Imps,           // 0x13fda0
    KindEightPeople // 0x140490
};

class CreatureTables {
public:
    // Reads the tables from an Extract/oracle folder. False (with the reason) when a required
    // file is missing or malformed.
    bool load(const std::filesystem::path &oracleDir, std::string *why = nullptr);
    bool loaded() const { return loaded_; }

    // The level of a wild creature of `race` (T1); 0 outside the table.
    int32_t raceLevel(int32_t race) const;
    const std::vector<SpawnGroup> &groups() const { return groups_; }

    // The variants of a race (T3): its sibling list, itself alone when it has none.
    const std::vector<int32_t> &siblings(int32_t race) const;
    // variant(r) (0x1420f0): one draw, even for a list of one.
    int32_t variant(int32_t race, Draw rand) const;

    // A picker's list for a culture argument (the people-or-beasts list depends on it; the other
    // pickers ignore it). Null when the extracted files do not hold it.
    const std::vector<int32_t> *pickerList(Picker p, int32_t culture = -1) const;
    // The picker (§3.3): list[rand % size], then variant (two draws). A picker whose list is not
    // known still draws twice and gives race 0.
    int32_t pick(Picker p, Draw rand, int32_t culture = -1) const;
    // Traveller 1 (0x141d70): three draws.
    int32_t traveller(Draw rand) const;

    // A race's fixed appearance (looks.bin), null for a race the files lack.
    const Appearance *baseAppearance(int32_t race) const;
    // The flags a race gains when its looks are made with `wild` set (looks-wild.txt).
    uint32_t wildFlags(int32_t race) const;

private:
    bool loaded_ = false;
    std::vector<int32_t> levels_;
    std::vector<SpawnGroup> groups_;
    std::vector<std::vector<int32_t>> siblings_; // by race + SIBLING_BASE
    std::vector<int32_t> special_;               // the list of selector -99991
    std::vector<std::vector<int32_t>> pickers_;  // by picker (culture -1)
    std::vector<std::vector<std::vector<int32_t>>> cultureLists_; // people-or-beasts by culture 0..15
    std::vector<Appearance> looks_;
    std::vector<bool> hasLooks_;
    std::vector<uint32_t> wild_;

    bool readSpawnTable(const std::filesystem::path &file, std::string *why);
    bool readSiblings(const std::filesystem::path &file, std::string *why);
    bool readPickers(const std::filesystem::path &file, std::string *why);
    bool readLooks(const std::filesystem::path &dir, std::string *why);
};

// Tables T5 and the marking pass's exception (0x65750's categories 3 and 6).
bool carriesConsumables(int32_t race);
bool markingExempt(int32_t race);

// The castle and dungeon race list of a big structure (T6): 8 for a dungeon, 0x29-0x2f castles,
// anything else the default list.
const std::vector<int32_t> &castleRaces(int structure);

// The process's tables, read once from the game folder's Extract/oracle by the caller
// (worldgen::loadCreatureTables, Generator.h; tests may load tables of their own).
CreatureTables &sharedCreatureTables();

} // namespace worldgen
