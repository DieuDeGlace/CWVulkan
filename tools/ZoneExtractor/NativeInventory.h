#pragma once
// Native layout: steam-inventory-slot-lifecycle.md section1. No SDK overlays.
#include "CaptureMemory.h"
#include "NativeItemRecord.h"
#include <utility>
#include <vector>

namespace captureabi {
constexpr size_t INVENTORY_OFFSET = 0x9f0;
constexpr size_t STACK_BYTES = 164;
constexpr size_t MAX_CAPTURE_TABS = 256;      // Capture policy; six is only the native default.
constexpr size_t MAX_CAPTURE_STACKS = 262144; // Combined budget across all tabs.
struct InventoryStack {
    std::array<uint8_t, STACK_BYTES> bytes;
    int32_t quantity() const {
        int32_t value;
        std::memcpy(&value, bytes.data(), sizeof value);
        return value;
    }
    ItemRecord item() const { return ItemRecord::copy(bytes.data() + 4); }
};
static_assert(sizeof(InventoryStack) == STACK_BYTES);
using Inventory = std::vector<std::vector<InventoryStack>>;

inline bool inventorySpan(const std::array<uintptr_t, 3> &span, size_t stride, size_t alignment, size_t budget,
                          size_t &count) {
    count = 0;
    const auto begin = span[0], end = span[1], capacityEnd = span[2];
    if ((!begin && (end || capacityEnd)) || begin > end || end > capacityEnd ||
        ((begin | end | capacityEnd) & (alignment - 1)) || (end - begin) % stride || (capacityEnd - begin) % stride)
        return false;
    count = (end - begin) / stride;
    return count <= budget;
}

// Produces owned raw stack values, retaining empty slots, negative quantities,
// tab order, duplicate items and padding. Caller retains creature and buffer
// lifetimes and their applicable protection through capture. Header rechecks
// reject changed ranges; they are not pins or an atomic in-place item snapshot.
template <class Read> bool readInventory(uintptr_t creature, Inventory &output, Read read) {
    output.clear();
    std::array<uintptr_t, 3> outer;
    size_t tabs = 0;
    if (!readField(creature, INVENTORY_OFFSET, outer.data(), sizeof outer, read) ||
        !inventorySpan(outer, 24, 8, MAX_CAPTURE_TABS, tabs))
        return false;
    std::vector<std::array<uintptr_t, 3>> bounds(tabs);
    if (tabs && !readField(outer[0], 0, bounds.data(), tabs * 24, read))
        return false;
    Inventory result(tabs);
    size_t total = 0;
    for (size_t tab = 0; tab < tabs; ++tab) {
        size_t stacks = 0;
        if (!inventorySpan(bounds[tab], STACK_BYTES, 4, MAX_CAPTURE_STACKS - total, stacks))
            return false;
        total += stacks;
        result[tab].resize(stacks);
        if (stacks && !readField(bounds[tab][0], 0, result[tab].data(), stacks * STACK_BYTES, read))
            return false;
    }
    std::array<uintptr_t, 3> current;
    if (!readField(creature, INVENTORY_OFFSET, current.data(), sizeof current, read) || current != outer)
        return false;
    for (size_t tab = 0; tab < tabs; ++tab) {
        if (!readField(outer[0], tab * 24, current.data(), sizeof current, read) || current != bounds[tab])
            return false;
    }
    output = std::move(result);
    return true;
}

inline const InventoryStack *inventorySlot(const Inventory &inventory, int tab, int slot) {
    if (tab < 0 || slot < 0 || size_t(tab) >= inventory.size() || size_t(slot) >= inventory[size_t(tab)].size())
        return nullptr;
    return &inventory[size_t(tab)][size_t(slot)];
}
} // namespace captureabi
