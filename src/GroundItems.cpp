// Ground items, the drop hop, pick-up and receiving items, written from
// docs/specs/loot-pickup.md. See GroundItems.h.
#include "GroundItems.h"

#include "NativeInteract.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace grounditems {
namespace {

// Item record fields (drops-and-items.md §0.1).
constexpr size_t TYPE = 0x00, SUBTYPE = 0x04, MODIFIER = 0x08, RARITY = 0x14, MATERIAL = 0x1c;

// The creature record (worldgen-creatures.md §7.1) and a damage number (creature-hit.md §0.3).
// (loot-pickup.md §5.2 gives the number's fields 0x10 further on, +0x10, +0x18, +0x20 and +0x55:
// offsets into the world list's node, whose record starts after two link pointers.)
namespace creature {
constexpr size_t ID = 0x08, RACE = 0x64, ABILITY = 0x68, ITEM_IN_USE = 0x1d8, PET_SLOT = 0x8b8, PET_ID = 0x9d8;
}
namespace number {
constexpr size_t FROM = 0x00, TO = 0x08, VALUE = 0x10, KIND = 0x45;
constexpr uint8_t TAMING = 6;
constexpr float TAME_THRESHOLD = 1.0f;
} // namespace number

// Wall items sit 0.2 block from their wall (0x4725ac: 13107.2, truncated).
constexpr int64_t WALL_GAP = 13107;

// The hold-up abilities (§4 step 3.5).
constexpr int HOLD_UP = 0x7c, VEHICLE_ONE = 0x78, VEHICLE_TWO = 0x79;
constexpr uint32_t PLAYER_FLAG_BIT_1 = 2;

// The gold bag's value (use-prompt.md §3.7.3).
constexpr uint32_t GOLD_BAG_SALT = 0xcc72f;

template <typename T> T get(const uint8_t *p, size_t at) {
    T v;
    std::memcpy(&v, p + at, sizeof v);
    return v;
}
template <typename T> void put(uint8_t *p, size_t at, T v) { std::memcpy(p + at, &v, sizeof v); }

int64_t truncatedBlock(int64_t v) { return v / BLOCK; } // toward zero
int32_t floorBlock(int64_t v) { return (int32_t)(v >> 16); }

bool isGear(int t) { return (t >= 3 && t <= 9) || t == 0x1a; }

} // namespace

void snapToWall(int64_t &x, int64_t &y, float &angle, int64_t groundZ, const SolidAt &solid) {
    const int32_t z = (int32_t)truncatedBlock(groundZ);
    auto solidAt = [&](int64_t px, int64_t py) { return solid((int32_t)truncatedBlock(px), (int32_t)truncatedBlock(py), z); };
    if (solidAt(x, y - BLOCK))
        y = truncatedBlock(y) * BLOCK + WALL_GAP, angle = 180.0f;
    if (solidAt(x, y + BLOCK))
        y = (truncatedBlock(y) + 1) * BLOCK - WALL_GAP, angle = 0.0f;
    if (solidAt(x - BLOCK, y))
        x = truncatedBlock(x) * BLOCK + WALL_GAP, angle = 90.0f;
    if (solidAt(x + BLOCK, y))
        x = (truncatedBlock(x) + 1) * BLOCK - WALL_GAP, angle = 270.0f;
}

bool countDown(float &delayMs, int32_t &timerMs, float dtMs) {
    if (timerMs > 0)
        timerMs = std::max((int32_t)((float)timerMs - dtMs), 0);
    if (!(delayMs > 0.0f))
        return false;
    delayMs -= dtMs;
    if (delayMs > 0.0f)
        return false;
    delayMs = 0.0f;
    return true;
}

float hopHeight(float delayMs) {
    if (!(delayMs > 0.0f))
        return 0.0f;
    const float h = (delayMs - HOP_HALF_TIME) / HOP_HALF_TIME;
    const int64_t dots = (int64_t)((1.0f - h * h) * HOP_HEIGHT * (float)BLOCK); // truncated, as drawn
    return (float)dots / (float)BLOCK;
}

float tumble(float delayMs) { return delayMs != 0.0f ? delayMs * TUMBLE_PER_MS : 0.0f; }

bool picksUpItself(int itemType, uint8_t flags, float delayMs, float distance2) {
    if (itemType != type::COIN && itemType != type::TYPE_0E)
        return false;
    if (flags & ground::STAYS_FOR_OTHERS)
        return false;
    return delayMs == 0.0f && distance2 <= AUTO_PICK_UP_RANGE * AUTO_PICK_UP_RANGE;
}

int moneyValue(const Item &item) {
    if (item[TYPE] != type::GOLD_BAG)
        return 0;
    uint32_t h = get<uint32_t>(item.data(), MODIFIER) + GOLD_BAG_SALT;
    h ^= h << 13;
    h ^= h >> 17;
    h ^= h << 5;
    return (int)(((float)(h % 100000u) / 100000.0f + (float)item[RARITY] + 1.0f) * 10.0f);
}

Receipt receive(Keeper &keeper, const Item &noticeItem, const Item &adapted, int count, uint8_t flags, const int64_t position[3],
                const int32_t region[2], uint32_t playerFlags, bool abilityFree) {
    Receipt r;
    // 3.1: an item that stays for others is taken once per character, by its block.
    if (flags & ground::STAYS_FOR_OTHERS) {
        const std::array<int32_t, 3> block{floorBlock(position[0]), floorBlock(position[1]), floorBlock(position[2])};
        if (!keeper.perPlayerTaken.insert(block).second) {
            r.announce = false;
            return r;
        }
    }
    // 3.2: into the region counter, the gold or the inventory.
    const int t = adapted[TYPE];
    if (t == type::COIN && adapted[MATERIAL] == REGION_COIN_MATERIAL) {
        keeper.regionCounters[{region[0], region[1]}] += count;
        r.into = Into::RegionCounter, r.amount = count;
    } else if (const int money = moneyValue(adapted)) {
        r.into = Into::Gold, r.amount = money;
    } else {
        r.into = Into::Inventory, r.amount = count;
    }
    // 3.4: treasure marks its cell as used, as opening a chest does.
    if (noticeItem[TYPE] == type::TREASURE) {
        r.markCell = true;
        r.cell = {(int32_t)(position[0] >> 20), (int32_t)(position[1] >> 20)}; // 16-block cells
    }
    // 3.5: the receipt dialog and holding the item up.
    if (noticeItem[TYPE] == type::TREASURE || t == type::SPECIAL_17 || t == type::VEHICLE || t == type::TYPE_1D) {
        const bool fine = t == type::SPECIAL_17 || t == type::VEHICLE || (isGear(t) && (int8_t)adapted[RARITY] > 2);
        r.dialog = {"define_item",
                    std::to_string(t),
                    std::to_string(get<int32_t>(adapted.data(), SUBTYPE)),
                    std::to_string(get<uint32_t>(adapted.data(), MODIFIER)),
                    std::to_string(adapted[MATERIAL]),
                    std::to_string(adapted[RARITY]),
                    fine ? "you_received_the_item" : "you_received_an_item"};
        if (t == type::VEHICLE)
            r.dialog.push_back("new_page"); // then define_land and the vehicle's text (not specified)
        if (abilityFree) {
            const int32_t subtype = get<int32_t>(adapted.data(), SUBTYPE);
            if (t == type::VEHICLE && subtype == 1)
                r.holdUpAbility = VEHICLE_ONE;
            else if (t == type::VEHICLE && subtype == 2)
                r.holdUpAbility = (playerFlags & PLAYER_FLAG_BIT_1) ? VEHICLE_TWO : 0;
            else
                r.holdUpAbility = HOLD_UP;
        }
    }
    return r;
}

PageGift pageGives(int count, const Item &item, bool mayCarry) {
    PageGift g;
    if (count == 0 || !mayCarry)
        return g;
    g.gives = true;
    g.receiptDialog = count == 1 && item[TYPE] == type::VEHICLE;
    return g;
}

void pageTakes(std::vector<Stack *> &stacksInOrder, int32_t &gold, const Item &item, int count) {
    if (count == 0)
        return;
    if (item[TYPE] == type::COIN && item[MATERIAL] == COIN_MATERIAL) {
        gold = std::max(gold - count, 0);
        return;
    }
    int left = count;
    for (Stack *s : stacksInOrder) {
        if (left <= 0)
            break;
        if (s->count <= 0 || !nativeinteract::sameItem(s->item.data(), item.data()))
            continue;
        const int taken = std::min(left, s->count);
        s->count -= taken, left -= taken;
        if (s->count == 0)
            s->item = Item{};
    }
}

void scriptOperation(PageItems &page, int operation, const Item &lastDefined, int n) {
    switch (operation) {
    case 8: page.give = lastDefined, page.giveCount = 1; break;
    case 9: {
        Item coins{};
        coins[TYPE] = type::COIN;
        coins[MATERIAL] = COIN_MATERIAL;
        page.give = coins, page.giveCount = n;
        break;
    }
    case 10: page.take = lastDefined, page.takeCount = n; break;
    default: break;
    }
}

Tamed tame(const uint8_t *n, uint8_t *player, const std::function<const uint8_t *(int64_t id)> &byId) {
    Tamed t;
    const int64_t from = get<int64_t>(n, number::FROM), to = get<int64_t>(n, number::TO);
    const float value = get<float>(n, number::VALUE);
    if (!player || !(value >= 0.0f) || n[number::KIND] != number::TAMING || !(value >= number::TAME_THRESHOLD))
        return t;
    if (to != get<int64_t>(player, creature::ID))
        return t;
    const uint8_t *tamed = byId(from);
    if (!tamed || player[creature::ITEM_IN_USE + TYPE] != type::PET_FOOD)
        return t;
    t.happened = true;
    std::memcpy(t.food.data(), player + creature::ITEM_IN_USE, t.food.size());
    std::memcpy(t.oldPet.data(), player + creature::PET_SLOT, t.oldPet.size());
    t.hadPet = t.oldPet[TYPE] != 0;
    Item pet{};
    pet[TYPE] = type::PET;
    std::memcpy(&pet[SUBTYPE], tamed + creature::RACE, 4);
    std::memcpy(player + creature::PET_SLOT, pet.data(), pet.size());
    player[creature::ABILITY] = 0; // (a byte)
    put<int64_t>(player, creature::PET_ID, get<int64_t>(tamed, creature::ID));
    std::memset(player + creature::ITEM_IN_USE, 0, Item{}.size());
    return t;
}

} // namespace grounditems
