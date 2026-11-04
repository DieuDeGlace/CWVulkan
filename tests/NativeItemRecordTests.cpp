#include "../tools/ZoneExtractor/NativeItemRecord.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>

int nativeItemRecordTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL capture item: " << label << '\n';
            ++failures;
        }
    };
    std::array<uint8_t, 160> source;
    for (size_t i = 0; i < source.size(); ++i)
        source[i] = uint8_t(i);
    const auto original = source;
    const auto copied = captureabi::ItemRecord::copy(source.data());
    check(copied.bytes == source && copied.category() == 0 && copied.subtype() == 0x07060504 &&
              copied.modifier() == 0x0b0a0908 && copied.regionX() == 0x0f0e0d0c && copied.regionY() == 0x13121110 &&
              copied.rarity() == 0x14 && copied.representedType() == 0x1b1a1918 && copied.material() == 0x1c &&
              copied.attributeCount() == 0x9d,
          "independent serialized bytes decode every item value while retaining attributes and padding");
    auto edited = copied;
    edited.setCategory(0xff);
    edited.setSubtype(INT32_MIN);
    edited.setModifier(0xfedcba98);
    edited.setRegion(-7, 11);
    edited.setRarity(0x80);
    edited.setMaterial(0xfe);
    check(edited.category() == 255 && edited.subtype() == INT32_MIN && edited.modifier() == 0xfedcba98 &&
              edited.regionX() == -7 && edited.regionY() == 11 && edited.rarity() == 128 && edited.material() == 254 &&
              source == original && copied.bytes == original,
          "owned probe values preserve signed/unsigned widths without mutating their borrowed source");
    check(edited.bytes[1] == 1 && edited.bytes[0x15] == 0x15 && edited.bytes[0x18] == 0x18 &&
              edited.bytes[0x1d] == 0x1d && edited.bytes[0x9d] == 0x9d && edited.bytes[0x9f] == 0x9f,
          "probe setters retain unrelated full-word, attribute, count and padding bytes");
    const captureabi::ItemRecord empty;
    check(std::all_of(empty.bytes.begin(), empty.bytes.end(), [](uint8_t byte) { return byte == 0; }),
          "project-owned empty probe storage is explicitly initialized");

    std::array<uint8_t, 0x278 + 11 * 160> creature{};
    for (int slot = 0; slot < 11; ++slot)
        std::fill_n(creature.begin() + 0x278 + slot * 160, 160, uint8_t(slot + 1));
    const auto equipment = captureabi::copyEquipment(creature.data());
    bool exact = true;
    for (int slot = 0; slot < 11; ++slot) {
        exact &= captureabi::equipmentItem(creature.data(), slot) == creature.data() + 0x278 + slot * 160;
        exact &= std::all_of(equipment[size_t(slot)].bytes.begin(), equipment[size_t(slot)].bytes.end(),
                             [slot](uint8_t byte) { return byte == slot + 1; });
    }
    check(exact, "all eleven equipment records retain exact byte order and native offsets");
    check(!captureabi::equipmentItem(nullptr, 0) && !captureabi::equipmentItem(creature.data(), -1) &&
              !captureabi::equipmentItem(creature.data(), 11) &&
              !captureabi::equipmentItem(reinterpret_cast<void *>(std::numeric_limits<uintptr_t>::max() - 8), 0),
          "equipment slot addressing rejects null, out-of-range and overflowing spans");
    std::cout << "Native capture item records: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
