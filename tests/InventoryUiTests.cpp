#include "InventorySlots.h"
#include "InventoryUi.h"
#include <iostream>
#include <string>

int inventoryUiTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL inventory UI: " << label << '\n';
            ++failures;
        }
    };
    int row = 99, refreshes = 0;
    const auto refresh = [&] {
        ++refreshes;
        if (row > 2)
            row = 2;
        return 3;
    };
    check(!inventoryui::requestRow(row, 1, refresh) && row == 2 && refreshes == 1,
          "refresh clamp at upper boundary is not a row movement or sound");
    check(inventoryui::requestRow(row, -1, refresh) && row == 1 && refreshes == 2,
          "previous row uses the state after refresh");
    check(!inventoryui::requestRow(row, 0, refresh) && row == 1 && refreshes == 2,
          "zero input does not reach a row handler");
    row = 0;
    check(!inventoryui::requestRow(row, -1, refresh) && refreshes == 3, "lower boundary still refreshes");
    // Two opposing inputs are separate visits, not a summed zero. The first
    // upper-bound attempt stays at2; the following previous-row request reaches1.
    row = 2;
    const bool next = inventoryui::requestRow(row, 1, refresh);
    const bool previous = inventoryui::requestRow(row, -1, refresh);
    check(!next && previous && row == 1 && refreshes == 5, "opposing input visits retain boundary and refresh order");
    row = 4;
    check(!inventoryui::requestRow(row, 1,
                                   [&] {
                                       row = 0;
                                       return 0;
                                   }) &&
              row == 0,
          "physically empty refreshed tab has no available row positions");
    row = -2;
    check(inventoryui::requestRow(row, 1, refresh) && row == -1,
          "row handler does not add an unproved lower clamp to refresh");
    inventoryui::Region region{1, 1};
    const std::vector<inventoryui::Region> choices{{1, 1}, {2, 2}};
    std::string visits;
    const auto sound = [&] {
        check(region == inventoryui::Region{1, 1}, "region sound precedes the active-region change");
        visits += 's';
    };
    const auto sort = [&] {
        check(region == inventoryui::Region{2, 2}, "explicit next-region sort observes the changed region");
        visits += 'c';
    };
    inventoryui::requestRegion(region, choices, 1, sound, sort, [&] { visits += 'r'; }, [&] { visits += 't'; });
    check(region == inventoryui::Region{2, 2} && visits == "scrt",
          "next region has sound, explicit canonical sort, refresh and thumb order");
    visits.clear();
    inventoryui::requestRegion(
        region, choices, -1, [&] { visits += 's'; }, [&] { visits += 'c'; }, [&] { visits += 'r'; },
        [&] { visits += 't'; });
    check(region == inventoryui::Region{1, 1} && visits == "srt",
          "previous region refreshes without the extra canonical sort");
    visits.clear();
    inventoryui::requestRegion(
        region, {}, 1, sound, [&] { visits += 'c'; }, [&] { visits += 'r'; }, [&] { visits += 't'; });
    check(region == inventoryui::Region{1, 1} && visits == "scrt",
          "missing region choices still request all reached callbacks");
    inventoryslots::Inventory inventory;
    const auto itemAt = [](inventoryui::Region where, uint8_t category = 11) {
        nativesaves::Item item{};
        item[0] = category;
        std::memcpy(item.data() + 0x0c, where.data(), sizeof where);
        return item;
    };
    inventory.tabs[0] = {
        {1, itemAt({5, 0})}, {2, itemAt({5, 0})}, {-1, itemAt({-2, 1})}, {0, itemAt({99, 99})}, {7, itemAt({8, 8}, 0)}};
    std::vector<inventoryui::Region> cache{{77, 88}};
    const auto initial = cache;
    check(!inventoryui::refreshRegionCache(false, inventoryui::Panel::Inventory, cache, inventory.tabs, 0, {1, 1}) &&
              cache == initial,
          "unbound widget preserves its previous region choices");
    check(inventoryui::refreshRegionCache(true, inventoryui::Panel::Inventory, cache, inventory.tabs, 0, {1, 1}) &&
              cache == std::vector<inventoryui::Region>{{1, 1}, {-2, 1}, {5, 0}, {8, 8}},
          "cache reads raw canonical rows, excludes only zero, deduplicates and includes player region");
    const auto retained = cache;
    inventory.tabs[0] = {{1, itemAt({44, 1})}};
    check(!inventoryui::refreshRegionCache(true, inventoryui::Panel::Crafting, cache, inventory.tabs, 0, {3, 4}) &&
              !inventoryui::refreshRegionCache(true, inventoryui::Panel::Shop, cache, inventory.tabs, 0, {3, 4}) &&
              cache == retained,
          "other widget refreshes do not recompute the bag's cached choices after carried/player changes");
    check(inventoryui::refreshRegionCache(true, inventoryui::Panel::Inventory, cache, inventory.tabs, 0, {3, 4}) &&
              cache == std::vector<inventoryui::Region>{{3, 4}, {44, 1}},
          "explicit inventory refresh publishes new carried/player regions");
    const auto selected = region;
    check(inventoryui::refreshRegionCache(true, inventoryui::Panel::Inventory, cache, inventory.tabs, 3, {6, 7}) &&
              cache == std::vector<inventoryui::Region>{{6, 7}} && region == selected,
          "valid empty inner tab retains player choice without selected-region repair");
    check(!inventoryui::refreshRegionCache(true, inventoryui::Panel::Inventory, cache, inventory.tabs, -1, {0, 0}) &&
              cache == std::vector<inventoryui::Region>{{6, 7}},
          "engine defers invalid carried headers without claiming native safe access");
    std::cout << "Inventory UI actions: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
