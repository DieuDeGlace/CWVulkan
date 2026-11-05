#pragma once
// Retained shop rows (docs/specs/steam-shop-collection-rebuild.md).
#include "GameShop.h"
#include "InventorySlots.h"

namespace gameshop {
struct Collection {
    inventoryslots::Tabs rows;

    // Release previous inner buffers before rebuilding exactly two groups. Stock
    // resolution/generation belongs to the caller and is never requested for tab1.
    void rebuild(int selectedTab, const BuyBack &history, const std::function<std::vector<Offer>()> &stock) {
        rows.clear();
        rows.resize(2);
        if (selectedTab == 1) {
            for (auto entry = history.rbegin(); entry != history.rend(); ++entry)
                rows[1].push_back({entry->second, entry->first});
        } else {
            for (const auto &offer : stock())
                inventoryslots::insertRow(rows[0], offer.item, offer.count);
        }
    }
};
} // namespace gameshop
