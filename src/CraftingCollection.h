#pragma once
// Retained count/item groups and the independently copied selected recipe.
// docs/specs/steam-crafting-collection-rebuild.md; generation is supplied by the caller.
#include "GameCraft.h"
#include "InventorySlots.h"
#include <optional>

namespace gamecraft {
using RecipeGroups = std::array<std::vector<Formula>, TABS>;
struct SelectedRecipe {
    inventoryslots::Index source;
    Formula formula;
};
struct Collection {
    inventoryslots::Tabs rows;
    std::optional<SelectedRecipe> button;

    // An empty carried outer vector skips the rebuild without clearing old state.
    // Rebuild groups0..5, clear logical group6, and retain any higher groups.
    bool rebuild(const inventoryslots::Inventory &, const RecipeGroups &, inventoryslots::Index selected);
    // The later left-down tail publishes even without a newly adopted hover.
    // Invalid bounds or a failed query reset the details recipe, not list selection.
    bool publish(inventoryslots::Index, const std::function<std::optional<Formula>(const ItemRecord &, bool tabSix)> &);
};
} // namespace gamecraft
