#include "UseCreatureCollection.h"
#include <cstring>
#include <iterator>

namespace usecollection {
namespace {
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
template <class T> void write(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
void copySpan(uint8_t *destination, const uint8_t *source, size_t offset, size_t bytes) {
    std::memmove(destination + offset, source + offset, bytes);
}
void copyItemValues(uint8_t *destination, const uint8_t *source, size_t base) {
    copySpan(destination, source, base, 1);
    copySpan(destination, source, base + 4, 16);
    copySpan(destination, source, base + 0x14, 1);
    copySpan(destination, source, base + 0x18, 0x86); // type-specific word, material, all 32 attributes and count
}
void copyValues(uint8_t *destination, const uint8_t *source) {
    // Contiguous assigned scalars are grouped; unassigned padding is retained.
    // Exact boundaries are in the independent collection contract section 5.2.
    for (const auto [offset, bytes] :
         {std::pair<size_t, size_t>{0x10, 24}, {0x28, 56}, {0x60, 1}, {0x64, 4}, {0x68, 1}, {0x6c, 12}})
        copySpan(destination, source, offset, bytes);
    copySpan(destination, source, 0x78, 5);
    copySpan(destination, source, 0x80, 0xa8); // appearance+8 through+0xaf
    for (const auto [offset, bytes] : {std::pair<size_t, size_t>{0x128, 2},
                                       {0x12c, 32},
                                       {0x14c, 2},
                                       {0x150, 76},
                                       {0x19c, 2},
                                       {0x1a0, 16},
                                       {0x1b0, 8},
                                       {0x1b8, 24},
                                       {0x1d0, 8}})
        copySpan(destination, source, offset, bytes);
    copyItemValues(destination, source, 0x1d8);
    for (size_t slot = 0; slot < 11; ++slot)
        copyItemValues(destination, source, 0x278 + slot * 0xa0);
    for (const auto [offset, bytes] : {std::pair<size_t, size_t>{0x958, 16},
                                       {0x968, 8},
                                       {0x970, 1},
                                       {0x974, 4},
                                       {0x978, 8},
                                       {0x998, 4},
                                       {0x99c, 4},
                                       {0xc44, 4},
                                       {0xc40, 4}})
        copySpan(destination, source, offset, bytes);
}
} // namespace

uint8_t *Collection::find(int64_t id) {
    const auto found = copies.find(id);
    return found == copies.end() ? nullptr : found->second.data();
}
const uint8_t *Collection::find(int64_t id) const {
    const auto found = copies.find(id);
    return found == copies.end() ? nullptr : found->second.data();
}
uint8_t *Collection::add(const uint8_t *source) {
    const auto id = read<int64_t>(source, 8);
    auto entry = copies.find(id);
    if (entry == copies.end())
        entry = copies.emplace(id, worldgen::newCreature(id)).first;
    auto *destination = entry->second.data();
    copyValues(destination, source);
    return destination;
}
bool Collection::remove(int64_t id) { return copies.erase(id) != 0; }

const uint8_t *Collection::wrapped() const {
    if (copies.empty())
        return nullptr;
    // This models the established ordinary count domain (<= INT32_MAX).
    // Larger collection counts remain outside this verified contract.
    const auto count = int64_t(copies.size());
    auto slot = int64_t(selectionIndex) % count;
    if (slot < 0)
        slot += count;
    auto found = copies.begin();
    std::advance(found, slot);
    return found->second.data();
}
void Collection::selectAdded(int64_t id) {
    selectedId = id;
    selectionIndex = 0;
    for (const auto &[key, record] : copies) {
        if (read<int64_t>(record.data(), 8) == id)
            break;
        ++selectionIndex;
    }
}
void Collection::complete(uint8_t *player, float effectTime, const Resolve &resolve, const Sound &sound) {
    if (!player)
        return;
    const auto ability = player[0x68];
    if ((ability != 0x96 && ability != 0x97) || !(read<float>(player, 0x6c) > effectTime))
        return;
    player[0x68] = 0;
    write(player, 0x6c, 0.0f);
    const auto target = read<int64_t>(player, 0x1a8);
    if (ability == 0x96) {
        const auto *source = resolve ? resolve(target) : nullptr;
        if (!source)
            return;
        const auto id = read<int64_t>(source, 8);
        add(source);
        if (sound)
            sound(0x30);
        selectAdded(id);
    } else if (find(target)) {
        remove(target);
        if (sound)
            sound(0x3c);
        const auto *next = wrapped();
        selectedId = next ? read<int64_t>(next, 8) : 0;
    }
}
} // namespace usecollection
