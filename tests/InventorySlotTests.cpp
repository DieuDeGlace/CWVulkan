#include "InventorySlots.h"
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
    inventory.takePage(food, 4);
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
    std::printf("Canonical inventory slots: %s\n", failures ? "FAILED" : "PASS");
    return failures;
}
