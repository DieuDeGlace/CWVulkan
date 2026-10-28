#pragma once
// The game's own save files, characters.db and world.db (docs/specs/native-saves.md), read and
// written 1:1 so CubeVulkan keeps its saves in the same format as Steam Cube World.
//
// Both files are SQLite databases with one table, blobs(key TEXT PRIMARY KEY, value BLOB):
//   characters.db: "num" (how many characters), "slot" (the last one played) and one zlib
//                  stream per character under "0", "1", ... (the character blob, version 11);
//   world.db:      "state", uncompressed (the world state, version 44): the day, the time of
//                  day, the dead creatures, the rewards given and what changed in each zone.
//
// CubeVulkan keeps these files in its own Save folder beside the engine. The owner's real
// saves in the game's Save folder are only ever opened read-only, to copy them in
// (importGameSaves).
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct sqlite3;

namespace nativesaves {

constexpr int32_t CHARACTER_VERSION = 11; // what 0x149760 writes
constexpr int32_t WORLD_VERSION = 44;     // the only world state version 0x148e90 accepts
constexpr size_t ITEM_SIZE = 0xa0;
constexpr size_t BUFF_SIZE = 0x18;
constexpr size_t LOOKS_SIZE = 0x14;
constexpr size_t ZONE_RECORD_SIZE = 0xe0; // a ground item or a static object, kept whole
constexpr size_t BLOCK_CHANGE_SIZE = 28;
constexpr int EQUIPMENT_SLOTS = 11;
constexpr int INVENTORY_TABS = 6;

// A new world's clock (0x2b5f20): day 1 at 10:00.
constexpr int32_t NEW_WORLD_DAY = 1;
constexpr float NEW_WORLD_TIME_MS = 36000000.0f;
constexpr float DAY_MS = 86400000.0f;

using Item = std::array<uint8_t, ITEM_SIZE>;
using Cell = std::array<int32_t, 2>; // a pair (x first): a cell, a zone or a region
using Record = std::array<uint8_t, ZONE_RECORD_SIZE>;

// An item record's fields (drops-and-items.md §0.1).
namespace item {
constexpr size_t TYPE = 0x00, SUBTYPE = 0x04, MODIFIER = 0x08, REGION = 0x0c, RARITY = 0x14, FORMULA = 0x18,
                 MATERIAL = 0x1c, ATTRIBUTE_COUNT = 0x9d;
}
// Builds an item: type, subtype and material, the rest zero.
Item makeItem(int type, int subtype, int material = 0);

// The 20 bytes of appearance choices (§3.5). Kept as the bytes read, padding included, so a
// blob writes back exactly as it was read.
struct Looks {
    std::array<uint8_t, LOOKS_SIZE> bytes{};
    int32_t people() const;  // +0x00: 0 human, 1 elf, 2 dwarf, 3 orc, 4 goblin, 5 lizard, 6 undead, 7 frogman
    int sex() const;         // +0x04 bit 0: 0 male, 1 female
    int32_t face() const;    // +0x08
    int32_t haircut() const; // +0x0c
    std::array<uint8_t, 3> hairColour() const; // +0x10..+0x12: red, green, blue
    void set(int32_t people, int sex, int32_t face, int32_t haircut, std::array<uint8_t, 3> colour);
};
// The race id for a people choice and sex (§3.5); -1 for a choice outside 0..7.
int32_t raceOf(int32_t people, int sex);

// One inventory stack: how many and the item (0xa4 bytes in the file).
struct Stack {
    int32_t count = 0;
    Item item{};
};

// A buff entry (0x18 bytes, copied whole): type byte +0, strength +4, duration +8, source +0x10.
struct Buff {
    std::array<uint8_t, BUFF_SIZE> bytes{};
};

// A map star: a value not established, and the block it marks.
struct Star {
    int32_t unknown = 0;
    int32_t x = 0, y = 0, z = 0;
    bool operator==(const Star &) const = default;
};

// The character blob of version 11 (§3.3), field by field in file order. Fields whose meaning
// the spec leaves open keep their record offset in their name.
struct Character {
    int32_t version = CHARACTER_VERSION;
    std::array<int64_t, 3> position{};      // +0x10, world units (1/65536 block)
    std::array<float, 3> orientation{};     // +0x28; kept as read, the game zeroes it on load
    float health = 0;                        // +0x180
    int32_t level = 1;                       // +0x144
    int32_t experience = 0;                  // +0x148
    uint8_t classType = 1;                   // +0x14c: 1 warrior, 2 ranger, 3 mage, 4 rogue
    uint8_t specialization = 0;              // +0x14d
    float field9a4 = 1.0f;                   // +0x9a4
    float field9a8 = 0.0f;                   // +0x9a8
    std::array<Item, EQUIPMENT_SLOTS> equipment{}; // +0x278, 0xa0 apart
    std::string name;                        // +0x958
    Looks looks;                             // player data +0x00..+0x13
    std::vector<std::vector<Stack>> inventory = std::vector<std::vector<Stack>>(INVENTORY_TABS); // +0x9f0
    int32_t gold = 0;                        // +0xaac
    int32_t fieldAb0 = 0;                    // +0xab0
    std::vector<Item> recipes;               // player data +0x18
    std::set<Cell> discoveredZones;          // player data +0x38
    std::set<Cell> set48;                    // player data +0x48
    std::set<Cell> discoveredLands;          // player data +0x58
    std::map<Cell, int32_t> map88, map98;    // player data +0x88, +0x98
    std::set<Cell> usedCells;                // player data +0xa8: reward chests opened, lore read
    std::set<Cell> setB8;                    // player data +0xb8
    std::set<Cell> setC8;                    // player data +0xc8 (missions solved)
    std::set<Cell> finishedMissions;         // player data +0xd8
    std::map<Cell, float> missionTimers;     // player data +0xe8, ms
    int32_t worldSeed = 0;                   // player data +0x2c8
    std::string worldName;                   // player data +0x2d0
    std::vector<Cell> flightPoints;          // player data +0x28, blocks
    Cell homeCell{};                         // +0x1d0: the 16-block cell it started in
    std::vector<Buff> buffs;                 // +0x980
    std::map<Cell, int32_t> regionValues;    // player data +0xf8
    std::map<Cell, std::vector<Star>> stars; // player data +0x108, grouped by (x / 4096, y / 4096)
    std::vector<Cell> regions118;            // player data +0x118
    std::map<int64_t, std::vector<Item>> purchases; // player data +0x138: what was bought, by shop
};

// The star group key the reader makes (§3.3 item 27): truncation toward zero.
Cell starGroup(int32_t x, int32_t y);

// Reads a character blob (0x147800) into `out`, which should start as a default Character.
// Every field is read with a bounds check: one that does not fit keeps its default and so does
// everything after it, so a truncated blob loads as a valid character with defaults. A list
// stops at the end of the data. The negative gold quirk is applied (§3.6).
void readCharacter(const std::vector<uint8_t> &blob, Character &out);
// Writes a character blob of version 11 (0x149760), sets and maps in sorted order.
std::vector<uint8_t> writeCharacter(const Character &c);
// The recipe every load makes sure of (§3.7): {type 1, subtype 7}, appended when missing.
void ensureLoadRecipe(Character &c);

// --- world.db --------------------------------------------------------------------------------

// One block change (28 bytes): the block (x, y, z), the block as it was (6 bytes at +0x0c),
// as it is now (6 bytes at +0x12), 4 bytes not used.
struct BlockChange {
    std::array<uint8_t, BLOCK_CHANGE_SIZE> bytes{};
    std::array<int32_t, 3> block() const;
    std::array<uint8_t, 6> was() const;
    std::array<uint8_t, 6> now() const;
};

// A zone's stored state (§5.2).
struct ZoneState {
    static constexpr uint8_t GROUND_ITEMS_SAVED = 1, STATIC_OBJECTS_SAVED = 2;
    uint8_t flags = 0;
    std::vector<Record> groundItems, staticObjects;
    std::vector<BlockChange> blockChanges;
    std::map<Cell, int32_t> placeCounters; // 16-block place cell -> count
    int32_t deadTracked = 0;               // zone +0x490
};

// The "state" blob (§5.1).
struct WorldState {
    int32_t version = WORLD_VERSION;
    int32_t day = NEW_WORLD_DAY;
    float timeMs = NEW_WORLD_TIME_MS;
    std::set<int64_t> dead;    // world +0x20
    std::set<int64_t> rewards; // world +0x30: residents whose reward was given
    std::map<Cell, ZoneState> zones;
};

// Reads a world state (0x148e90): the version, day and time first; for any version but 44 the
// rest is left as `out` had it. Truncated data leaves the fields that do not fit as they were.
void readWorld(const std::vector<uint8_t> &blob, WorldState &out);
std::vector<uint8_t> writeWorld(const WorldState &w);

// A zone as the world keeps it, for recording and re-applying its state.
struct ZoneLists {
    std::vector<Record> groundItems, staticObjects;
    bool groundItemsChanged = false, staticObjectsChanged = false; // zone +0xf9, +0xfa
    std::vector<BlockChange> blockChanges;                         // zone +0xd8
    std::map<Cell, int32_t> placeCounters;                         // zone +0x4d8
    int32_t deadTracked = 0;                                       // zone +0x490
};
// Records a zone's state into the world's map as the zone is unloaded (0x2d3e20).
void recordZone(WorldState &world, Cell zone, const ZoneLists &lists);
// Applies a stored state on top of a freshly generated zone (0x2ae0c0); nothing in server mode.
// `setBlock` re-applies each block change's "now" block, in order.
void applyStoredZone(const WorldState &world, Cell zone, ZoneLists &lists, bool serverMode,
                     const std::function<void(const BlockChange &)> &setBlock);
// The world reset (0x2c34c0) on the world's side: the dead set and the zone states emptied, the
// rewards kept. (The player's side is resetCharacterForWorld.)
void resetWorld(WorldState &w);
// The world reset on the player's side: every item of type 0x16 taken from the inventory, and
// player data +0x88, +0xd8 and +0x138 emptied.
void resetCharacterForWorld(Character &c);

// --- The files -------------------------------------------------------------------------------

// zlib as the game uses it (§2.2): compress2 at the default level (6), and inflate to the end
// of the stream (a damaged stream gives what inflated before the damage).
std::vector<uint8_t> compress(const std::vector<uint8_t> &data);
std::vector<uint8_t> inflate(const std::vector<uint8_t> &data);

// One of the two databases: the blobs table and its four statements (§1.2).
class BlobDatabase {
  public:
    BlobDatabase() = default;
    BlobDatabase(const BlobDatabase &) = delete;
    BlobDatabase &operator=(const BlobDatabase &) = delete;
    ~BlobDatabase();
    // Opens read-write, creating the file and its table (0x67070). False with a reason.
    bool open(const std::filesystem::path &file, std::string *why = nullptr);
    // Opens read-only (SQLITE_OPEN_READONLY): nothing can be written through it.
    bool openReadOnly(const std::filesystem::path &file, std::string *why = nullptr);
    bool isOpen() const { return db != nullptr; }
    void close();
    bool read(const std::string &key, std::vector<uint8_t> &value) const;              // 0x670f0
    bool write(const std::string &key, const std::vector<uint8_t> &value);             // 0x672e0
    bool remove(const std::string &key);                                               // 0x66fc0
    bool rename(const std::string &from, const std::string &to);                       // 0x671f0
    std::vector<std::string> keys() const; // every key, for tests and the log
    bool readInt(const std::string &key, int32_t &value) const;
    bool writeInt(const std::string &key, int32_t value);
    sqlite3 *handle() const { return db; }

  private:
    sqlite3 *db = nullptr;
    bool readOnly = false;
};

// characters.db (§2, §4).
class CharacterStore {
  public:
    bool open(const std::filesystem::path &file, std::string *why = nullptr);
    // Read-only (a run that must not write): saving, creating and deleting then fail.
    bool openReadOnly(const std::filesystem::path &file, std::string *why = nullptr) { return db.openReadOnly(file, why); }
    bool isOpen() const { return db.isOpen(); }
    void close() { db.close(); }
    int count() const;               // "num"; 0 without it
    std::optional<int> slot() const; // "slot"
    bool setSlot(int index);
    // Loads character `index` (0x9bb20): inflated, read and given the load recipe. False when
    // there is no such row (`out` is then a default character).
    bool load(int index, Character &out) const;
    // Saves character `index` (§4.5): written, compressed and stored under its key.
    bool save(int index, const Character &c);
    // Adds a character (§4.3 steps 2 to 4): "num" grows, the character is saved under the old
    // count and "slot" names it. Returns its index, -1 on failure.
    int create(const Character &c);
    // Deletes character `index` (§4.4): "num" shrinks, its key is deleted and the keys above it
    // move down one. "slot" is left as it was.
    bool remove(int index);
    // CubeVulkan's own notes on a character that the game's format has no place for (its quests
    // and the places townspeople told it about), kept in the same table under
    // "cubevulkan:<index>" (the game reads only "num", "slot" and the numbered keys). They move
    // with their character when one before it is deleted.
    bool readNotes(int index, std::string &text) const;
    bool writeNotes(int index, const std::string &text);
    const BlobDatabase &database() const { return db; }

  private:
    BlobDatabase db;
};

// The steps of Create Character (§4.3 step 1) that change the character: level 1 and its home
// cell among the cells it used.
void prepareNewCharacter(Character &c);

// world.db (§5).
bool readWorldFile(const std::filesystem::path &file, WorldState &out, std::string *why = nullptr);
bool writeWorldFile(const std::filesystem::path &file, const WorldState &w, std::string *why = nullptr);

// Copies the game's characters.db and world.db from `gameSaveDir` into `ownSaveDir`: each source
// is opened read-only and copied whole (SQLite's backup), never changed. A file already in
// `ownSaveDir` is left alone (CubeVulkan's progress is not overwritten). Refuses when the two
// folders are the same. `report` says what was done, one line per file.
bool importGameSaves(const std::filesystem::path &gameSaveDir, const std::filesystem::path &ownSaveDir,
                     std::string &report);

} // namespace nativesaves
