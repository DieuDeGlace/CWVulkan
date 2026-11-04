#include "../tools/ZoneExtractor/NativeCreatureScalars.h"
#include <cmath>
#include <cstring>
#include <iostream>

namespace {
template <class T> void set(uint8_t *record, size_t offset, T value) {
    std::memcpy(record + offset, &value, sizeof value);
}
template <class T> T read(const uint8_t *record, size_t offset) {
    T value;
    std::memcpy(&value, record + offset, sizeof value);
    return value;
}
} // namespace

int nativeCreatureScalarsTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL creature scalars: " << label << '\n';
            ++failures;
        }
    };
    std::array<uint8_t, 0xb00> record;
    record.fill(0xa5);
    set(record.data(), 8, int64_t(-42));
    set(record.data(), 0x10, INT64_MIN);
    set(record.data(), 0x18, int64_t(-1));
    set(record.data(), 0x20, INT64_MAX);
    set(record.data(), 0x28, 17.25f);
    set(record.data(), 0x2c, -45.5f);
    set(record.data(), 0x30, -15.5f);
    set(record.data(), 0x5c, uint32_t(0x80000005));
    set(record.data(), 0x64, uint32_t(0xf0000001));
    set(record.data(), 0x80, uint32_t(0x80002000));
    record[0x60] = 0xf3;
    record[0x68] = 0xfe;
    record[0x74] = 0xf5;
    set(record.data(), 0x70, INT32_MIN);
    set(record.data(), 0x128, uint16_t(0xff08));
    set(record.data(), 0x130, -3000.25f);
    set(record.data(), 0x140, -0.f);
    set(record.data(), 0x158, 0.5f);
    set(record.data(), 0x6c, 2.75f);
    set(record.data(), 0x34, -0.0f);
    set(record.data(), 0x38, -2.0f);
    set(record.data(), 0x3c, 3.0f);
    set(record.data(), 0x144, int32_t(-7));
    record[0x14c] = 3;
    record[0x14d] = 1;
    set(record.data(), 0x150, INT32_MIN);
    set(record.data(), 0x154, INT32_MAX);
    set(record.data(), 0x180, 50.0f);
    set(record.data(), 0x184, 0.25f); // block power, not mana
    set(record.data(), 0x188, 0.75f);
    set(record.data(), 0x9a0, 0.625f);
    set(record.data(), 0xaac, int32_t(INT32_MIN));
    const std::array<char, 16> name{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::memcpy(record.data() + 0x958, name.data(), name.size());
    captureabi::CreatureScalars values(record.data());
    const auto position = values.position();
    const auto velocity = values.velocity();
    check(values.id() == -42 && position.x == INT64_MIN && position.y == -1 && position.z == INT64_MAX &&
              read<uint32_t>(reinterpret_cast<const uint8_t *>(&velocity), 0) == 0x80000000 && velocity.y == -2 &&
              velocity.z == 3 && values.level() == -7 && values.classId() == 3 && values.specialization() == 1 &&
              values.health() == 50 && values.mana() == 0.75f && values.gold() == INT32_MIN && values.name() == name,
          "independent record bytes decode fixed/scalar values and full unterminated name");
    check(captureabi::fixedFromBlocks(-1) == -65536 && captureabi::fixedFromBlocks(1) == 65536 &&
              captureabi::fixedFromBlocks(INT64_MIN) == 0 && captureabi::fixedFromBlocks(INT64_MAX) == -65536 &&
              captureabi::fixedFromBlocks(-(int64_t(1) << 47)) == INT64_MIN,
          "negative and boundary block coordinates scale without signed-shift overflow");
    check(values.pitch() == 17.25f && values.roll() == -45.5f && values.yaw() == -15.5f &&
              values.movementFlags() == 0x80000005 && values.ability() == 0xfe && values.abilityClock() == 2.75f &&
              values.race() == 0xf0000001 && values.appearanceFlags() == 0x80002000 && values.hostility() == 0xf3,
          "recording metadata retains yaw, full flag bits, unsigned ability byte and float clock");
    check(captureabi::fixedDelta(INT64_MAX, INT64_MAX - 1) == 1 &&
              captureabi::fixedDelta(INT64_MIN, INT64_MIN + 1) == -1 &&
              captureabi::fixedDelta(INT64_MAX, INT64_MIN) == 18446744073709551616.0 &&
              captureabi::fixedDelta(INT64_MIN, INT64_MAX) == -18446744073709551616.0 &&
              captureabi::fixedDelta(-65536, 65536) == -131072 && captureabi::fixedDelta(-1, -1) == 0,
          "diagnostic signed64 differences retain nearby precision without signed subtraction overflow");
    check(values.stamina() == 0.625f && values.playerRegion() == std::array<int32_t, 2>{INT32_MIN, INT32_MAX},
          "player item-region coordinates retain both signed32 extremes");
    check(values.state74() == 0xf5 && values.actionCounter() == INT32_MIN && values.physicalFlags() == 0xff08 &&
              values.stunTimer() == -3000.25f && values.charge158() == 0.5f && values.mana() == 0.75f &&
              std::signbit(values.timer140()),
          "documented state byte, word, counter and float timers remain separate from mana/charge accumulator");
    const auto beforePartialVelocity = record;
    values.setVelocityX(7);
    values.setVelocityY(-8);
    bool partialVelocityRetained = true;
    for (size_t i = 0; i < record.size(); ++i)
        if (i < 0x34 || i >= 0x3c)
            partialVelocityRetained &= record[i] == beforePartialVelocity[i];
    check(values.velocity().x == 7 && values.velocity().y == -8 && values.velocity().z == 3 && partialVelocityRetained,
          "axis-only recorder writes preserve vertical velocity and every other byte");
    const auto before = record;
    values.setPosition({11, -12, 13});
    values.setVelocity({});
    values.setLevel(14);
    values.setSpecialization(0);
    values.setHealth(15);
    values.setMana(1);
    check(values.position().y == -12 && values.velocity().x == 0 && values.velocity().z == 0 && values.level() == 14 &&
              values.specialization() == 0 && values.health() == 15 && values.mana() == 1 &&
              read<float>(record.data(), 0x184) == 0.25f && values.name() == name && values.id() == -42,
          "recorder mutations keep mana/block power separate and retain identity/name");
    bool unrelatedRetained = true;
    for (size_t i = 0; i < record.size(); ++i) {
        const bool assigned = (i >= 0x10 && i < 0x28) || (i >= 0x34 && i < 0x40) || (i >= 0x144 && i < 0x148) ||
                              i == 0x14d || (i >= 0x180 && i < 0x184) || (i >= 0x188 && i < 0x18c);
        if (!assigned)
            unrelatedRetained &= record[i] == before[i];
    }
    check(unrelatedRetained, "primitive writes preserve every unrelated record byte");
    std::cout << "Native creature scalar views: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
