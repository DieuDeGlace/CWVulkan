#pragma once
// Independent value layout: drops-and-items.md section0.1 and
// steam-use-creature-collection.md section5.2. No SDK constructors or overlays.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace captureabi {
static_assert(sizeof(std::uintptr_t) == 8, "The capture ABI is AMD64");
constexpr std::size_t ITEM_BYTES = 0xa0;
constexpr std::size_t EQUIPMENT_ITEMS = 11;
constexpr std::size_t EQUIPMENT_OFFSET = 0x278;

struct alignas(8) ItemRecord {
    std::array<std::uint8_t, ITEM_BYTES> bytes{};

    template <class T> T field(std::size_t offset) const {
        T value;
        std::memcpy(&value, bytes.data() + offset, sizeof value);
        return value;
    }
    template <class T> void set(std::size_t offset, T value) {
        std::memcpy(bytes.data() + offset, &value, sizeof value);
    }
    std::uint8_t category() const { return bytes[0]; }
    std::int32_t subtype() const { return field<std::int32_t>(4); }
    std::uint32_t modifier() const { return field<std::uint32_t>(8); }
    std::int32_t regionX() const { return field<std::int32_t>(0x0c); }
    std::int32_t regionY() const { return field<std::int32_t>(0x10); }
    std::uint8_t rarity() const { return bytes[0x14]; }
    std::int32_t representedType() const { return field<std::int32_t>(0x18); }
    std::uint8_t material() const { return bytes[0x1c]; }
    std::uint8_t attributeCount() const { return bytes[0x9d]; }

    void setCategory(std::uint8_t value) { bytes[0] = value; }
    void setSubtype(std::int32_t value) { set(4, value); }
    void setModifier(std::uint32_t value) { set(8, value); }
    void setRarity(std::uint8_t value) { bytes[0x14] = value; }
    void setMaterial(std::uint8_t value) { bytes[0x1c] = value; }
    void setRegion(std::int32_t x, std::int32_t y) {
        set(0x0c, x);
        set(0x10, y);
    }

    // Caller retains a valid readable ITEM_BYTES span. This copies original
    // bytes, including padding, for capture; it is not native semantic assignment.
    static ItemRecord copy(const void *source) {
        ItemRecord result;
        std::memcpy(result.bytes.data(), source, ITEM_BYTES);
        return result;
    }
};
static_assert(sizeof(ItemRecord) == ITEM_BYTES && std::is_trivially_copyable<ItemRecord>::value &&
              std::is_standard_layout<ItemRecord>::value && offsetof(ItemRecord, bytes) == 0);
using EquipmentRecord = std::array<ItemRecord, EQUIPMENT_ITEMS>;
static_assert(sizeof(EquipmentRecord) == EQUIPMENT_ITEMS * ITEM_BYTES);

inline std::uint8_t *equipmentItem(void *creature, int slot) {
    if (!creature || slot < 0 || std::size_t(slot) >= EQUIPMENT_ITEMS)
        return nullptr;
    const auto address = reinterpret_cast<std::uintptr_t>(creature);
    const auto offset = EQUIPMENT_OFFSET + std::size_t(slot) * ITEM_BYTES;
    if (address > std::numeric_limits<std::uintptr_t>::max() - offset - (ITEM_BYTES - 1))
        return nullptr;
    return reinterpret_cast<std::uint8_t *>(address + offset);
}
inline EquipmentRecord copyEquipment(const void *creature) {
    // The same caller-owned lifetime/readability requirement covers all 11 slots.
    EquipmentRecord result;
    std::memcpy(result.data(), static_cast<const std::uint8_t *>(creature) + EQUIPMENT_OFFSET, sizeof result);
    return result;
}
} // namespace captureabi
