#pragma once
// Items between the ground and the inventory (docs/specs/loot-pickup.md):
//
//   * a ground item's update (0x2e3510): wall items snap against a wall, the timer at +0xcc
//     counts down, and the pick-up delay at +0xc8 runs out with a landing sound;
//   * how it is drawn while the delay runs (0xd3170): a five-block hop in place, tumbling;
//   * which items are picked up automatically: coins and type 14 within 4 blocks once their
//     delay has ended (nothing ever flies to the player);
//   * receiving a pick-up notice (0xa5a50): per-player items taken once (player data +0x88),
//     the per-region counter (+0xf8), money, the inventory, the used cell of type 0x1e, the
//     receipt dialog and holding the item up;
//   * items given or taken by a dialog page (0xb40d0) and the client side of taming (0xa0750).
// Records are game layout: the ground item (0xe0 bytes, drops-and-items.md §0.4), items (0xa0
// bytes), creatures and damage numbers (0x48 bytes).
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace grounditems {

using Item = std::array<uint8_t, 0xa0>;

// The ground item record (drops-and-items.md §0.4).
namespace ground {
constexpr size_t ITEM = 0x00, POSITION = 0xa0, GROUND_Z = 0xb0, ANGLE = 0xc0, FLAGS = 0xc4, DELAY = 0xc8, TIMER = 0xcc,
                 DAY = 0xd0, COUNT = 0xd4, OWNER = 0xd8;
constexpr uint8_t STAYS_FOR_OTHERS = 2; // flag bit 1: no automatic pick-up, and it stays for others
} // namespace ground

// Item types this part tests.
namespace type {
constexpr int COIN = 0x0c, TYPE_0E = 0x0e, WALL_ITEM = 0x11, SPECIAL_17 = 0x17, VEHICLE = 0x18, TYPE_1D = 0x1d, TREASURE = 0x1e,
              GOLD_BAG = 0x1f, PET = 0x14, PET_FOOD = 0x15;
}
constexpr int COIN_MATERIAL = 11;   // coins (type 12, material 11) are the gold counter
constexpr int REGION_COIN_MATERIAL = 13; // type 12 material 13 counts per region (+0xf8)

constexpr int64_t BLOCK = 65536; // positions are in 1/65536 of a block

// --- The ground item update (§1) --------------------------------------------------------------

// Whether the block at (x, y, z) is solid (block type with bit 1 masked off is not 0).
using SolidAt = std::function<bool(int32_t x, int32_t y, int32_t z)>;
// §1 step 1, for a wall item (type 0x11): tested against the four neighbours in order (y − 1,
// y + 1, x − 1, x + 1) at its ground height; each solid one moves it 0.2 block from that wall
// and turns it to face away. Later tests override earlier ones.
void snapToWall(int64_t &x, int64_t &y, float &angle, int64_t groundZ, const SolidAt &solid);

// §1 steps 2–3: the timer counts down to 0 and the delay runs out. True on the update the delay
// reaches 0: the caller plays the landing sound (soundrules::itemLanded) unless it is a server.
bool countDown(float &delayMs, int32_t &timerMs, float dtMs);

// --- Drawing (§2) -------------------------------------------------------------------------

constexpr float HOP_HALF_TIME = 250.0f; // ms
constexpr float HOP_HEIGHT = 5.0f;      // blocks
constexpr float TUMBLE_PER_MS = 0.7f;   // degrees per ms of delay left
// How far above its ground point an item is drawn with `delayMs` of its delay left: a parabola
// 0 at 500 ms, 5 blocks at 250 ms and 0 at the end. Delays above 500 ms (drops start at
// 500..799) are below the ground point (open question 1).
float hopHeight(float delayMs);
// How far it is turned while it hops, in degrees.
float tumble(float delayMs);

// --- Picking up (§3) ----------------------------------------------------------------------

constexpr float AUTO_PICK_UP_RANGE = 4.0f; // blocks
// Whether a ground item is taken without the use key: coins and type 14 without flag bit 1,
// within 4 blocks, once the delay is exactly 0.
bool picksUpItself(int itemType, uint8_t flags, float delayMs, float distance2);

// --- Receiving a pick-up (§4) -------------------------------------------------------------

// The money value of an item (0x109070, use-prompt.md §3.7.3): only a gold bag (type 0x1f) has
// one: ((h mod 100000) / 100000 + rarity + 1) × 10, h a xorshift of its modifier + 0xcc72f.
int moneyValue(const Item &item);

// What the local player keeps about picked-up items (player data).
struct Keeper {
    std::set<std::array<int32_t, 3>> perPlayerTaken;           // +0x88: blocks of items that stay for others
    std::map<std::array<int32_t, 2>, int32_t> regionCounters;  // +0xf8: type 12 material 13, by region
};

// Where a received item goes (§4 step 3.2).
enum class Into { Nothing, RegionCounter, Gold, Inventory };

struct Receipt {
    Into into = Into::Nothing;
    int amount = 0;                      // the counter's gain, the gold, or how many to add
    bool markCell = false;               // a type-0x1e notice: its 16-block cell becomes used
    std::array<int32_t, 2> cell{};
    // The receipt dialog (empty: none): define_item and the line, plus for a vehicle new_page
    // and define_land (the caller fills the land); shown with a seed of one rand mod 10000.
    std::vector<std::string> dialog;
    int holdUpAbility = 0;               // 0x78, 0x79 or 0x7c when the player may hold it up; 0: none
    bool announce = true;                // false only for a per-player item taken before
};

// §4 step 3 for the local player: `noticeItem` as the notice carries it, `adapted` after the class
// adaptation (0x10a720), `count` and `flags` the notice's, `position` its position (1/65536
// block), `region` the player's current region, `playerFlags` its +0x5c, `abilityFree` whether its
// ability is 0 or over.
Receipt receive(Keeper &keeper, const Item &noticeItem, const Item &adapted, int count, uint8_t flags, const int64_t position[3],
                const int32_t region[2], uint32_t playerFlags, bool abilityFree);

// --- Items given without a ground item (§5) ---------------------------------------------------

// One inventory stack (count and item), as the inventory keeps them.
struct Stack {
    int count = 0;
    Item item{};
};

// A dialog page's give (§5.1): whether it gives (the count is not 0 and the player may carry it),
// and whether it also shows "you received the item" and holds it up (one vehicle).
struct PageGift {
    bool gives = false;
    bool receiptDialog = false;
};
PageGift pageGives(int count, const Item &item, bool mayCarry);
// A dialog page's take (0xa2210, §5.1): coins from the gold counter (not below 0); anything else
// from the first equal stacks in order, emptying them as needed.
void pageTakes(std::vector<Stack *> &stacksInOrder, int32_t &gold, const Item &item, int count);
// The three script operations of the page builder (0x168120): 8 gives the last defined item,
// 9 gives n coins, 10 takes n of the last defined item. Fills the page's give or take.
struct PageItems {
    int giveCount = 0, takeCount = 0;
    Item give{}, take{};
};
void scriptOperation(PageItems &page, int operation, const Item &lastDefined, int n);

// The client side of taming (0xa0750, §5.2), for one damage number (0x48 bytes) and the local
// player's record. When the number is a kind-6 tame from an existing creature to this player
// holding pet food: the pet slot becomes the tamed race's pet item, the ability 0, the pet id the
// tamed creature's; the held item is cleared. The caller moves `oldPet` (when `hadPet`) to the
// inventory and removes one `food` from it, and prints "TAMED!".
struct Tamed {
    bool happened = false;
    bool hadPet = false;
    Item oldPet{}, food{};
};
Tamed tame(const uint8_t *number, uint8_t *player, const std::function<const uint8_t *(int64_t id)> &byId);

} // namespace grounditems
