#include "InventorySlots.h"
#include "ItemTypes.h"
#include "NativeInteract.h"
#include "NativeKeyOrder.h"
#include "NativeRules.h"
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
void copyItem(Item &destination, const Item &source) {
    using namespace nativesaves::item;
    destination[TYPE] = source[TYPE];
    // Subtype, seed, region and rarity are contiguous; the following padding is not copied.
    std::memcpy(destination.data() + SUBTYPE, source.data() + SUBTYPE, RARITY - SUBTYPE + 1);
    std::memcpy(destination.data() + FORMULA, source.data() + FORMULA, 4);
    // Material, all 32 attribute entries and their count form the final meaningful span.
    std::memcpy(destination.data() + MATERIAL, source.data() + MATERIAL, ATTRIBUTE_COUNT - MATERIAL + 1);
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

std::optional<Index> Inventory::add(Item item, int32_t quantity, int tab) {
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
    if (empty < 0) {
        empty = (int)list.size();
        list.emplace_back();
    }
    auto &added = list[(size_t)empty];
    added.count = quantity;
    copyItem(added.item, item);
    return Index{tab, empty};
}

void Inventory::takePage(Item item, int32_t quantity) {
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
            stack.count = 0;
            copyItem(stack.item, {});
            if (quantity == 0)
                return;
        }
}
void Inventory::takeCreature(Item item, int32_t quantity) {
    for (auto &tab : tabs)
        for (auto &stack : tab) {
            if (!nativeinteract::sameItem(stack.item.data(), item.data()))
                continue;
            const int32_t used = std::min(quantity, stack.count);
            stack.count = minus(stack.count, used);
            quantity = minus(quantity, used);
            if (stack.count == 0)
                copyItem(stack.item, {});
        }
}

void Inventory::takeCraft(Item item, int32_t quantity) {
    for (auto &tab : tabs)
        for (auto &stack : tab) {
            if (!nativeinteract::sameItem(stack.item.data(), item.data()))
                continue;
            if (quantity < stack.count) {
                stack.count = minus(stack.count, quantity);
                break;
            }
            stack.count = 0;
            copyItem(stack.item, {});
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

int32_t Inventory::quantity(const Item &item) const {
    int32_t count = 0;
    for (const auto &tab : tabs)
        for (const auto &stack : tab)
            if (nativeinteract::sameItem(stack.item.data(), item.data()))
                count = plus(count, stack.count);
    return count;
}

void Inventory::clearType(uint8_t type) {
    for (auto &tab : tabs)
        for (auto &stack : tab)
            if (stack.item[0] == type) {
                copyItem(stack.item, {});
                stack.count = 0;
            }
}

std::optional<Item> Inventory::dropOne(Index source, const uint8_t *actor) {
    const auto *stack = slot(source);
    if (!actor || !stack || stack->count == 0)
        return {};
    const auto &item = stack->item;
    if (item[0] == 22 || item[0] == 23 || item[0] == 24 || (actor[0x68] == 0x56 && nativeinteract::sameItem(item.data(), actor + 0x1d8)))
        return {};
    Item dropped{};
    copyItem(dropped, item);
    consumeOne(source);
    return dropped;
}

PlayerState PlayerState::fromCharacter(const nativesaves::Character &character) {
    PlayerState state;
    state.inventory.tabs = character.inventory;
    state.inventory.gold = character.gold;
    state.inventory.fieldAb0 = character.fieldAb0;
    state.equipment = character.equipment;
    return state;
}

void PlayerState::writeCharacter(nativesaves::Character &character) const {
    character.inventory = inventory.tabs;
    character.gold = inventory.gold;
    character.fieldAb0 = inventory.fieldAb0;
    character.equipment = equipment;
}

bool PlayerState::equip(Index source, int button, bool shift) {
    const Stack *stack = inventory.slot(source);
    if (!stack || !stack->count || (button != 0 && button != 1))
        return false;
    Item incoming{};
    copyItem(incoming, stack->item);
    const int type = incoming[0];
    auto put = [&](int slot) { copyItem(equipment[(size_t)slot], incoming); };
    auto giveBack = [&](int slot) { inventory.add(equipment[(size_t)slot], 1); };
    auto clear = [&](int slot) { copyItem(equipment[(size_t)slot], {}); };
    if (type == 3) {
        inventory.consumeOne(source);
        const auto id = subtype(incoming);
        constexpr int left = 6, right = 7;
        if (id == 13 || id == 14) {
            if (nativerules::twoHandedWeapon(equipment[right].data())) {
                giveBack(right);
                clear(right);
            }
            giveBack(left);
            put(left);
        } else if (id == 6 || id == 7 || nativerules::twoHandedWeapon(incoming.data())) {
            const int destination = (id == 6 || id == 7) && button == 0 ? left : right;
            const int other = destination == left ? right : left;
            giveBack(destination);
            put(destination);
            giveBack(other);
            clear(other);
        } else if (button == 0 && !shift) {
            giveBack(right);
            put(right);
        } else {
            giveBack(left);
            if (nativerules::twoHandedWeapon(equipment[right].data())) {
                giveBack(right);
                clear(right);
            }
            put(left);
        }
        return true;
    }
    int destination = -1;
    switch (type) {
    case 4:
        destination = 2;
        break;
    case 5:
        destination = 4;
        break;
    case 6:
        destination = 3;
        break;
    case 7:
        destination = 5;
        break;
    case 8:
        destination = 1;
        break;
    case 9:
        destination = button == 0 ? 9 : 8;
        break;
    case 20:
        destination = 10;
        break;
    case 26:
        destination = 0;
        break;
    default:
        return false;
    }
    if (type != 20)
        inventory.consumeOne(source);
    giveBack(type == 26 ? 2 : destination);
    put(destination);
    if (type == 20)
        // Keep a checked index across vector growth; no allocator-dependent native dangling-pointer behavior is inferred.
        inventory.consumeOne(source);
    return true;
}

bool PlayerState::unequip(int slot) {
    if (slot < 0 || (size_t)slot >= equipment.size() || !equipment[(size_t)slot][0])
        return false;
    inventory.add(equipment[(size_t)slot], 1);
    auto &item = equipment[(size_t)slot];
    item[0] = 0;
    const uint32_t zero = 0;
    std::memcpy(item.data() + nativesaves::item::SUBTYPE, &zero, 4);
    return true;
}

UseResult use(PlayerState &state, Index source, uint8_t *actor, int button, bool shift, float abilityDuration) {
    const auto *stack = state.inventory.slot(source);
    if (!actor || !stack || stack->count == 0 || (button != 0 && button != 1))
        return {};
    if (!nativerules::mayUse(actor, stack->item.data()))
        return {Action::Refused};
    Item incoming{};
    copyItem(incoming, stack->item);
    if (state.equip(source, button, shift))
        return {incoming[0] == 3 ? Action::WeaponEquipped : Action::Equipped};
    auto readFloat = [&](size_t at) {
        float value;
        std::memcpy(&value, actor + at, 4);
        return value;
    };
    auto writeFloat = [&](size_t at, float value) { std::memcpy(actor + at, &value, 4); };
    auto active = [&] {
        Item item;
        std::memcpy(item.data(), actor + 0x1d8, item.size());
        copyItem(item, incoming);
        std::memcpy(actor + 0x1d8, item.data(), item.size());
    };
    if (incoming[0] == 21) {
        if (actor[0x68] != 0 && !(readFloat(0x6c) > abilityDuration))
            return {};
        active();
        actor[0x68] = 0x56;
        writeFloat(0x6c, 0);
        return {Action::PetFood};
    }
    return useItem(state, source, actor);
}

UseResult useItem(PlayerState &state, Index source, uint8_t *actor) {
    const auto *stack = state.inventory.slot(source);
    if (!actor || !stack || stack->count == 0)
        return {};
    Item incoming{};
    copyItem(incoming, stack->item);
    auto readFloat = [&](size_t at) {
        float value;
        std::memcpy(&value, actor + at, 4);
        return value;
    };
    auto writeFloat = [&](size_t at, float value) { std::memcpy(actor + at, &value, 4); };
    auto active = [&] {
        Item item;
        std::memcpy(item.data(), actor + 0x1d8, item.size());
        copyItem(item, incoming);
        std::memcpy(actor + 0x1d8, item.data(), item.size());
    };
    if (!(readFloat(0x130) <= 0))
        return {};
    const auto id = subtype(incoming);
    if (incoming[0] == 1 || incoming[0] == 28) {
        if (incoming[0] == 1 && (id == 9 || id == 13)) {
            if (id == 9)
                state.inventory.consumeOne(source);
            return {id == 13 ? Action::Map : Action::Used, incoming};
        }
        active();
        actor[0x68] = incoming[0] == 28 || id == 1 || id == 2 || id == 3 || id == 12 ? 0x54 : 0x55;
        writeFloat(0x6c, 0);
        writeFloat(0x12c, 0);
        state.inventory.consumeOne(source);
        return {Action::Used};
    }
    if (incoming[0] == 24) {
        if (std::memcmp(incoming.data() + nativesaves::item::REGION, actor + 0x150, 8) != 0)
            return {Action::WrongRegion};
        active();
        const uint8_t ability = id == 1 ? 0x78 : id == 2 ? 0x79 : 0x89;
        if (id <= 2 && id >= 1 && actor[0x68] == ability)
            actor[0x68] = 0;
        else {
            actor[0x68] = ability;
            writeFloat(0x6c, 0);
        }
        return {Action::Used};
    }
    return {};
}

float adjustHealth(uint8_t *actor, float oldMaximum, const std::function<float()> &maximum) {
    const float ratio = maximum() / oldMaximum;
    float health;
    std::memcpy(&health, actor + 0x180, 4);
    health = ratio * health;
    std::memcpy(actor + 0x180, &health, 4);
    const float cap = maximum();
    std::memcpy(&health, actor + 0x180, 4);
    if (cap < health) {
        health = cap;
        std::memcpy(actor + 0x180, &health, 4);
    }
    return health;
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
