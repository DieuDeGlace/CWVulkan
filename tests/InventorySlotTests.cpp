#include "InventorySlots.h"
#include "GameShop.h"
#include <string>
#include "NativePose.h"
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>

int inventorySlotTests() {
    using namespace inventoryslots;
    int failures = 0;
    auto check = [&](bool passed, const char *what) {
        if (!passed) {
            std::printf("FAIL inventory slots: %s\n", what);
            ++failures;
        }
    };
    const auto food = nativesaves::makeItem(1, 7);
    Inventory inventory;
    inventory.initializeKitTabs();
    check(inventory.tabs.size() == 6 && inventory.tabs[0].size() == 10 && inventory.tabs[3].size() == 30 && inventory.tabs[5].empty(),
          "kit tab lengths");
    inventory.tabs[2] = {{0, {}}, {3, food}};
    auto added = inventory.add(food, 4);
    check(added && *added == Index{2, 1} && inventory.tabs[2][0].count == 0 && inventory.tabs[2][1].count == 7,
          "later equal stack wins over earlier hole");
    inventory.add(food, -1);
    check(inventory.tabs[2][1].count == -1, "requested minus one overwrites quantity");
    inventory.add(food, 1);
    check(inventory.tabs[2][1].count == 0 && inventory.tabs[2][1].item == food, "merge does not rewrite or erase payload");
    const auto unknown = nativesaves::makeItem(15, 8);
    auto first = inventory.add(unknown, 1), second = inventory.add(unknown, 1);
    check(first && second && *first == Index{0, 0} && *second == Index{0, 1}, "unidentified records remain separate");
    auto oldPadding = unknown, newPadding = unknown;
    oldPadding[1] = 11, oldPadding[0x15] = 22, oldPadding[0x9e] = 77;
    newPadding[1] = 33, newPadding[0x15] = 44, newPadding[0x9e] = 55;
    inventory.tabs[0] = {{0, oldPadding}};
    inventory.add(newPadding, 1);
    check(inventory.tabs[0][0].item[1] == 11 && inventory.tabs[0][0].item[0x15] == 22 && inventory.tabs[0][0].item[0x9e] == 77,
          "semantic insertion preserves destination padding");
    inventory.takePage(unknown, 1);
    check(inventory.tabs[0][0].count == 0 && inventory.tabs[0][0].item[0] == 0 && inventory.tabs[0][0].item[0x9e] == 77,
          "default-item clearing copies meaningful fields without assigning padding");
    auto attached = food;
    attached[0x9d] = 1;
    attached[0x1d] = 2;
    inventory.tabs[2] = {{2, attached}};
    auto other = attached;
    other[0x1d] = 3;
    inventory.add(other, 1);
    check(inventory.tabs[2].size() == 2, "active attributes participate in stacking");
    attached[0x9e] = 71;
    inventory.add(attached, 1);
    check(inventory.tabs[2][0].count == 3 && inventory.tabs[2].size() == 2, "inactive padding is not an equality key");
    inventory.tabs[2] = {{2, food}, {3, food}, {-1, food}};
    inventory.takePage(inventory.tabs[2][0].item, 4);
    check(inventory.tabs[2].size() == 3 && inventory.tabs[2][0].count == 0 && inventory.tabs[2][0].item == Item{} &&
              inventory.tabs[2][1].count == 1 && inventory.tabs[2][2].count == -1,
          "page removal keeps indices and skips negative stacks");
    inventory.tabs[2] = {{1, food}, {-1, food}, {2, food}};
    inventory.takeCreature(food, 1);
    check(inventory.tabs[2][0].count == 0 && inventory.tabs[2][1].count == 0 && inventory.tabs[2][2].count == 1,
          "creature removal continues after request zero without protecting negative stacks");
    inventory.cursor = {1, food};
    inventory.clearTabs();
    inventory.add(nativesaves::makeItem(12, 0, 11), 3, 8);
    check(inventory.tabs.size() == 9 && inventory.gold == 3 && inventory.tabs[8].empty(), "grow tabs before coin shortcut");
    inventory.takePage(nativesaves::makeItem(12, 0, 11), 8);
    check(inventory.gold == 0 && inventory.cursor.count == 1, "coin clamp and cursor retention");
    inventory.add(nativesaves::makeItem(14, 0), 7);
    check(inventory.fieldAb0 == 7, "separate unnamed counter");
    auto stale = unknown;
    stale[0x14] = 3;
    stale[0x9e] = 77;
    inventory.tabs[0] = {{1, stale}};
    check(inventory.consumeOne({0, 0}) && inventory.tabs[0].size() == 1 && inventory.tabs[0][0].count == 0 &&
              inventory.tabs[0][0].item[0] == 0 && inventory.tabs[0][0].item[4] == 0 && inventory.tabs[0][0].item[0x14] == 3 &&
              inventory.tabs[0][0].item[0x9e] == 77,
          "UI clear changes only its three fields");
    auto gear = [](int id, int regionX, int usable, int marker) {
        auto item = nativesaves::makeItem(3, id, usable);
        const int32_t region[2] = {regionX, 0};
        std::memcpy(item.data() + 0xc, region, 8);
        item[0x9e] = (uint8_t)marker;
        return item;
    };
    inventory.tabs[0] = {{1, gear(3, 1, 1, 1)},  {1, gear(99, 2, 1, 2)}, {0, {}}, {1, gear(20, 1, 0, 3)}, {1, gear(5, 1, 1, 4)},
                         {-1, gear(4, 1, 1, 5)}, {1, gear(3, 1, 1, 6)}};
    check(!inventory.refresh({}) && inventory.tabs[0][0].item[0x9e] == 1, "missing stat queries leave storage untouched");
    SortContext context;
    context.selectedRegion = {1, 0};
    context.usable = [](const Item &item) { return item[0x1c] != 0; };
    context.power = [](const Item &item) {
        uint32_t id;
        std::memcpy(&id, item.data() + 4, 4);
        return (float)id;
    };
    check(inventory.refresh(context) && inventory.tabs[0].size() == 6, "sort then compact, retaining occupied negative quantity");
    const int markers[] = {4, 5, 1, 6, 3, 2};
    for (int index = 0; index < 6; ++index)
        check(inventory.tabs[0][(size_t)index].item[0x9e] == markers[index], "region/usability/power precedence and small ties");
    check(inventory.slot({0, 0})->item[0x9e] == 4, "same indices resolve current content after sort");
    auto special = [&](int type, uint32_t id, int regionX) {
        auto item = gear(0, regionX, 1, 0);
        item[0] = (uint8_t)type;
        std::memcpy(item.data() + 4, &id, 4);
        return item;
    };
    inventory.tabs[1] = {
        {1, special(22, 0, 2)}, {1, special(29, 0xffffffffu, 1)}, {1, special(22, 0xffffffffu, 1)}, {1, special(22, 1, 1)}};
    inventory.tabs[7] = {{1, unknown}, {1, special(1, 0xffffffffu, 2)}, {1, special(1, 1, 3)}, {0, stale}};
    check(inventory.refresh(context) && inventory.tabs[1][0].item[4] == 1 && inventory.tabs[1][1].item[4] == 255 &&
              inventory.tabs[1][2].item[0] == 29 && inventory.tabs[1][3].item[4] == 0,
          "tab1 selected region precedes unsigned type/subtype ordering");
    check(inventory.tabs[7].size() == 3 && inventory.tabs[7][0].item[4] == 1 && inventory.tabs[7][1].item[4] == 255 &&
              inventory.tabs[7][2].item == unknown,
          "later tabs preserve membership and use unsigned subtype ordering");
    inventory.tabs[0].assign(100, {});
    inventory.tabs[0][37] = {-1, gear(7, 1, 1, 42)};
    check(inventory.refresh(context) && inventory.tabs[0].size() == 1 && inventory.tabs[0][0].item[0x9e] == 42,
          "large empty partition compacts after native non-strict empty comparisons");
    struct WeaponCase {
        int incoming, button;
        bool shift;
        int left, right;
    };
    const WeaponCase weaponCases[] = {{13, 0, false, 13, 1}, {14, 1, true, 14, 1}, {6, 0, false, 6, 0},
                                      {7, 1, false, 0, 7},   {5, 0, false, 0, 5},  {8, 1, true, 0, 8},
                                      {0, 0, false, 4, 0},   {2, 1, false, 2, 1},  {2, 0, true, 2, 1}};
    for (const auto &c : weaponCases) {
        PlayerState state;
        state.inventory.tabs.resize(8);
        state.inventory.tabs[7] = {{1, nativesaves::makeItem(3, c.incoming)}};
        state.equipment[6] = nativesaves::makeItem(3, 4);
        state.equipment[7] = nativesaves::makeItem(3, 1);
        check(state.equip({7, 0}, c.button, c.shift), "weapon table admitted");
        auto matchesWeapon = [](const Item &item, int id) { return item[0] == 3 && item[4] == id; };
        const bool emptyLeft = c.left == 0 && (c.incoming == 5 || c.incoming == 6 || c.incoming == 7 || c.incoming == 8);
        const bool emptyRight = c.right == 0 && (c.incoming == 6 || c.incoming == 7);
        check((emptyLeft ? state.equipment[6][0] == 0 : matchesWeapon(state.equipment[6], c.left)) &&
                  (emptyRight ? state.equipment[7][0] == 0 : matchesWeapon(state.equipment[7], c.right)),
              "weapon destination fixture");
        check(state.inventory.tabs[7][0].count == 0 && state.inventory.tabs[7][0].item[0] == 0 && !state.inventory.tabs[0].empty(),
              "returns use default tab rather than nonstandard source tab");
    }
    PlayerState state;
    auto incoming = nativesaves::makeItem(3, 13);
    incoming[0x1d] = 81;
    incoming[0x9d] = 1;
    state.inventory.tabs[0] = {{1, incoming}};
    state.equipment[6] = nativesaves::makeItem(3, 4);
    state.equipment[7] = nativesaves::makeItem(3, 5);
    check(state.equip({0, 0}, 0, false) && state.inventory.tabs[0].size() == 2 && state.inventory.tabs[0][0].item[4] == 5 &&
              state.inventory.tabs[0][1].item[4] == 4 && state.equipment[6][0x1d] == 81 && state.equipment[7][0] == 0,
          "13 returns two-handed right before left, reusing freed source and retaining incoming attributes");
    state = {};
    state.inventory.tabs[0] = {{1, nativesaves::makeItem(3, 2)}};
    state.equipment[6] = nativesaves::makeItem(3, 5);
    check(state.equip({0, 0}, 0, false) && state.equipment[6][4] == 5 && state.equipment[7][4] == 2,
          "ordinary left retains an existing two-handed left weapon");
    for (int button : {0, 1}) {
        state = {};
        state.inventory.tabs[0] = {{1, nativesaves::makeItem(9, 2)}};
        check(state.equip({0, 0}, button, true) && state.equipment[button == 0 ? 9 : 8][0] == 9,
              "left and right rings follow separate native destinations independent of Shift");
    }
    state = {};
    state.equipment[0] = nativesaves::makeItem(26, 9);
    state.equipment[2] = nativesaves::makeItem(4, 3);
    state.inventory.tabs[0] = {{1, nativesaves::makeItem(26, 2)}};
    check(state.equip({0, 0}, 1, false) && state.equipment[0][4] == 2 && state.equipment[2][4] == 3 &&
              state.inventory.tabs[0][0].item[0] == 4,
          "head action returns chest while retaining chest and replacing old head");
    state = {};
    state.equipment[10] = nativesaves::makeItem(20, 1);
    state.inventory.tabs[4] = {{1, nativesaves::makeItem(20, 2)}};
    check(state.equip({4, 0}, 0, false) && state.inventory.tabs[4].size() == 2 && state.inventory.tabs[4][0].count == 0 &&
              state.inventory.tabs[4][1].item[4] == 1 && state.equipment[10][4] == 2,
          "pet returns before its still-occupied source is consumed, safely retaining indices over growth");
    auto worn = state.equipment[10];
    worn[0x9d] = 1;
    worn[0x1d] = 22;
    state.equipment[10] = worn;
    check(state.unequip(10) && state.equipment[10][0] == 0 && state.equipment[10][0x1d] == 22,
          "unequip returns item and clears only equipped type/subtype");

    std::array<uint8_t, nativepose::CREATURE_SIZE> actor{};
    actor[0x60] = 1; // nonplayer kind admits every item's class
    auto setFloat = [&](size_t at, float value) { std::memcpy(actor.data() + at, &value, 4); };
    auto floatAt = [&](size_t at) {
        float value;
        std::memcpy(&value, actor.data() + at, 4);
        return value;
    };
    for (const auto &c : {std::array<int, 3>{1, 1, 0x54}, {1, 7, 0x55}, {28, 99, 0x54}}) {
        state = {};
        auto food = nativesaves::makeItem(c[0], c[1]);
        food[0x1d] = 19;
        state.inventory.tabs[2] = {{-1, food}};
        setFloat(0x130, 0);
        setFloat(0x12c, 70);
        setFloat(0x6c, 900);
        const auto result = use(state, {2, 0}, actor.data(), 1, true, 200);
        check(result.action == Action::Used && actor[0x68] == c[2] && floatAt(0x6c) == 0 && floatAt(0x12c) == 0 &&
                  actor[0x1d8 + 0x1d] == 19 && state.inventory.tabs[2][0].count == 0,
              "direct consumption starts correct ability, retains payload and decrements even minus-one count");
    }
    state = {};
    state.inventory.tabs[2] = {{1, nativesaves::makeItem(1, 9)}};
    setFloat(0x130, 1);
    check(use(state, {2, 0}, actor.data(), 0, false, 0).action == Action::None && state.inventory.tabs[2][0].count == 1,
          "positive stun prevents delegated use");
    setFloat(0x130, 0);
    const auto bomb = use(state, {2, 0}, actor.data(), 0, false, 0);
    check(bomb.interaction && bomb.interaction->at(4) == 9 && state.inventory.tabs[2][0].count == 0,
          "bomb interaction owns incoming item before source clearing");
    state.inventory.tabs[2] = {{1, nativesaves::makeItem(1, 13)}};
    const auto portal = use(state, {2, 0}, actor.data(), 1, false, 0);
    check(portal.action == Action::Map && portal.interaction && state.inventory.tabs[2][0].count == 1,
          "map-item producer queues kind-one item without consuming source");
    state.inventory.tabs[4] = {{1, nativesaves::makeItem(21, 2)}};
    actor[0x68] = 0x54;
    setFloat(0x6c, 200);
    check(use(state, {4, 0}, actor.data(), 0, false, 200).action == Action::None, "pet-food age equality is not finished");
    setFloat(0x6c, 201);
    check(use(state, {4, 0}, actor.data(), 0, false, 200).action == Action::PetFood && actor[0x68] == 0x56 &&
              state.inventory.tabs[4][0].count == 1,
          "pet food starts presentation without immediate consumption");
    state.inventory.tabs[1] = {{1, nativesaves::makeItem(24, 1)}};
    actor[0x68] = 0x78;
    setFloat(0x6c, 123);
    setFloat(0x12c, 77);
    check(use(state, {1, 0}, actor.data(), 1, false, 0).action == Action::Used && actor[0x68] == 0 && floatAt(0x6c) == 123 &&
              floatAt(0x12c) == 77 && state.inventory.tabs[1][0].count == 1,
          "vehicle toggle off preserves age/dodge/quantity");
    actor[0x150] = 1;
    check(use(state, {1, 0}, actor.data(), 0, false, 0).action == Action::WrongRegion && actor[0x68] == 0,
          "vehicle region rejection leaves ability unchanged");
    actor[0x60] = 0;
    actor[0x14c] = 1;
    state.inventory.tabs[0] = {{1, nativesaves::makeItem(3, 6)}};
    check(use(state, {0, 0}, actor.data(), 0, false, 0).action == Action::Refused && state.inventory.tabs[0][0].count == 1,
          "native class admission precedes equipment mutation");
    inventory.tabs[3] = {{1, food}, {2, food}, {8, food}, {3, food}};
    inventory.tabs[4] = {{7, food}};
    inventory.takeCraft(food, 3);
    check(inventory.tabs[3][0].count == 0 && inventory.tabs[3][1].count == 0 && inventory.tabs[3][2].count == 5 &&
              inventory.tabs[3][3].count == 3 && inventory.tabs[4][0].count == 4,
          "craft consumption retains original request and moves to next tab after partial stack");
    setFloat(0x180, 5);
    int maximumQueries = 0;
    const float changedHealth = adjustHealth(actor.data(), 10, [&] {
        ++maximumQueries;
        check(floatAt(0x180) == (maximumQueries == 1 ? 5 : 10), "maximum callback observes HP before/after rescaling in order");
        return maximumQueries == 1 ? 20.0f : 8.0f;
    });
    check(changedHealth == 8 && maximumQueries == 2 && floatAt(0x180) == 8, "common health tail stores scale before second maximum/cap");
    setFloat(0x180, -5);
    check(adjustHealth(actor.data(), 10, [] { return 20.0f; }) == -10, "health tail has no lower-zero clamp");
    setFloat(0x180, 10);
    check(std::isnan(adjustHealth(actor.data(), 0, [] { return 0.0f; })), "health tail has no old-maximum-zero fallback");
    inventory = {};
    auto daily = nativesaves::makeItem(22, 9);
    daily[1] = 81;
    daily[0x1d] = 42;
    daily[0x9d] = 1;
    inventory.tabs.resize(8);
    inventory.tabs[7] = {{0, daily}, {-1, daily}, {4, food}};
    inventory.cursor = {1, daily};
    inventory.clearType(22);
    check(inventory.tabs[7].size() == 3 && inventory.tabs[7][0].item[0] == 0 && inventory.tabs[7][0].item[1] == 81 &&
              inventory.tabs[7][1].count == 0 && inventory.tabs[7][1].item[0x1d] == 0 && inventory.cursor.item == daily,
          "daily purge clears meaningful fields including stale empty records, retains holes/padding/cursor");
    inventory.tabs[2] = {{3, food}, {-1, food}};
    check(inventory.quantity(food) == 6, "quantity query includes full-equal signed counts across all tabs");
    actor[0x68] = 0;
    const auto dropped = inventory.dropOne({2, 1}, actor.data());
    check(dropped && *dropped == food && inventory.tabs[2][1].count == 0, "middle drop owns one payload and depletes negative quantity");
    for (int type : {22, 23, 24}) {
        inventory.tabs[2] = {{2, nativesaves::makeItem(type, 1)}};
        check(!inventory.dropOne({2, 0}, actor.data()) && inventory.tabs[2][0].count == 2, "protected category cannot be dropped");
    }
    inventory.tabs[2] = {{2, food}};
    actor[0x68] = 0x56;
    std::memcpy(actor.data() + 0x1d8, food.data(), food.size());
    check(!inventory.dropOne({2, 0}, actor.data()) && inventory.tabs[2][0].count == 2, "active pet-food record cannot be dropped");
    // Shop transactions assert observable call order as well as inventory/currency changes.
    gameshop::TradeHooks trade;
    std::string calls;
    trade.inventory = [&] { calls += 'i'; };
    trade.shop = [&] { calls += 's'; };
    trade.crafting = [&] { calls += 'c'; };
    trade.announce = [&](const auto &) { calls += 'a'; };
    trade.rememberBought = [&](const auto &) { calls += 'b'; };
    trade.sound = [&](int sound) { calls += sound == gameshop::SOUND_SOLD ? 'S' : 'B'; };
    trade.refused = [&](gameshop::Refusal reason) {
        calls += reason == gameshop::Refusal::Money ? 'm' : reason == gameshop::Refusal::Capacity ? 'r' : 'p';
    };
    const auto jewel = nativesaves::makeItem(8, 1, 11);
    const int buyPrice = gameshop::price(jewel), resale = gameshop::sellPrice(jewel);
    check(buyPrice > 0 && resale > 0, "shop fixture has positive prices");
    inventory = {};
    inventory.gold = buyPrice;
    check(gameshop::purchase(inventory, jewel, trade) && calls == "bBisac" && inventory.gold == 0 && inventory.quantity(jewel) == 1,
          "regular purchase data and direct/nested refresh announcement order");
    calls.clear();
    check(!gameshop::purchase(inventory, jewel, trade) && calls == "m" && inventory.quantity(jewel) == 1,
          "regular money failure has no completion callbacks");
    gameshop::BuyBack buyback{{jewel, 3}};
    inventory = {};
    inventory.gold = 2 * resale;
    calls.clear();
    check(gameshop::repurchase(inventory, buyback, 0, trade) == 2 && buyback.size() == 1 && buyback[0].second == 1 &&
              inventory.quantity(jewel) == 2 && inventory.gold == 0 && calls == "aaBsi",
          "one buyback click attempts whole node, retains unpaid remainder without money warning");
    const auto vehicle = nativesaves::makeItem(24, 5);
    inventory = {};
    inventory.gold = 100000;
    inventory.add(vehicle, 1);
    buyback = {{vehicle, 3}};
    calls.clear();
    check(gameshop::repurchase(inventory, buyback, 0, trade) == 0 && buyback[0].second == 3 && calls == "rBsi",
          "buyback carry failure stops attempts but still finalizes sound/shop/inventory refresh");
    buyback = {{jewel, -1}};
    calls.clear();
    check(gameshop::repurchase(inventory, buyback, 0, trade) == 0 && buyback[0].second == -1 && calls == "Bsi",
          "nonpositive buyback node is retained unless exactly zero");
    buyback[0].second = 0;
    calls.clear();
    check(gameshop::repurchase(inventory, buyback, 0, trade) == 0 && buyback.empty() && calls == "Bsi",
          "zero buyback node unlinks during finalization");
    inventory = {};
    inventory.tabs[0] = {{-1, jewel}, {2, jewel}};
    buyback.clear();
    calls.clear();
    check(gameshop::sellSlot(inventory, {0, 0}, buyback, trade) && inventory.tabs[0][0].count == 0 && inventory.tabs[0][1].count == 2 &&
              inventory.gold == resale && buyback.size() == 1 && calls == "Ssc",
          "canonical sale consumes exact live slot before separate buyback/shop/crafting finish");
    calls.clear();
    check(gameshop::sellSlot(inventory, {0, 1}, buyback, trade) && buyback.size() == 2 && buyback[0].second == 1 && buyback[1].second == 1,
          "equal sales remain separate buyback nodes");
    inventory.tabs[1] = {{1, jewel}};
    calls.clear();
    check(!gameshop::sellSlot(inventory, {1, 0}, buyback, trade) && calls == "p" && inventory.tabs[1][0].count == 1,
          "protected sale depends on actual saved tab, not category");
    state = {};
    state.equipment[2] = nativesaves::makeItem(4, 2, 1);
    state.equipment[2][0x1d] = 77;
    trade.shop = [&] {
        calls += 's';
        check(state.equipment[2][0] == 4, "equipped sale clears only after shop rebuild");
    };
    calls.clear();
    buyback.clear();
    check(gameshop::sellEquipment(state, 2, buyback, trade) && state.equipment[2][0] == 0 && state.equipment[2][0x1d] == 77 &&
              buyback.size() == 1 && calls == "Ss",
          "equipped sale has no tab decrement or extra inventory/crafting refresh");
    std::printf("Canonical inventory slots: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
