#pragma once
// Primitive values: player-control.md record table, creature-upkeep.md section0,
// native-saves.md section3.3 and steam-world-creature-views.md section2.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace captureabi {
struct FixedPosition {
    std::int64_t x = 0, y = 0, z = 0;
};
struct Velocity {
    float x = 0, y = 0, z = 0;
};
static_assert(sizeof(FixedPosition) == 24 && sizeof(Velocity) == 12);

// Recorder block-to-fixed conversion. Unsigned scaling defines the low 64 bits
// even for negative coordinates; it avoids shifting a signed negative value.
inline std::int64_t fixedFromBlocks(std::int64_t blocks) {
    const auto bits = static_cast<std::uint64_t>(blocks) << 16;
    std::int64_t result;
    std::memcpy(&result, &bits, sizeof result);
    return result;
}

// A borrowed native record, not an overlay C++ class or an allocation owner.
// Caller supplies a valid retained readable/writable span for every field used.
class CreatureScalars {
    std::uint8_t *record;
    template <class T> T read(std::size_t offset) const {
        T value;
        std::memcpy(&value, record + offset, sizeof value);
        return value;
    }
    template <class T> void write(std::size_t offset, T value) { std::memcpy(record + offset, &value, sizeof value); }

  public:
    explicit CreatureScalars(void *native) : record(static_cast<std::uint8_t *>(native)) {}
    std::int64_t id() const { return read<std::int64_t>(8); }
    FixedPosition position() const { return read<FixedPosition>(0x10); }
    Velocity velocity() const { return read<Velocity>(0x34); }
    std::int32_t level() const { return read<std::int32_t>(0x144); }
    std::uint8_t classId() const { return record[0x14c]; }
    std::uint8_t specialization() const { return record[0x14d]; }
    float yaw() const { return read<float>(0x30); }
    std::uint32_t movementFlags() const { return read<std::uint32_t>(0x5c); }
    std::uint32_t race() const { return read<std::uint32_t>(0x64); }
    std::uint32_t appearanceFlags() const { return read<std::uint32_t>(0x80); }
    std::uint8_t ability() const { return record[0x68]; }
    float abilityClock() const { return read<float>(0x6c); }
    std::int32_t gold() const { return read<std::int32_t>(0xaac); }
    float health() const { return read<float>(0x180); }
    float mana() const { return read<float>(0x188); }
    std::array<char, 16> name() const { return read<std::array<char, 16>>(0x958); }

    void setPosition(FixedPosition value) { write(0x10, value); }
    void setVelocity(Velocity value) { write(0x34, value); }
    void setLevel(std::int32_t value) { write(0x144, value); }
    void setSpecialization(std::uint8_t value) { record[0x14d] = value; }
    void setHealth(float value) { write(0x180, value); }
    void setMana(float value) { write(0x188, value); }
};
} // namespace captureabi
