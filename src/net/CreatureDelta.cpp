#include "net/CreatureDelta.h"

#include "NativeInteract.h"

#include <cstring>

namespace cwnet {

namespace {

constexpr size_t ITEM_SIZE = 0xa0;
constexpr int EQUIPMENT_SLOTS = 11;
constexpr size_t NAME_SIZE = 16;

// The field table of netcode-packets.md §3.
const std::array<CreatureField, CREATURE_FIELD_COUNT> FIELDS = {{
    {0, 0x10, 24, FieldTest::Bytes, "position (i64 x3, 1/65536 block)"},
    {1, 0x28, 12, FieldTest::Floats, "orientation"},
    {2, 0x34, 12, FieldTest::Floats, "velocity"},
    {3, 0x40, 12, FieldTest::Floats, "movement intent"},
    {4, 0x4c, 12, FieldTest::Floats, "push (knock-back)"},
    {5, 0x58, 4, FieldTest::Floats, "look pitch / body tilt"},
    {6, 0x5c, 4, FieldTest::Floats, "contact flags (compared as a float)"},
    {7, 0x60, 1, FieldTest::Bytes, "hostility kind"},
    {8, 0x64, 4, FieldTest::Floats, "race (compared as a float)"},
    {9, 0x68, 1, FieldTest::Bytes, "current ability"},
    {10, 0x6c, 4, FieldTest::Floats, "ability time"},
    {11, 0x70, 4, FieldTest::Floats, "combo counter (compared as a float)"},
    {12, 0x74, 4, FieldTest::Floats, "time since the last landed hit"},
    {13, 0x78, 0xb0, FieldTest::Appearance, "appearance"},
    {14, 0x128, 2, FieldTest::Bytes, "state flags"},
    {15, 0x12c, 4, FieldTest::Floats, "dodge timer"},
    {16, 0x130, 4, FieldTest::Floats, "stun timer"},
    {17, 0x134, 4, FieldTest::Floats, "timer"},
    {18, 0x138, 4, FieldTest::Floats, "timer"},
    {19, 0x13c, 4, FieldTest::Floats, "timer"},
    {20, 0x140, 4, FieldTest::Floats, "recovering value"},
    {21, 0x144, 4, FieldTest::Bytes, "level"},
    {22, 0x148, 4, FieldTest::Bytes, "experience"},
    {23, 0x14c, 1, FieldTest::Bytes, "class"},
    {24, 0x14d, 1, FieldTest::Bytes, "specialisation"},
    {25, 0x158, 4, FieldTest::Floats, "charge"},
    {26, 0x15c, 12, FieldTest::Floats, "stance A"},
    {27, 0x168, 12, FieldTest::Floats, "stance B"},
    {28, 0x174, 12, FieldTest::Floats, "aim / carrier offset"},
    {29, 0x180, 4, FieldTest::Floats, "health"},
    {30, 0x188, 4, FieldTest::Floats, "mana"},
    {31, 0x18c, 4, FieldTest::Floats, "stealth"},
    {32, 0x190, 4, FieldTest::Floats, "value"},
    {33, 0x194, 4, FieldTest::Floats, "ground grip"},
    {34, 0x198, 4, FieldTest::Floats, "light radius"},
    {35, 0x19c, 1, FieldTest::Bytes, "stage byte"},
    {36, 0x19d, 1, FieldTest::Bytes, "byte"},
    {37, 0x1a0, 8, FieldTest::Bytes, "owner id"},
    {38, 0x1a8, 8, FieldTest::Bytes, "linked creature id"},
    {39, 0x1b0, 8, FieldTest::Bytes, "flight zone pair"},
    {40, 0x150, 8, FieldTest::Bytes, "current region"},
    {41, 0x1b8, 24, FieldTest::Bytes, "home position"},
    {42, 0x1d8, ITEM_SIZE, FieldTest::Item, "item in use"},
    {43, 0x278, ITEM_SIZE * EQUIPMENT_SLOTS, FieldTest::Equipment, "equipment"},
    {44, 0x958, NAME_SIZE, FieldTest::Name, "name"},
    {45, 0x968, 8, FieldTest::Bytes, "reward-owner id"},
    {46, 0x970, 1, FieldTest::Bytes, "byte"},
    {47, 0x974, 4, FieldTest::Bytes, "value"},
    {48, 0x978, 8, FieldTest::Bytes, "value"},
}};

bool sameBytes(const uint8_t *a, const uint8_t *b, size_t n) { return std::memcmp(a, b, n) == 0; }

// Float by float, as the game compares: equal unless a != b in single precision.
bool sameFloats(const uint8_t *a, const uint8_t *b, size_t n) {
    for (size_t i = 0; i + 4 <= n; i += 4) {
        float x, y;
        std::memcpy(&x, a + i, 4);
        std::memcpy(&y, b + i, 4);
        if (x != y)
            return false;
    }
    return true;
}

// The appearance block's test (offsets within the block). Bytes 5..7 are never compared.
bool sameAppearance(const uint8_t *a, const uint8_t *b) {
    constexpr size_t FLAGS = 0x08, SCALES = 0x0c, MODELS = 0x18, MODELS_END = 0x28, FLOATS = 0x28, END = 0xb0;
    return sameBytes(a, b, 2) &&                                       // bytes 0-1
           sameBytes(a + 2, b + 2, 3) &&                               // bytes 2-4
           sameBytes(a + FLAGS, b + FLAGS, 4) &&                       // the appearance flags
           sameFloats(a + SCALES, b + SCALES, 12) &&                   // three floats
           sameBytes(a + MODELS, b + MODELS, MODELS_END - MODELS) &&   // eight model numbers (u16)
           sameFloats(a + FLOATS, b + FLOATS, END - FLOATS);           // the rest, float by float
}

bool sameName(const uint8_t *a, const uint8_t *b) {
    for (size_t i = 0; i < NAME_SIZE; i++) {
        if (a[i] != b[i])
            return false;
        if (a[i] == 0)
            break;
    }
    return true;
}

bool fieldChanged(const CreatureField &f, const uint8_t *a, const uint8_t *b) {
    switch (f.test) {
    case FieldTest::Bytes: return !sameBytes(a, b, f.size);
    case FieldTest::Floats: return !sameFloats(a, b, f.size);
    case FieldTest::Appearance: return !sameAppearance(a, b);
    case FieldTest::Item: return !nativeinteract::sameItem(a, b);
    case FieldTest::Equipment:
        for (int slot = 0; slot < EQUIPMENT_SLOTS; slot++)
            if (!nativeinteract::sameItem(a + slot * ITEM_SIZE, b + slot * ITEM_SIZE))
                return true;
        return false;
    case FieldTest::Name: return !sameName(a, b);
    }
    return true;
}

bool has(uint64_t mask, int bit) { return (mask >> bit) & 1; }

// Writes the delta's fields at the addresses `at` gives for each record offset.
template <class At> void writeFields(const CreatureDelta &delta, At at) {
    size_t read = 0;
    for (const CreatureField &f : FIELDS) {
        if (!has(delta.mask, f.bit))
            continue;
        if (read + f.size > delta.fields.size())
            return; // a cut update: what came is kept, the rest left as it was
        std::memcpy(at(f.offset), delta.fields.data() + read, f.size);
        read += f.size;
    }
}

} // namespace

CreatureSnapshot CreatureSnapshot::of(const uint8_t *record) {
    CreatureSnapshot s;
    std::memcpy(s.bytes.data(), record + SNAPSHOT_BEGIN, SNAPSHOT_SIZE);
    return s;
}

void CreatureSnapshot::writeTo(uint8_t *record) const { std::memcpy(record + SNAPSHOT_BEGIN, bytes.data(), SNAPSHOT_SIZE); }

const std::array<CreatureField, CREATURE_FIELD_COUNT> &creatureFields() { return FIELDS; }

size_t fieldBytes(uint64_t mask) {
    size_t total = 0;
    for (const CreatureField &f : FIELDS)
        if (has(mask, f.bit))
            total += f.size;
    return total;
}

uint64_t changedFields(const CreatureSnapshot &now, const CreatureSnapshot &previous) {
    uint64_t mask = 0;
    for (const CreatureField &f : FIELDS)
        if (fieldChanged(f, now.at(f.offset), previous.at(f.offset)))
            mask |= 1ull << f.bit;
    return mask;
}

CreatureDelta makeDelta(int64_t id, const CreatureSnapshot &now, const CreatureSnapshot *previous) {
    CreatureDelta d;
    d.id = id;
    d.mask = previous ? changedFields(now, *previous) : ALL_CREATURE_FIELDS;
    d.fields.reserve(fieldBytes(d.mask));
    for (const CreatureField &f : FIELDS)
        if (has(d.mask, f.bit))
            d.fields.insert(d.fields.end(), now.at(f.offset), now.at(f.offset) + f.size);
    return d;
}

void applyDelta(const CreatureDelta &delta, CreatureSnapshot &into) {
    writeFields(delta, [&](size_t offset) { return into.at(offset); });
}

void applyDelta(const CreatureDelta &delta, uint8_t *record) {
    writeFields(delta, [&](size_t offset) { return record + offset; });
}

Bytes encodeDeltaBody(const CreatureDelta &delta) {
    Bytes out;
    ByteWriter w(out);
    w.i64(delta.id);
    w.value(delta.mask);
    w.bytes(delta.fields.data(), delta.fields.size());
    return out;
}

CreatureDelta decodeDeltaBody(const uint8_t *data, size_t size) {
    CreatureDelta d;
    ByteReader r(data, size);
    r.value(d.id);
    r.value(d.mask);
    // Each field is read whole or not at all (the reader rule); bits past 48 name no field.
    for (const CreatureField &f : FIELDS) {
        if (!has(d.mask, f.bit))
            continue;
        if (r.remaining() < f.size) {
            d.cut = true;
            break;
        }
        d.fields.insert(d.fields.end(), r.here(), r.here() + f.size);
        r.skip(f.size);
    }
    d.cut = d.cut || r.wasShort();
    return d;
}

} // namespace cwnet
