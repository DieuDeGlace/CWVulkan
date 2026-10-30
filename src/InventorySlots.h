#pragma once
// Complete player inventory records and live slot operations. Loading never
// reinserts/recategorizes slots; refresh explicitly mutates their actual order.
#include "NativeSaves.h"
#include <array>
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
    void takeCraft(Item, int32_t quantity); // crafting keeps the original request for each matching stack/tab
    bool consumeOne(Index); // direct UI clearing retains other item bytes
    int32_t quantity(const Item &) const;
    void clearType(uint8_t);
    std::optional<Item> dropOne(Index, const uint8_t *actor);
    // Returns false without mutation when tab0 needs unavailable stat queries.
    bool refresh(const SortContext &);

  private:
    bool contains(Index) const;
};
struct PlayerState {
    Inventory inventory;
    std::array<Item, nativesaves::EQUIPMENT_SLOTS> equipment{};
    static PlayerState fromCharacter(const nativesaves::Character &);
    void writeCharacter(nativesaves::Character &) const;
    // Equipment returns use ordinary insertion, rather than swapping into the source slot.
    bool equip(Index, int button, bool shift);
    bool unequip(int slot);
};
enum class Action { None, Refused, Equipped, WeaponEquipped, Used, PetFood, Map, WrongRegion };
struct UseResult {
    Action action = Action::None;
    std::optional<Item> interaction;
};
// Reached ordinary inventory down, after shop/identification/customization routing.
// actor is a live, full creature record; refresh/layout and the common HP/UI tail belong to the caller.
UseResult use(PlayerState &, Index, uint8_t *actor, int button, bool shift, float abilityDuration);
// Two ordered maximum queries: store the rescaled HP before the second callback/cap.
float adjustHealth(uint8_t *actor, float oldMaximum, const std::function<float()> &maximum);
} // namespace inventoryslots
