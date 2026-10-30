#pragma once
// Characters and saves. Each character played in Steam Cube World is recorded
// by tools/ZoneExtractor into its own folder, Extract/characters/<name>-<class>/
// (snapshot.txt, anim-*.cwa, gear.txt); the older single folder
// Extract/character still counts as one character. CubeVulkan keeps its own
// progress for each of them in Saves/<folder>.sav next to the exe, and never
// writes to the game's save (characters.db / world.db).
#include <glm/glm.hpp>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <array>
#include <vector>

class Gear;

struct CharacterEntry {
    std::filesystem::path dir; // the recording folder
    std::string id;            // folder name, used for the save file
    std::string name;          // the character's name in the game
    int race = 0, classType = 0, level = 0, specialization = 0;
    std::string raceName;      // from the head sprite the game drew ("elf-head-m04" -> "elf")
    // Made in CubeVulkan's character creation (not recorded in the game): its own
    // look, worn on the recording of a character of its class (`dir`).
    bool created = false;
    std::string head, hair, hand, foot; // sprite names
    glm::u8vec3 hairColor{255};
    // A character of CubeVulkan's characters.db (the game's format, NativeProgress.h): its
    // index there, and its appearance (+0x78) as the looks routine makes it from its choices.
    bool native = false;
    int slot = -1;
    bool hasAppearance = false;
    std::array<uint8_t, 0xb0> appearance{};
    std::string describe() const; // "dieudeglace, level 1 elf rogue"
    // Progress is kept per character (name and class), shared by its specializations.
    std::string saveId() const;
};

// Every recorded character, newest snapshot first.
std::vector<CharacterEntry> listCharacters(const std::filesystem::path &gameDir);
// Characters made in CubeVulkan: the saves (in `saveDir` and the Steam Cloud bundle)
// that carry a "created" look, each on the newest recording of its class and
// specialization (or class) among `recorded`.
std::vector<CharacterEntry> listCreated(const std::filesystem::path &saveDir, const std::string &cloudBundle,
                                        const std::vector<CharacterEntry> &recorded);
// The character called `wanted` (name or folder, any case); the newest when
// `wanted` is empty. Null when there is none (or no such character).
const CharacterEntry *pickCharacter(const std::vector<CharacterEntry> &all, const std::string &wanted);
const char *className(int classType); // the game's class ids: 1 warrior, 2 ranger, 3 mage, 4 rogue
// The class's two specializations (subclasses): berserker/guardian, sniper/scout,
// fire/water, assassin/ninja.
const char *specializationName(int classType, int specialization);

// CubeVulkan's own progress for one character: where they are and what they wear.
struct SaveGame {
    struct Worn {
        int category = 0, id = 0, material = 0, rarity = 0;
        unsigned modifier = 0;
    };
    std::string world;         // the extracted world the position belongs to (seed_<n>)
    bool hasPosition = false;
    glm::vec3 position{0};
    float yaw = 0, heading = 0;
    float hpFraction = 1;
    bool hasGear = false;
    std::vector<Worn> worn; // one per Gear slot, in the game's slot order
    // Raw known recipes belong to characters.db, not the supplemental text notes.
    // Unset legacy progress retains the existing native list; an empty list is explicit.
    std::optional<std::vector<std::array<uint8_t, 0xa0>>> recipes;
    int specialization = -1; // optional current byte; old recordings remain unchanged
    int level = 0, xp = 0, gold = 0; // level 0: not saved yet (use the recorded level)
    float hour = -1; // time of day in the world (-1: not saved)
    int day = 0;
    // Items picked up in CubeVulkan, with the stats they were given.
    // What is carried besides gear (resources, food, pets, specials): the game's item and
    // how many.
    struct Bagged {
        int category = 0, id = 0, material = 0, rarity = 0, count = 0;
        unsigned modifier = 0;
        int region[2] = {0, 0};
    };
    std::vector<Bagged> bag;
    struct Found {
        int category = 0, id = 0, material = 0, rarity = 0, level = 0, slot = 0;
        unsigned modifier = 0;
        std::string sprite;
        float hp = 0, attack = 0, armor = 0, critical = 0;
        int region[2] = {0, 0}; // where it comes from (older saves: 0, 0)
        // A weapon's upgrade cubes as the item record keeps them (+0x1d.., count +0x9d); only
        // the game's save format holds them, the old text saves do not.
        std::array<uint8_t, 0x80> cubes{};
        uint8_t cubeCount = 0;
    };
    std::vector<Found> found;
    // Quests taken from NPCs: who asked (the creature id), what to defeat (the
    // dictionary id of the creature), where, how many, and how far it got.
    struct Quest {
        unsigned long long giver = 0;
        int state = 0; // 0 under way, 1 done (report back), 2 rewarded
        int need = 0, done = 0;
        std::string target, zone;
    };
    std::vector<Quest> quests;
    // Creatures killed in CubeVulkan in `world` (the game's creature ids). As in the
    // game, death is permanent and belongs to the world, not the character: every
    // character's save lists the kills it knew of, and loading joins them (worldKills).
    std::vector<int64_t> killed;
    // The 16-block cells this character used (the character's data +0xa8): the reward chests
    // it opened and the lore buildings it read. The lore by realm is rebuilt from them
    // (Lore.h), as the game does; it is not saved.
    std::vector<std::array<int32_t, 2>> usedCells;
    // The places (1024-block cells, a big place's rounded down to even) and lands (regions)
    // this character discovered (the character's discovered places, discoveredAreas): the map
    // names them.
    std::vector<std::array<int32_t, 2>> discoveredPlaces, discoveredAreas;
    // The spots the player marked on the map with the middle button (the character's map stars:
    // a star each), in blocks.
    std::vector<std::array<int32_t, 3>> treasures;
    // The flight points the player bought (the character's known flight points), in blocks.
    std::vector<std::array<int32_t, 2>> flightPoints;
    // The places whose mission this character finished (the character's data +0xd8) and solved
    // (+0xc8), by place cell (Missions.h).
    std::set<std::array<int32_t, 2>> finishedMissions, solvedMissions;
    // The missions marked lately (the character's data +0xe8, missionTimers): ms left of their "NEW"
    // on the map, 3 minutes from a townsperson marking it.
    std::map<std::array<int32_t, 2>, float> missionTimers;
    // Loot the game's format keeps with the character (loot-shops.md §1.1, loot-pickup.md §4):
    // the items bought from each shop by its id (player data +0x138), the blocks of items that
    // stay for others and were taken once (+0x88), and the per-region counters (+0xf8).
    std::map<int64_t, std::vector<std::array<uint8_t, 0xa0>>> purchases;
    std::set<std::array<int32_t, 3>> perPlayerTaken;
    std::map<std::array<int32_t, 2>, int32_t> regionCounters;
    // The items it took from where zones keep them (zone x, y and the item's index in the zone).
    std::vector<std::array<int32_t, 3>> takenItems;
    // The objects given the item they take (a brazier its flame): no longer active
    // (static object +0x28). By zone x, y and the object's index in the zone.
    std::vector<std::array<int32_t, 3>> usedObjects;
    // Where the character last died (the character's death position, game blocks), and the ms its
    // tombstone stays on the map (MapMarks.h: 5 minutes, or until the player is within 20 blocks).
    std::array<int64_t, 3> deathPosition{0, 0, 0};
    float tombstoneMs = 0;
    // Places townspeople told this character about (the game's mark_location): the
    // 1024-block cell, its name and what is there. The game keeps them with the character.
    struct Marked {
        int x = 0, y = 0;
        std::string place, item;
    };
    std::vector<Marked> marked;
    // A character made in CubeVulkan: its class, look and name (empty `name`: recorded).
    struct Created {
        int classType = 0, specialization = 0;
        std::string name, race, gender, head, hair, hand, foot;
        glm::u8vec3 hairColor{255};
    } created;

    long long savedAt = 0; // seconds since 1970, to pick the newer of two copies
    std::string text() const;
    bool parse(const std::string &text);
    bool load(const std::filesystem::path &file);
    bool save(const std::filesystem::path &file) const;
    void takeGear(const Gear &gear);
    // Steam Cloud holds one file for every character (Cube World's cloud space has
    // room for only one more file besides the game's two): sections that start
    // with "character <saveId>".
    static std::string fromBundle(const std::string &bundle, const std::string &id);
    static std::string intoBundle(const std::string &bundle, const std::string &id, const std::string &text);
    // Every kill recorded for `world` by any character in a cloud bundle.
    static std::set<int64_t> worldKills(const std::string &bundle, const std::string &world);
    // Puts the saved items back on (those still among the recorded items).
    void applyGear(Gear &gear) const;
  private:
    bool parseFields(const std::string &text);
};
