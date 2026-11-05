#include "CraftingCollection.h"
#include <utility>

namespace gamecraft {
bool Collection::rebuild(const inventoryslots::Inventory &carried, const RecipeGroups &recipes,
                         inventoryslots::Index selected) {
    if (carried.tabs.empty())
        return false;
    if (rows.size() < 7)
        rows.resize(7);
    const auto held = [&](const Thing &item) { return carried.quantity(toRecord(item)); };
    for (int tab = 0; tab < TABS; ++tab) {
        auto &group = rows[(size_t)tab];
        group.clear();
        for (const auto &recipe : recipes[(size_t)tab]) {
            const size_t slot = group.size();
            group.push_back({times(recipe, held), toRecord(recipe.result)});
            if (selected.tab == tab && selected.slot >= 0 && (size_t)selected.slot == slot)
                button = SelectedRecipe{selected, recipe};
        }
    }
    rows[6].clear();
    return true;
}
bool Collection::publish(inventoryslots::Index selected,
                         const std::function<std::optional<Formula>(const ItemRecord &, bool)> &query) {
    if (selected.tab >= 0 && (size_t)selected.tab < rows.size() && selected.slot >= 0 &&
        (size_t)selected.slot < rows[(size_t)selected.tab].size()) {
        if (auto recipe = query(rows[(size_t)selected.tab][(size_t)selected.slot].item, selected.tab == 6)) {
            button = SelectedRecipe{selected, std::move(*recipe)};
            return true;
        }
    }
    button.reset();
    return false;
}
} // namespace gamecraft
