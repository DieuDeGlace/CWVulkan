#pragma once
// Layout and ordering from docs/specs/steam-dungeon-spawn-guard-trees.md.
#include "CaptureMemory.h"
#include <array>
#include <utility>
#include <vector>

namespace captureabi {
using SpawnCell = std::array<int32_t, 2>;
inline constexpr size_t MaxSpawnCells = 256; // Diagnostic cap, not a native limit.
inline constexpr size_t MaxZoneSpawnRecords = 1024;
inline constexpr size_t MaxZoneSpawnCells = 4096;

struct SpawnCellTrees {
    std::vector<SpawnCell> guards;
    std::vector<SpawnCell> activation;
};

struct SpawnCellRecord {
    int64_t id = 0;
    SpawnCellTrees cells;
};

namespace spawn_tree {
struct Links {
    uintptr_t left, parent, right;
    uint8_t colour, nil;
    bool operator==(const Links &other) const {
        return left == other.left && parent == other.parent && right == other.right &&
               colour == other.colour && nil == other.nil;
    }
};
static_assert(offsetof(Links, nil) == 0x19);
inline constexpr size_t LinkBytes = offsetof(Links, nil) + 1;

// The caller keeps the native owner alive and unmodified while walking it.
template <class Read, class Visit>
bool visit(uintptr_t owner, size_t offset, size_t limit, Read &read, Visit visitNode) {
    std::array<uintptr_t, 2> header{};
    if (!readField(owner, offset, header.data(), sizeof header, read) || !header[0] || header[1] > limit)
        return false;
    const auto [head, count] = header;
    Links ends{};
    if (!readField(head, 0, &ends, LinkBytes, read) || ends.nil != 1 ||
        !ends.left || !ends.parent || !ends.right)
        return false;

    struct Frame {
        uintptr_t address;
        Links links;
    };
    std::vector<Frame> pending;
    auto next = ends.parent, parent = head;
    auto last = head;
    size_t entered = 0, emitted = 0;
    while (next != head || !pending.empty()) {
        while (next != head) {
            Links links{};
            if (++entered > count || !readField(next, 0, &links, LinkBytes, read) ||
                links.nil || links.parent != parent || !links.left || !links.right)
                return false;
            pending.push_back({next, links});
            parent = next;
            next = links.left;
        }
        const auto [node, links] = pending.back();
        pending.pop_back();
        if ((!emitted && node != ends.left) || !visitNode(node))
            return false;
        ++emitted;
        last = node;
        parent = node;
        next = links.right;
    }
    if (emitted != count || last != ends.right || (!count && ends.left != head))
        return false;

    std::array<uintptr_t, 2> currentHeader{};
    Links currentEnds{};
    return readField(owner, offset, currentHeader.data(), sizeof currentHeader, read) && currentHeader == header &&
           readField(head, 0, &currentEnds, LinkBytes, read) && currentEnds == ends;
}
} // namespace spawn_tree

template <class Read>
bool readSpawnCellTrees(uintptr_t spawn, SpawnCellTrees &output, Read read) {
    const auto copy = [&](size_t offset, std::vector<SpawnCell> &cells) {
        return spawn_tree::visit(spawn, offset, MaxSpawnCells, read, [&](uintptr_t node) {
            SpawnCell cell{};
            if (!readField(node, 0x1c, cell.data(), sizeof cell, read) || (!cells.empty() && !(cells.back() < cell)))
                return false;
            cells.push_back(cell);
            return true;
        });
    };
    SpawnCellTrees trees;
    if (!copy(0x900, trees.guards) || !copy(0x928, trees.activation))
        return false;
    output = std::move(trees);
    return true;
}

template <class Read>
bool readZoneSpawnCells(uintptr_t zone, std::vector<SpawnCellRecord> &output, Read read) {
    std::vector<SpawnCellRecord> records;
    size_t cellCount = 0;
    if (!spawn_tree::visit(zone, 0x460, MaxZoneSpawnRecords, read, [&](uintptr_t node) {
            SpawnCellRecord record;
            uintptr_t spawn = 0;
            int64_t spawnId = 0;
            if (!readField(node, 0x20, &record.id, sizeof record.id, read) ||
                (!records.empty() && record.id <= records.back().id) ||
                !readField(node, 0x28, &spawn, sizeof spawn, read) ||
                !readField(spawn, 0x38, &spawnId, sizeof spawnId, read) || spawnId != record.id ||
                !readSpawnCellTrees(spawn, record.cells, read))
                return false;
            cellCount += record.cells.guards.size() + record.cells.activation.size();
            if (cellCount > MaxZoneSpawnCells)
                return false;
            records.push_back(std::move(record));
            return true;
        }))
        return false;
    output = std::move(records);
    return true;
}
} // namespace captureabi
