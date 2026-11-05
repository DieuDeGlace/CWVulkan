#include "GameCraft.h"
#include "InventorySlots.h"
#include <iostream>

int craftingCountTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL crafting count: " << label << '\n';
            ++failures;
        }
    };
    gamecraft::Thing a, b;
    a.category = b.category = 11;
    a.id = 1;
    b.id = 2;
    gamecraft::Formula formula;
    check(gamecraft::times(formula,
                           [&](const auto &) {
                               check(false, "empty ingredients must not query inventory");
                               return 1;
                           }) == 0,
          "empty ingredients retain count0");
    const auto held = [&](const auto &item) { return item.id == 1 ? -7 : 12; };
    formula.ingredients = {{a, 2}};
    check(gamecraft::times(formula, held) == -3, "surviving negative signed quotient is not zero-clamped");
    formula.ingredients = {{a, 2}, {b, 3}};
    check(gamecraft::times(formula, held) == 4, "negative candidate is unset before the next ingredient");
    formula.ingredients = {{b, 3}, {a, 2}};
    check(gamecraft::times(formula, held) == -3, "ingredient order matters for negative candidates");
    formula.ingredients = {{a, -3}};
    check(gamecraft::times(formula, [](const auto &) { return 7; }) == -2, "signed division truncates toward zero");
    inventoryslots::Inventory inventory;
    const auto raw = gamecraft::toRecord(a);
    inventory.tabs[3] = {{INT32_MAX, raw}, {2, raw}};
    formula.ingredients = {{a, 2}};
    check(gamecraft::times(formula, [&](const auto &item) { return inventory.quantity(gamecraft::toRecord(item)); }) ==
              -1073741823,
          "actual carried-total wrapping occurs before signed division");
    formula.ingredients = {{a, 0}};
    check(gamecraft::times(formula, held) == 0,
          "engine defines a zero-divisor fallback without claiming native success");
    formula.ingredients = {{a, -1}};
    check(gamecraft::times(formula, [](const auto &) { return INT32_MIN; }) == 0,
          "engine avoids signed division overflow for an unqualified native fault input");
    std::cout << "Crafting retained counts: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
