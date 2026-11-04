#include "../tools/ZoneExtractor/NativeInventory.h"
#include <iostream>
#include <map>

namespace {
struct Memory {
    std::map<uintptr_t, std::vector<uint8_t>> blocks;
    template <class T> void store(uintptr_t at, const T &value) {
        auto &bytes = blocks[at];
        bytes.resize(sizeof value);
        std::memcpy(bytes.data(), &value, sizeof value);
    }
    bool read(uintptr_t at, void *output, size_t count) const {
        auto found = blocks.upper_bound(at);
        if (found == blocks.begin())
            return false;
        --found;
        const auto offset = at - found->first;
        if (offset > found->second.size() || count > found->second.size() - offset)
            return false;
        std::memcpy(output, found->second.data() + offset, count);
        return true;
    }
};
} // namespace

int nativeInventoryTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL inventory capture: " << label << '\n';
            ++failures;
        }
    };
    constexpr uintptr_t creature = 0x100000, tabs = 0x200000, slots = 0x300004;
    Memory memory;
    std::array<std::array<uintptr_t, 3>, 7> tabBounds{};
    std::array<captureabi::InventoryStack, 3> native;
    for (size_t i = 0; i < native.size(); ++i) {
        native[i].bytes.fill(uint8_t(0x80 + i));
        const int32_t quantity = i == 0 ? 0 : i == 1 ? -1 : INT32_MIN;
        std::memcpy(native[i].bytes.data(), &quantity, 4);
        native[i].bytes[4] = 3; // Even the empty slot retains an equippable stale item.
        const int32_t subtype = int32_t(i + 17);
        std::memcpy(native[i].bytes.data() + 8, &subtype, 4);
    }
    const auto fixture = [&] {
        memory.blocks.clear();
        tabBounds = {};
        tabBounds[0] = {slots, slots + sizeof native, slots + sizeof native + 164};
        tabBounds[6] = {slots + 0x1000, slots + 0x1000, slots + 0x1000 + 164};
        memory.store(creature + 0x9f0,
                     std::array<uintptr_t, 3>{tabs, tabs + sizeof tabBounds, tabs + sizeof tabBounds + 24});
        memory.store(tabs, tabBounds);
        memory.store(slots, native);
    };
    const auto read = [&](uintptr_t at, void *out, size_t count) { return memory.read(at, out, count); };
    captureabi::Inventory output;
    const auto capture = [&] { return captureabi::readInventory(creature, output, read); };
    fixture();
    check(capture() && output.size() == 7 && output[0].size() == 3 && output[6].empty() &&
              output[0][0].quantity() == 0 && output[0][1].quantity() == -1 && output[0][2].quantity() == INT32_MIN &&
              std::memcmp(output[0].data(), native.data(), sizeof native) == 0,
          "seven ordered tabs, empty slots, signed quantities and all 164 raw bytes survive; capacity is not read");
    const auto item = output[0][1].item();
    check(item.category() == 3 && item.subtype() == 18 &&
              std::memcmp(item.bytes.data(), native[1].bytes.data() + 4, 160) == 0,
          "item begins at slot+4 without an eight-byte alignment/padding overlay");
    check(captureabi::inventorySlot(output, 0, 0) && !captureabi::inventorySlot(output, -1, 0) &&
              !captureabi::inventorySlot(output, 7, 0) && !captureabi::inventorySlot(output, 6, 0) &&
              !captureabi::inventorySlot(output, 0, -1) && !captureabi::inventorySlot(output, 0, 3),
          "physical slot lookup retains empties and rejects stale/negative/out-of-range indices");
    const std::array<std::array<uintptr_t, 3>, 5> badSlots{{{0, slots, slots},
                                                            {slots + 164, slots, slots + 328},
                                                            {slots, slots + 328, slots + 164},
                                                            {slots, slots + 165, slots + 328},
                                                            {slots + 1, slots + 165, slots + 329}}};
    for (const auto &bad : badSlots) {
        fixture();
        tabBounds[1] = bad;
        memory.store(tabs, tabBounds);
        check(!capture() && output.empty(), "malformed later tab rejects the entire capture without partial output");
    }
    fixture();
    memory.blocks.erase(slots);
    check(!capture() && output.empty(), "unreadable slot data clears prior owned capture");
    fixture();
    memory.store(creature + 0x9f0, std::array<uintptr_t, 3>{tabs, tabs + 25, tabs + 48});
    check(!capture(), "outer tab vector requires a 24-byte stride");
    fixture();
    memory.store(creature + 0x9f0, std::array<uintptr_t, 3>{tabs, tabs + 24 * (captureabi::MAX_CAPTURE_TABS + 1),
                                                            tabs + 24 * (captureabi::MAX_CAPTURE_TABS + 1)});
    check(!capture(), "tab budget is a separate bounded capture policy");
    fixture();
    const auto changed = [&](uintptr_t at, void *out, size_t count) {
        const bool copied = memory.read(at, out, count);
        if (at == slots) {
            tabBounds[0][1] -= 164;
            memory.store(tabs, tabBounds);
        }
        return copied;
    };
    check(!captureabi::readInventory(creature, output, changed) && output.empty(),
          "changed inner range rejects an otherwise readable owned copy");
    fixture();
    const auto changedOuter = [&](uintptr_t at, void *out, size_t count) {
        const bool copied = memory.read(at, out, count);
        if (at == slots)
            memory.store(creature + 0x9f0, std::array<uintptr_t, 3>{tabs, tabs, tabs + sizeof tabBounds});
        return copied;
    };
    check(!captureabi::readInventory(creature, output, changedOuter) && output.empty(),
          "changed outer range cannot publish obsolete tabs");
    fixture();
    tabBounds[0] = {slots, slots + 164 * captureabi::MAX_CAPTURE_STACKS, slots + 164 * captureabi::MAX_CAPTURE_STACKS};
    tabBounds[1] = {slots + 0x100, slots + 0x100 + 164, slots + 0x100 + 164};
    memory.store(tabs, tabBounds);
    const auto largeRead = [&](uintptr_t at, void *out, size_t count) {
        if (at == slots) {
            std::memset(out, 0, count);
            return true;
        }
        return memory.read(at, out, count);
    };
    check(!captureabi::readInventory(creature, output, largeRead) && output.empty(),
          "combined slot budget is enforced across tabs rather than per allocation");
    fixture();
    memory.store(creature + 0x9f0, std::array<uintptr_t, 3>{0, 0, 0});
    check(capture() && output.empty(), "readable constructed empty inventory remains a valid value");
    fixture();
    const auto maximum = std::numeric_limits<uintptr_t>::max();
    check(!captureabi::readInventory(maximum, output, read) && output.empty(), "creature owner arithmetic never wraps");
    memory.store(creature + 0x9f0, std::array<uintptr_t, 3>{maximum - 7, maximum - 7, maximum - 7});
    check(capture() && output.empty(), "an allocated empty outer range does not dereference its capacity");
    std::cout << "Native inventory capture: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
