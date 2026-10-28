#pragma once
// The creature record delta of packet 3 (docs/specs/netcode-packets.md §3).
//
// A creature travels as its game-layout record (nativepose::CREATURE_SIZE bytes), or rather the
// part of it from +0x10 to +0x980 that the game copies into its "last sent" snapshots (0x3b7a0).
// That part is cut into 49 fields. A 64-bit mask says which fields follow, in bit order, each as
// its raw bytes. A sender includes a field when its "changed" test against the copy it last sent
// to that receiver says so, or every field for a full update (a creature new to that receiver,
// after packet 4, after connecting, after a world reset).
//
// Field offsets below are creature record offsets, as the creature specs give them.
#include "net/Wire.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace cwnet {

// The snapshot: creature record bytes [SNAPSHOT_BEGIN, SNAPSHOT_END).
constexpr size_t SNAPSHOT_BEGIN = 0x10;
constexpr size_t SNAPSHOT_END = 0x980;
constexpr size_t SNAPSHOT_SIZE = SNAPSHOT_END - SNAPSHOT_BEGIN;

constexpr int CREATURE_FIELD_COUNT = 49;
constexpr uint64_t ALL_CREATURE_FIELDS = (1ull << CREATURE_FIELD_COUNT) - 1;

struct CreatureSnapshot {
    std::array<uint8_t, SNAPSHOT_SIZE> bytes{};

    // Copies the snapshot part out of / into a whole creature record.
    static CreatureSnapshot of(const uint8_t *record);
    void writeTo(uint8_t *record) const;
    // The byte at a creature record offset (SNAPSHOT_BEGIN <= offset < SNAPSHOT_END).
    uint8_t *at(size_t recordOffset) { return bytes.data() + (recordOffset - SNAPSHOT_BEGIN); }
    const uint8_t *at(size_t recordOffset) const { return bytes.data() + (recordOffset - SNAPSHOT_BEGIN); }
};

// How a field's "changed" test compares it.
enum class FieldTest {
    Bytes,      // any byte differs (integers, bytes, ids, pairs and triples of integers)
    Floats,     // any of its f32 differs as a float (+0 equals -0, a NaN always differs)
    Appearance, // the appearance block's own list of compared parts
    Item,       // item equality (zone-object-interaction.md §0.7)
    Equipment,  // any of the eleven slots unequal by item equality
    Name,       // the text up to its terminator
};

struct CreatureField {
    int bit;
    size_t offset; // creature record offset
    size_t size;   // bytes on the wire
    FieldTest test;
    const char *meaning;
};

// The 49 fields in wire order (bit 0 first). Bits 39 and 40 are out of address order.
const std::array<CreatureField, CREATURE_FIELD_COUNT> &creatureFields();

// Total bytes of the fields a mask selects (bits past 48 count nothing).
size_t fieldBytes(uint64_t mask);

// The fields whose "changed" test says `now` differs from `previous`.
uint64_t changedFields(const CreatureSnapshot &now, const CreatureSnapshot &previous);

// One creature's update: its id, the mask, and the selected fields' bytes in bit order.
struct CreatureDelta {
    int64_t id = 0;
    uint64_t mask = 0;
    Bytes fields;
    // Set by decode when the body ended inside a field (the receiver keeps what came).
    bool cut = false;
};

// The update of `now` against `previous` (the copy last sent to that receiver), or a full one
// (every field) when `previous` is null. A delta is made even when nothing changed (mask 0).
CreatureDelta makeDelta(int64_t id, const CreatureSnapshot &now, const CreatureSnapshot *previous);

// Writes the delta's fields into a snapshot or a whole record; absent fields stay as they were.
void applyDelta(const CreatureDelta &delta, CreatureSnapshot &into);
void applyDelta(const CreatureDelta &delta, uint8_t *record);

// The decompressed body of packet 3: id, mask, fields.
Bytes encodeDeltaBody(const CreatureDelta &delta);
CreatureDelta decodeDeltaBody(const uint8_t *data, size_t size);

} // namespace cwnet
