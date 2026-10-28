#pragma once
// CubeVulkan's progress kept in the game's own save format (NativeSaves.h): a character of
// characters.db becomes the progress gameplay works with (SaveGame) and back, its look becomes
// the appearance and sprites the engine draws, and the old text saves (Saves/*.sav and the Steam
// Cloud bundle) are moved into characters.db once.
#include "NativeSaves.h"
#include "Saves.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

class Gear;

namespace nativeprogress {

// Where CubeVulkan's local blocks sit in the game's world: the local origin in world units
// (ExtractedWorld::originDotsX/Y; 0 without an extracted world). A position in the save is the
// creature record's, a block above the feet CubeVulkan keeps (SpawnedCreatures::NATIVE_FEET).
struct Frame {
    int64_t originX = 0, originY = 0;
};
glm::vec3 localFeet(const std::array<int64_t, 3> &position, const Frame &frame);
std::array<int64_t, 3> gamePosition(glm::vec3 feet, const Frame &frame);

// The game's model list (Extract/sprites.txt: id, file, size), for the names of the models a
// look picks.
class SpriteTable {
  public:
    bool load(const std::filesystem::path &extractDir);
    std::string name(int id) const; // without sprites/ and .cub; "" when unknown
    bool has(const std::string &name) const;
    bool loaded() const { return !names.empty(); }

  private:
    std::vector<std::string> names;
    std::map<std::string, int> ids;
};

// The people choices (§3.5 of native-saves.md) by the word the sprites use.
const char *peopleWord(int32_t people); // "human", "elf", ... ("" outside 0..7)
// Looks from a recorded character's snapshot (its race, hair colour and head and hair sprites).
bool looksFromSnapshot(const std::filesystem::path &snapshot, nativesaves::Looks &out);
// Looks from sprite names ("elf-head-f03", "elf-hair-f02"): the sprite's number less one.
bool looksFromSprites(const std::string &race, const std::string &gender, const std::string &head,
                      const std::string &hair, glm::u8vec3 colour, nativesaves::Looks &out);
// The appearance the looks routine (0x53e80) makes for a player's looks record: the race's fixed
// data with the chosen head, haircut and hair colour. False without the creature tables.
bool appearanceOf(const nativesaves::Looks &looks, std::array<uint8_t, 0xb0> &out);

// The list entry of character `slot` of characters.db: its name, class, level and look, moving
// with the newest recording of its class and specialization (or class) among `recorded`.
// False when no recording of its class exists.
bool entryFor(const nativesaves::Character &c, int slot, const std::vector<CharacterEntry> &recorded,
              const SpriteTable &sprites, CharacterEntry &out);

// The race word the character screen prints for a race id (frontend-characters.md §2.2).
const char *raceWord(int32_t race);

// The character's maximum health by the game's rules (0x5fac0) with what it wears.
float maxHealth(const nativesaves::Character &c);

// The progress gameplay works with, from a character and its notes (the parts of CubeVulkan's
// progress the game's format has no place for). `itemSprite` names an item's model.
SaveGame toProgress(const nativesaves::Character &c, const std::string &notes, const Frame &frame,
                    const std::function<std::string(const nativesaves::Item &)> &itemSprite);
// Puts the progress back into the character (keeping everything CubeVulkan does not model) and
// gives the notes to keep beside it. `gear`: what is worn and carried now (none: as saved).
void fromProgress(const SaveGame &s, const Gear *gear, float health, const Frame &frame, nativesaves::Character &c,
                  std::string &notes);
// A game item from CubeVulkan's item fields, and back.
nativesaves::Item gameItem(int category, int id, int material, int rarity, uint32_t modifier, const int region[2],
                           int formula = 0);

// Moves CubeVulkan's old text saves into `store` once: each character's newer copy (local
// `saveDir`/*.sav or the cloud bundle), as a recorded character of `recorded` or one made in the
// old character creation. Returns how many were added; nothing when the store already holds
// characters. `log` gets one line per character; the creatures they killed join `worldDead`.
int migrateOldSaves(nativesaves::CharacterStore &store, const std::filesystem::path &saveDir, const std::string &cloudBundle,
                    const std::vector<CharacterEntry> &recorded, const std::string &worldName, const Frame &frame,
                    const std::function<void(const std::string &)> &log, std::set<int64_t> *worldDead = nullptr);

// CubeVulkan's Save folder for a session: characters.db and world.db in the game's format.
class ProgressFile {
  public:
    // Opens `folder` (created when missing). With `importFrom` set, the game's two files are first
    // copied in, read-only, if CubeVulkan has none of its own yet. Refuses a folder that is the
    // game's own Save folder (`gameSave`): the owner's real saves are never written. Not
    // `writable` (a capture, --no-save): nothing is created, imported or written.
    bool open(const std::filesystem::path &folder, const std::filesystem::path &gameSave,
              const std::filesystem::path &importFrom, bool writable, const std::function<void(const std::string &)> &log);
    bool isOpen() const { return store_.isOpen(); }
    nativesaves::CharacterStore &store() { return store_; }
    nativesaves::WorldState &world() { return world_; }
    const std::filesystem::path &folder() const { return folder_; }
    // Every recorded character and every character of characters.db that can be played.
    std::vector<CharacterEntry> characters(const std::vector<CharacterEntry> &recorded, const SpriteTable &sprites);
    // The character this session plays: a character of characters.db, or a recorded one (whose
    // progress is the character of characters.db with its name and class, made at its first
    // save when there is none).
    void play(const CharacterEntry &entry);
    int slot() const { return slot_; }
    // Its progress, with the world's day, time and dead (false: nothing saved yet).
    bool load(SaveGame &out, const Frame &frame, const std::string &worldName,
              const std::function<std::string(const nativesaves::Item &)> &itemSprite);
    // Writes the character (with `gear` and its health) and the world (day, time of day in hours,
    // the dead). False when either write failed.
    // With `world` false (the player is a guest in a friend's world) only the character is written:
    // the world it plays in is not this machine's.
    bool save(const SaveGame &s, const Gear *gear, float health, const Frame &frame, bool world = true);
    // Writes the world alone (day, time of day in ms, the dead): a dedicated server, which has no
    // character.
    bool saveWorld(int32_t day, float timeMs, const std::set<int64_t> &dead);

  private:
    std::filesystem::path folder_;
    nativesaves::CharacterStore store_;
    nativesaves::WorldState world_;
    CharacterEntry playing_;
    int slot_ = -1;
};

} // namespace nativeprogress
