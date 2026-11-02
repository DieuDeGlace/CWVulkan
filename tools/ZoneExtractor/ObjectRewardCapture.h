#pragma once
// Read-only native-memory adapter, independently implemented from
// docs/specs/steam-static-object-reward-map-capture.md. No native routines are called.
#include "../../src/StaticObjectRewards.h"
#include <limits>
#include <set>
#include <utility>

namespace rewardcapture {
using Node = std::array<uint8_t, 0xc0>;
template <class T> T field(const Node &node, size_t at) {
    T value;
    std::memcpy(&value, node.data() + at, sizeof value);
    return value;
}

// Read(address, destination, bytes) must fail rather than dereference unavailable memory.
// A repeated consistent observation is a capture check, not an atomic snapshot protocol.
template <class Read> std::optional<objectrewards::Map> read(uint64_t zone, Read copy) {
    if (!zone || zone > std::numeric_limits<uint64_t>::max() - 0x460)
        return {};
    std::array<uint64_t, 2> wrapper;
    if (!copy(zone + 0x450, wrapper.data(), sizeof wrapper))
        return {};
    const uint64_t head = wrapper[0], count = wrapper[1];
    constexpr uint64_t MAX_CAPTURE_ENTRIES = 100000; // capture resource limit, not a gameplay rule
    if (!head || count > MAX_CAPTURE_ENTRIES)
        return {};
    const auto readNode = [&](uint64_t address, Node &node) {
        return address && address <= std::numeric_limits<uint64_t>::max() - node.size() && copy(address, node.data(), node.size());
    };
    Node header;
    if (!readNode(head, header) || header[0x18] != 1 || header[0x19] != 1)
        return {};
    const uint64_t minimum = field<uint64_t>(header, 0), root = field<uint64_t>(header, 8), maximum = field<uint64_t>(header, 16);
    if (!count && (minimum != head || root != head || maximum != head))
        return {};
    if (count && (minimum == head || root == head || maximum == head))
        return {};

    struct Entry {
        uint64_t address;
        Node bytes;
    };
    std::vector<Entry> stack, observed;
    std::set<uint64_t> visited;
    objectrewards::Map rewards;
    uint64_t current = root, parent = head, first = head, last = head;
    std::optional<int32_t> previous;
    while (current != head || !stack.empty()) {
        while (current != head) {
            if (visited.size() >= count || !visited.insert(current).second)
                return {};
            Entry entry{current, {}};
            if (!readNode(current, entry.bytes) || entry.bytes[0x19] != 0 || entry.bytes[0x18] > 1 ||
                field<uint64_t>(entry.bytes, 8) != parent)
                return {};
            stack.push_back(entry);
            parent = current;
            current = field<uint64_t>(entry.bytes, 0);
        }
        Entry entry = std::move(stack.back());
        stack.pop_back();
        const int32_t key = field<int32_t>(entry.bytes, 0x1c);
        if (previous && key <= *previous)
            return {};
        objectrewards::Item item;
        std::memcpy(item.data(), entry.bytes.data() + 0x20, item.size());
        rewards.emplace(key, item);
        if (!previous)
            first = entry.address;
        previous = key;
        last = entry.address;
        parent = entry.address;
        current = field<uint64_t>(entry.bytes, 16);
        observed.push_back(std::move(entry));
    }
    if (rewards.size() != count || first != minimum || last != maximum)
        return {};

    // Detect visible structural/value changes and unavailable allocations. This cannot
    // certify quiescence against all concurrent mutations; the ABI establishes no lock.
    for (const auto &entry : observed) {
        Node now;
        if (!readNode(entry.address, now) || std::memcmp(now.data(), entry.bytes.data(), 0x1a) ||
            std::memcmp(now.data() + 0x1c, entry.bytes.data() + 0x1c, 0xa4))
            return {};
    }
    Node finalHeader;
    std::array<uint64_t, 2> finalWrapper;
    if (!copy(zone + 0x450, finalWrapper.data(), sizeof finalWrapper) || finalWrapper != wrapper || !readNode(head, finalHeader) ||
        std::memcmp(header.data(), finalHeader.data(), 0x1a))
        return {};
    return rewards;
}
} // namespace rewardcapture
