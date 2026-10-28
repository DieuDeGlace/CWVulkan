#pragma once
// Lore, the tales lore objects tell, and the artifacts lore reveals, as Steam Cube World keeps
// them. Behaviour from docs/specs/lore-and-artifacts.md ("spec 2.3" and the like below name its
// sections); the realm draws and the token language are the conversations' (TalkGen.h,
// docs/specs/conversation-script.md).
//
// In short:
//   - Some 16-block cells hold a lore building (a pavilion, a crypt, a menhir, a sanctuary).
//     Examining a lore object in or next to one tells that cell's tale: a story of its realm,
//     a tombstone's inscription, or the tale of an artifact hidden somewhere in the realm.
//   - When the tale's last page is shown the cell counts as read: it joins the character's
//     used cells (where opened reward chests also go) and its realm's lore. Ten cells of a realm
//     are 100 % of its lore and reveal the realm's artifacts on the map.
//   - The lore by realm is not saved: it is rebuilt from the used cells.
//   - Every artifact carried raises the character's level by one and one of seven skills.
#include "worldgen/ZoneRecords.h"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace lore {

using Cell = std::array<int32_t, 2>; // a 16-block cell, or a 1024-block place where noted

// --- Lore buildings and their cells (spec 2.2, 2.3) -----------------------------------------

// The cell objects that are lore buildings: pavilion 13, crypt 37 (its tombstone), menhir 55,
// sanctuaries 69 and 70.
constexpr int LORE_BUILDINGS[5] = {13, 37, 55, 69, 70};
constexpr int TOMBSTONE_BUILDING = 37;
bool isLoreBuilding(Cell cell);
// Whether a static object offers "Examine" (spec 2.2): the lore objects 118..125, the keepers'
// brazier 126 (once it holds its flame) and the tombstone 130.
bool examinable(int objectType);
// The 16-block cell of a position in 1/65536 blocks.
Cell cellOf(int64_t x, int64_t y);
// The place (1024-block cell) a cell lies in, and that place's realm (spec 4.3).
Cell placeOf(Cell cell);
uint32_t realmOf(Cell cell);

// --- Examining and the tales (spec 2.3, 3) ------------------------------------------------------

// The cells looked at when an object in `cell` is examined, in order: the cell alone when it is
// a lore building, otherwise its 3 x 3 neighbourhood column by column.
std::vector<Cell> searchOrder(Cell cell);
// What examining an object in `cell` reads: the first lore building in the search order whose
// tale says something, and that tale. None when there is no lore building around.
struct Reading {
    Cell cell{};
    std::vector<std::string> tale;
};
std::optional<Reading> examine(Cell cell);

// The tale of a lore cell (spec 3): a script of tokens, the same for the cell every time.
std::vector<std::string> tale(Cell cell);
// Its three kinds, for tests: a story of the realm (spec 3.3), with the land's biome and seed;
// the tale of an artifact hidden at `place` (spec 3.4), `kind` the place's kind or its big
// structure's; a tombstone's inscription (spec 3.5).
std::vector<std::string> realmStory(uint32_t realm, uint32_t seed, int biome, int32_t landSeed);
std::vector<std::string> artifactTale(uint32_t realm, const worldgen::Item &item, Cell place, int kind, uint32_t seed);
std::vector<std::string> tombstoneTale(uint32_t realm, uint32_t seed);

// --- The character's lore (spec 4) ---------------------------------------------------------------

// The lore by realm: the lore cells read in each realm (the character's data +0x128).
using Book = std::map<uint32_t, std::set<Cell>>;
// Ten pieces are all of a realm's lore: they reveal its artifacts on the map (spec 5.4).
constexpr size_t PIECES_TO_REVEAL = 10;
// What a realm's lore reads as, in percent: ten a piece, not capped.
inline int percent(size_t pieces) { return 10 * (int)pieces; }
// The book rebuilt from the used cells (spec 4.4): every used cell that is a lore building,
// under its realm.
Book rebuild(const std::set<Cell> &usedCells);
// Reading a cell (spec 4.1): it joins the used cells, and, if it is a lore building not yet in
// its realm's lore, that lore.
struct Credit {
    bool newlyUsed = false;  // the cell was not a used cell before
    bool newPiece = false;   // a lore piece was added (the chat line and the sound)
    uint32_t realm = 0;
    size_t pieces = 0;       // the realm's lore after it
};
Credit credit(Cell cell, std::set<Cell> &usedCells, Book &book);

// --- Carried artifacts (spec 5.2) -------------------------------------------------------------------

constexpr int ARTIFACT_CATEGORY = 23;
// The seven skills an artifact can raise, by its modifier mod 7. The game reads swimming,
// diving, sailing and light radius; climbing, riding and gliding show on the character
// sheet only.
enum ArtifactSkill { CLIMBING, SWIMMING, DIVING, RIDING, GLIDING, SAILING, LIGHT_RADIUS, ARTIFACT_SKILLS };
// The character's level and skill counters from the stacks of its artifact tab (inventory
// tab 5): one level and one skill point a stack, whatever its count.
struct ArtifactProgress {
    int level = 1;
    std::array<int32_t, ARTIFACT_SKILLS> skills{};
};
ArtifactProgress artifactProgress(const std::vector<worldgen::Item> &stacks);

} // namespace lore
