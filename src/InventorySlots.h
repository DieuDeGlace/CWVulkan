#pragma once
// Complete player inventory records and live slot operations. Loading never
// reinserts/recategorizes slots; refresh explicitly mutates their actual order.
#include "NativeSaves.h"
#include <functional>
#include <optional>
#include <vector>

namespace inventoryslots {
using Item = nativesaves::Item;
using Stack = nativesaves::Stack;
using Tabs = std::vector<std::vector<Stack>>;
struct Index {
    int tab = -1, slot = -1;
    bool operator==(const Index &) const = default;
};
struct SortContext {
    nativesaves::Cell selectedRegion{};
    // These use the actual local player/reference region, not selectedRegion.
    std::function<bool(const Item &)> usable;
    std::function<float(const Item &)> power;
};
struct Inventory {
    Tabs tabs = Tabs(nativesaves::INVENTORY_TABS);
    int32_t gold = 0, fieldAb0 = 0;
    Stack cursor; // transient state; no inferred UI grab producer or save field

    void initializeKitTabs(); // changes tabs only; six lengths 10,10,10,30,30,0
    void clearTabs();         // the native tab-only reset retains cursor/counters
    Stack *slot(Index);
    const Stack *slot(Index) const;
    // Own the incoming value while insertion grows vectors or removal clears its source slot.
    std::optional<Index> add(Item, int32_t quantity, int tab = -1);
    void takePage(Item, int32_t quantity);
    void takeCreature(Item, int32_t quantity);
    bool consumeOne(Index); // direct UI clearing retains other item bytes
    // Returns false without mutation when tab0 needs unavailable stat queries.
    bool refresh(const SortContext &);

  private:
    bool contains(Index) const;
};
} // namespace inventoryslots
