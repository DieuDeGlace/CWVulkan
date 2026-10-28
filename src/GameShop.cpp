// Shops and the identify window, written from docs/specs/loot-shops.md. See GameShop.h.
#include "GameShop.h"

#include "NativeDrop.h"
#include "NativeInteract.h"
#include "NativeRules.h"
#include "worldgen/Noise.h"
#include "worldgen/Planner.h"
#include "worldgen/Regions.h"

#include <algorithm>
#include <cstring>

namespace gameshop {
namespace {

// Item record fields (drops-and-items.md §0.1).
constexpr size_t TYPE = 0x00, SUBTYPE = 0x04, MODIFIER = 0x08, REGION = 0x0c, RARITY = 0x14, MATERIAL = 0x1c;

// The seeds (§2): low 32 bits of the shopkeeper's id + day × 13, plus an offset for gear.
constexpr uint32_t DAY_FACTOR = 13;
constexpr uint32_t ARMOUR_SEED_OFFSET = 0x10ec, WEAPON_SEED_OFFSET = 0x156b6;
// Pieces per rarity: rand mod 4 + 1 armour pieces, rand mod 2 + 1 weapons.
constexpr int ARMOUR_PER_RARITY = 4, WEAPONS_PER_RARITY = 2;
constexpr int HIGHEST_TIER = 4;

// Goods the general and tier shops sell (type 11 resources, type 0x15 pet food, type 1).
constexpr uint8_t CONSUMABLE = 1, RESOURCE = 11, PET_FOOD = 0x15;
constexpr int32_t PET_FOODS[] = {0x22, 0x23, 0x13, 0x1a, 0x1e, 0x57};

uint32_t low32(int64_t id) { return (uint32_t)(uint64_t)id; }

// The truncated block of a coordinate in 1/65536 of a block (toward zero).
int32_t truncatedBlock(int64_t dots) { return (int32_t)(dots / 65536); }

Item makeItem(uint8_t type, int32_t subtype, uint8_t material = 0, uint8_t rarity = 0) {
    Item it{};
    it[TYPE] = type;
    std::memcpy(&it[SUBTYPE], &subtype, 4);
    it[MATERIAL] = material;
    it[RARITY] = rarity;
    return it;
}

// A generator world that only draws from `rng`.
nativedrop::World generatorWorld(worldgen::CrtRandom &rng) {
    nativedrop::World w;
    w.rand = [&rng] { return rng.next(); };
    return w;
}

// §2.1 and §2.2: gear of rarity 0..tier, a few pieces each, of the player's class.
std::vector<Offer> gearStock(const Shopkeeper &shop, int day, int playerClass, int tier, bool weapons,
                             const std::function<int(const Item &)> &bought) {
    worldgen::CrtRandom rng;
    rng.seed(low32(shop.id) + (uint32_t)day * DAY_FACTOR + (weapons ? WEAPON_SEED_OFFSET : ARMOUR_SEED_OFFSET));
    nativedrop::World w = generatorWorld(rng);
    const worldgen::Int2 region = worldgen::regionAt(truncatedBlock(shop.home[0]), truncatedBlock(shop.home[1]));
    std::vector<Item> made;
    for (int rarity = 0; rarity <= tier; rarity++) {
        const int pieces = rng.next() % (weapons ? WEAPONS_PER_RARITY : ARMOUR_PER_RARITY) + 1;
        for (int k = 0; k < pieces; k++) {
            Item it{};
            if (weapons)
                nativedrop::weapon(w, it.data(), (uint8_t)rarity, playerClass);
            else
                nativedrop::armour(w, it.data(), (uint8_t)rarity, playerClass, false);
            std::memcpy(&it[REGION], &region.x, 4);
            std::memcpy(&it[REGION + 4], &region.y, 4);
            const uint32_t modifier = (uint32_t)rng.next();
            std::memcpy(&it[MODIFIER], &modifier, 4);
            made.push_back(it);
        }
    }
    // The game sorts by rarity; the list is made in rarity order and the sort is stable, so
    // this keeps the order.
    std::stable_sort(made.begin(), made.end(), [](const Item &a, const Item &b) { return a[RARITY] < b[RARITY]; });
    std::vector<Offer> offers;
    for (const Item &it : made)
        offers.push_back({it, std::max(1 - bought(it), 0)});
    return offers;
}

// §2.3: the four tier stones, a few each a day.
std::vector<Offer> tierStock(const Shopkeeper &shop, int day, const std::function<int(const Item &)> &bought) {
    worldgen::CrtRandom rng;
    rng.seed((uint32_t)day * DAY_FACTOR + low32(shop.id));
    std::vector<Offer> offers;
    for (int rarity = 1; rarity <= 4; rarity++) {
        const Item stone = makeItem(RESOURCE, 0x1f + rarity, 0, (uint8_t)rarity);
        const int base = rng.next() % 5 + (4 - rarity); // 3, 2, 1, 0 more for higher tiers
        offers.push_back({stone, std::max(base - bought(stone), 0)});
    }
    return offers;
}

// §2.4: flasks, the other resource and type 1 subtype 9 without limit, and one pet food a day.
std::vector<Offer> generalStock(const Shopkeeper &shop, int day, const std::function<int(const Item &)> &bought) {
    worldgen::CrtRandom rng;
    rng.seed((uint32_t)day * DAY_FACTOR + low32(shop.id));
    std::vector<Offer> offers;
    offers.push_back({makeItem(RESOURCE, 0x0c, 0x19), -1});
    offers.push_back({makeItem(RESOURCE, 0x26), -1});
    const Item food = makeItem(PET_FOOD, PET_FOODS[rng.next() % 6]);
    offers.push_back({food, std::max(1 - bought(food), 0)});
    offers.push_back({makeItem(CONSUMABLE, 9), -1});
    return offers;
}

} // namespace

std::vector<Offer> stock(const Shopkeeper &shop, int day, int playerClass, int tier, const std::function<int(const Item &)> &bought) {
    std::vector<Offer> offers;
    switch (shop.cls) {
    case WEAPON_SHOP: offers = gearStock(shop, day, playerClass, tier, true, bought); break;
    case ARMOUR_SHOP: offers = gearStock(shop, day, playerClass, tier, false, bought); break;
    case TIER_SHOP: offers = tierStock(shop, day, bought); break;
    default: offers = generalStock(shop, day, bought); break;
    }
    // A quantity of 0 adds nothing to the list.
    offers.erase(std::remove_if(offers.begin(), offers.end(), [](const Offer &o) { return o.count == 0; }), offers.end());
    return offers;
}

int shopTier(int32_t regionX, int32_t regionY, const std::set<std::array<int32_t, 2>> &solved) {
    const auto plan = worldgen::regionPlan(regionX, regionY);
    int tier = 0;
    for (const auto &[place, role] : plan->roles)
        if (role == worldgen::role::GNOME && solved.count({place.first, place.second}))
            tier++;
    return std::min(tier, HIGHEST_TIER);
}

int boughtCount(const std::vector<Item> &boughtHere, const Item &item) {
    int n = 0;
    for (const Item &b : boughtHere)
        n += nativeinteract::sameItem(b.data(), item.data());
    return n;
}

bool renowned(const Item &item) { return nativerules::renowned(item.data()); }
int price(const Item &item) { return nativerules::itemPrice(item.data()); }
int sellPrice(const Item &item) { return nativerules::sellPrice(nativerules::itemPrice(item.data())); }

void rememberSold(BuyBack &list, const Item &item) {
    list.push_back({item, 1});
    if (list.size() > BUY_BACK_LENGTH)
        list.pop_front();
}

int identifyPrice(const Item &item) { return std::max(price(item) / 2, 1); }

Item identify(const Item &slotted, const std::function<int()> &rand) {
    nativedrop::World w;
    w.rand = rand;
    Item made{};
    if (rand() % 2 == 0)
        nativedrop::weapon(w, made.data(), slotted[RARITY], -1);
    else
        nativedrop::armour(w, made.data(), slotted[RARITY], -1, false);
    if (nativedrop::inRegionSet(made[TYPE]))
        std::memcpy(&made[REGION], &slotted[REGION], 8);
    return made;
}

} // namespace gameshop
