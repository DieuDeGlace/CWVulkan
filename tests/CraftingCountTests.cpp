#include "CraftingCollection.h"
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
    gamecraft::Collection collection;
    collection.rows.resize(9);
    collection.rows[6] = {{19, raw}};
    collection.rows[8] = {{23, raw}};
    gamecraft::RecipeGroups recipes;
    formula.result = b;
    formula.result.attributes[90] = 0x9b;
    formula.workstation = gamecraft::Campfire;
    formula.ingredients = {{a, 2}};
    recipes[0] = {formula, formula};
    recipes[0].push_back(gamecraft::Formula{});
    check(collection.rebuild(inventory, recipes, {0, 1}) && collection.rows.size() == 9 &&
              collection.rows[0].size() == 3 && collection.rows[0][0].count == -1073741823 &&
              collection.rows[0][1].count == -1073741823 && collection.rows[0][2].count == 0,
          "retained groups preserve recipe order, duplicates, negative counts and zero rows");
    check(collection.rows[6].empty() && collection.rows[8].size() == 1 && collection.rows[8][0].count == 23,
          "rebuild clears group6 without truncating higher groups");
    check(collection.button && collection.button->source == inventoryslots::Index{0, 1} &&
              collection.button->formula.workstation == gamecraft::Campfire &&
              collection.button->formula.ingredients[0].quantity == 2,
          "matching physical selection copies recipe scalar and ingredients, not the computed row count");
    recipes[0][1].ingredients[0].quantity = 91;
    recipes[0][1].result.attributes[90] = 0;
    check(collection.button->formula.ingredients[0].quantity == 2 &&
              collection.button->formula.result.attributes[90] == 0x9b &&
              collection.rows[0][1].item == gamecraft::toRecord(formula.result),
          "selected recipe and retained items own copies after source mutation");
    inventory.tabs.clear();
    recipes = {};
    check(!collection.rebuild(inventory, recipes, {0, 0}) && collection.rows[0].size() == 3 &&
              collection.button->source == inventoryslots::Index{0, 1},
          "empty carried outer vector skips without clearing retained rows or selected recipe");
    inventory.tabs.resize(1); // A valid empty carried group admits rebuilding.
    check(collection.rebuild(inventory, recipes, {-1, -1}) && collection.rows[0].empty() && collection.button &&
              collection.button->formula.ingredients[0].quantity == 2,
          "empty recipe inputs clear live groups but a nonmatching selection retains the previous button copy");
    collection.rows[0] = {{0, raw}, {-7, raw}};
    int queries = 0;
    check(collection.publish({0, 1},
                             [&](const auto &item, bool tabSix) -> std::optional<gamecraft::Formula> {
                                 ++queries;
                                 check(!tabSix && item == raw,
                                       "publication receives physical row item despite negative count");
                                 return formula;
                             }) &&
              queries == 1 && collection.button->source == inventoryslots::Index{0, 1},
          "left-down publication uses retained row identity independently of craftable count");
    auto actionSnapshot = collection.button->formula;
    formula.ingredients[0].quantity = 9;
    check(
        collection.publish({0, 0}, [&](const auto &, bool) -> std::optional<gamecraft::Formula> { return formula; }) &&
            collection.button->formula.ingredients[0].quantity == 9 && actionSnapshot.ingredients[0].quantity == 2,
        "later publication does not replace an owned in-progress action snapshot");
    check(!collection.publish({0, 99},
                              [&](const auto &, bool) -> std::optional<gamecraft::Formula> {
                                  check(false, "invalid pair must not generate a recipe");
                                  return formula;
                              }) &&
              !collection.button,
          "invalid publication clears details while leaving the caller's selection separate");
    check(!collection.publish({0, 0},
                              [](const auto &, bool) -> std::optional<gamecraft::Formula> { return std::nullopt; }) &&
              !collection.button,
          "failed recipe generation clears the details value");
    collection.rows[6] = {{0, raw}};
    check(collection.publish({6, 0},
                             [&](const auto &, bool tabSix) -> std::optional<gamecraft::Formula> {
                                 check(tabSix, "group6 is forwarded explicitly to the recipe query");
                                 return gamecraft::Formula{};
                             }) &&
              collection.button && collection.button->source == inventoryslots::Index{6, 0},
          "successful empty recipe publication retains its valid source pair");
    std::cout << "Crafting retained counts: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
