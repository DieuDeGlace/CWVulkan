#pragma once
// Shops, buying, selling, buying back and identifying (docs/specs/loot-shops.md):
//
//   * what a shopkeeper offers on a day (0x2ac6e0 armour, 0x2acf90 weapons, 0x2acbd0 tier
//     resources, 0x2acdd0 general goods), from a seed of its id and the day through the game's
//     rand() and its item generators (NativeDrop), less what the player already bought there;
//   * the shop tier (0x2bb290): one more rarity per solved role-1 mission place of the region;
//   * the buy-back list of the last ten items sold (0x94900);
//   * the identify window (0x965e0): a type-15 item becomes a random weapon or armour piece of
//     its rarity, bound to its region.
// Items are game-layout records (0xa0 bytes, drops-and-items.md §0.1).
#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <set>
#include <utility>
#include <vector>

namespace inventoryslots {
struct Inventory;
struct PlayerState;
struct Index;
} // namespace inventoryslots

namespace gameshop {

using Item = std::array<uint8_t, 0xa0>;

// The townsfolk classes that trade (loot-shops.md §6).
constexpr int GENERAL_SHOP = 0x80, WEAPON_SHOP = 0x81, ARMOUR_SHOP = 0x82, IDENTIFIER = 0x83, TIER_SHOP = 0x9c;

// The interface sounds of trading (sound.md).
constexpr int SOUND_REFUSED = 0x34, SOUND_BOUGHT = 0x3c, SOUND_SOLD = 0x3d, SOUND_IDENTIFIED = 0x31;
constexpr float SOLD_VOLUME = 0.75f;

struct Offer {
    Item item{};
    int count = -1; // −1: as many as wanted
};

// The shopkeeper: its class, its id (creature +0x08) and its home (+0x1b8, +0x1c0, in 1/65536
// of a block), where its gear's region comes from.
struct Shopkeeper {
    int cls = GENERAL_SHOP;
    int64_t id = 0;
    int64_t home[2] = {0, 0};
};

// The stock on `day` (0xb75d0, §1–§2), everything on tab 0 in the game's order. `playerClass`
// is the local player's; `tier` the shop tier; `bought` how many of an item the player bought
// from this shop (0x2a0c80, by item equality).
std::vector<Offer> stock(const Shopkeeper &shop, int day, int playerClass, int tier, const std::function<int(const Item &)> &bought);

// The shop tier (0x2bb290, §1.2): the region's role-1 places that are solved, at most 4.
// `solved` holds the player's solved place cells (player data +0xc8).
int shopTier(int32_t regionX, int32_t regionY, const std::set<std::array<int32_t, 2>> &solved);

// How many of `item` a bought list holds (0x2a0c80).
int boughtCount(const std::vector<Item> &boughtHere, const Item &item);

// Gear whose price doubles and that counts in the regions next to its own (stat-rules.md §2.7).
bool renowned(const Item &item);
int price(const Item &item);     // the buying price (0x109d30)
int sellPrice(const Item &item); // the selling price (0x10a390)

// The buy-back list (controller +0x21b0, §4.2): (count, item) entries, oldest first.
constexpr size_t BUY_BACK_LENGTH = 10;
using BuyBack = std::deque<std::pair<Item, int>>;
// A sale: one of `item` joins the list; past ten entries the oldest goes.
void rememberSold(BuyBack &list, const Item &item);

// Reached native shop actions. Rebuild callbacks include their own widget refresh;
// each bound widget can sort the player's canonical slots even while hidden.
enum class Refusal { Money, Capacity, ProtectedTab };
struct TradeHooks {
    std::function<void()> inventory, shop, crafting;
    std::function<void(const Item &)> announce, rememberBought;
    std::function<void(int)> sound;
    std::function<void(Refusal)> refused;
};
bool purchase(inventoryslots::Inventory &, Item, const TradeHooks &);
int repurchase(inventoryslots::Inventory &, BuyBack &, size_t newestRow, const TradeHooks &);
bool sellSlot(inventoryslots::Inventory &, inventoryslots::Index, BuyBack &, const TradeHooks &);
bool sellEquipment(inventoryslots::PlayerState &, int slot, BuyBack &, const TradeHooks &);

// --- Identifying (§5) ------------------------------------------------------------------------

constexpr uint8_t UNIDENTIFIED = 0x0f; // the only type the window takes
// The window's price (0x277bc0): half the item's price, at least 1.
int identifyPrice(const Item &item);
// What identifying `slotted` gives (0x965e0 steps 3–4): one rand picks a weapon (even) or an
// armour piece (odd) of the item's rarity for any class; it keeps the item's region.
Item identify(const Item &slotted, const std::function<int()> &rand);

} // namespace gameshop
