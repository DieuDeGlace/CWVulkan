#include "InventorySlots.h"
#include "ItemTypes.h"
#include "NativeInteract.h"
#include "NativeKeyOrder.h"
#include <algorithm>
#include <bit>
#include <cstring>

namespace inventoryslots {
namespace {
int32_t plus(int32_t a, int32_t b) {
    return std::bit_cast<int32_t>((uint32_t)a + (uint32_t)b);
}
int32_t minus(int32_t a, int32_t b) {
    return std::bit_cast<int32_t>((uint32_t)a - (uint32_t)b);
}
uint32_t subtype(const Item &item) {
    uint32_t value;
    std::memcpy(&value, item.data() + nativesaves::item::SUBTYPE, 4);
    return value;
}
struct SortKey {
    bool empty = false, here = false, usable = false;
    int family = 2;
    float power = 0;
    uint8_t type = 0;
    uint32_t subtype = 0;
    bool operator<(const SortKey &right) const {
        // The native predicate intentionally says true for two empty slots.
        if (right.empty)
            return true;
        if (empty)
            return false;
        if (family < 2 && here != right.here)
            return here;
        if (family == 0) {
            if (usable != right.usable)
                return usable;
            return power > right.power;
        }
        return type != right.type ? type < right.type : subtype < right.subtype;
    }
};
} // namespace

void Inventory::initializeKitTabs() {
    tabs.assign(nativesaves::INVENTORY_TABS, {});
    for (int tab : {0, 1, 2})
        tabs[(size_t)tab].resize(10);
    for (int tab : {3, 4})
        tabs[(size_t)tab].resize(30);
}
void Inventory::clearTabs() {
    tabs.clear();
}
bool Inventory::contains(Index index) const {
    return index.tab >= 0 && (size_t)index.tab < tabs.size() && index.slot >= 0 && (size_t)index.slot < tabs[(size_t)index.tab].size();
}
Stack *Inventory::slot(Index index) {
    return contains(index) ? &tabs[(size_t)index.tab][(size_t)index.slot] : nullptr;
}
const Stack *Inventory::slot(Index index) const {
    return contains(index) ? &tabs[(size_t)index.tab][(size_t)index.slot] : nullptr;
}

std::optional<Index> Inventory::add(const Item &item, int32_t quantity, int tab) {
    if (quantity == 0 || item[0] == 0 || tab < -1)
        return {};
    if (tab == -1)
        tab = itemtypes::inventoryTab(item[0]);
    if ((size_t)tab >= tabs.size())
        tabs.resize((size_t)tab + 1);
    if (item[0] == 12 && item[nativesaves::item::MATERIAL] == 11) {
        gold = plus(gold, quantity);
        return {};
    }
    if (item[0] == 14) {
        fieldAb0 = plus(fieldAb0, quantity);
        return {};
    }
    auto &list = tabs[(size_t)tab];
    int empty = -1;
    for (size_t index = 0; index < list.size(); ++index) {
        auto &stack = list[index];
        if (stack.count == 0 && empty < 0)
            empty = (int)index;
        if (itemtypes::stackable(item[0]) && nativeinteract::sameItem(stack.item.data(), item.data())) {
            stack.count = quantity == -1 ? -1 : plus(stack.count, quantity);
            return Index{tab, (int)index};
        }
    }
    if (empty >= 0)
        list[(size_t)empty] = {quantity, item};
    else {
        empty = (int)list.size();
        list.push_back({quantity, item});
    }
    return Index{tab, empty};
}

void Inventory::takePage(const Item &item, int32_t quantity) {
    if (quantity == 0 || item[0] == 0)
        return;
    if (item[0] == 12 && item[nativesaves::item::MATERIAL] == 11) {
        gold = std::max(0, minus(gold, quantity));
        return;
    }
    for (auto &tab : tabs)
        for (auto &stack : tab) {
            if (stack.count <= 0 || !nativeinteract::sameItem(stack.item.data(), item.data()))
                continue;
            if (stack.count > quantity) {
                stack.count = minus(stack.count, quantity);
                return;
            }
            quantity = minus(quantity, stack.count);
            stack = {};
            if (quantity == 0)
                return;
        }
}
void Inventory::takeCreature(const Item &item, int32_t quantity) {
    for (auto &tab : tabs)
        for (auto &stack : tab) {
            if (!nativeinteract::sameItem(stack.item.data(), item.data()))
                continue;
            const int32_t used = std::min(quantity, stack.count);
            stack.count = minus(stack.count, used);
            quantity = minus(quantity, used);
            if (stack.count == 0)
                stack.item = {};
        }
}

bool Inventory::consumeOne(Index index) {
    auto *stack = slot(index);
    if (!stack || stack->count == 0)
        return false;
    stack->count = minus(stack->count, 1);
    if (stack->count < 1) {
        stack->count = 0;
        stack->item[0] = 0;
        const uint32_t zero = 0;
        std::memcpy(stack->item.data() + nativesaves::item::SUBTYPE, &zero, 4);
    }
    return true;
}

bool Inventory::refresh(const SortContext &context) {
    if (!tabs.empty() && (!context.usable || !context.power))
        for (const auto &stack : tabs[0])
            if (stack.count != 0)
                return false;
    for (size_t tab = 0; tab < tabs.size(); ++tab) {
        auto key = [&](const Stack &stack) {
            SortKey result;
            result.empty = stack.count == 0;
            result.family = (int)std::min<size_t>(tab, 2);
            if (result.empty)
                return result;
            result.type = stack.item[0];
            result.subtype = subtype(stack.item);
            nativesaves::Cell region;
            std::memcpy(region.data(), stack.item.data() + nativesaves::item::REGION, 8);
            result.here = region == context.selectedRegion;
            if (tab == 0) {
                result.usable = context.usable(stack.item);
                result.power = context.power(stack.item);
            }
            return result;
        };
        keyorder::sort(tabs[tab], key);
        const auto count = std::count_if(tabs[tab].begin(), tabs[tab].end(), [](const Stack &stack) { return stack.count != 0; });
        tabs[tab].resize((size_t)count);
    }
    return true;
}
} // namespace inventoryslots
