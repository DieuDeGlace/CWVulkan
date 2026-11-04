#include "../tools/ZoneExtractor/CapturePlugin.h"
#include <cstring>
#include <iostream>

namespace {
struct Probe {
    unsigned initializations = 0, ticks = 0;
    std::uintptr_t context = 0;
};
void initialize(void *user) { ++static_cast<Probe *>(user)->initializations; }
void tick(void *user, std::uintptr_t context) {
    auto &probe = *static_cast<Probe *>(user);
    ++probe.ticks;
    probe.context = context;
}
} // namespace

int capturePluginTests() {
    int failures = 0;
    const auto check = [&](bool passed, const char *label) {
        if (!passed) {
            std::cout << "FAIL capture plugin ABI: " << label << '\n';
            ++failures;
        }
    };
    Probe probe;
    captureabi::CapturePlugin plugin(&probe, initialize, tick);
    // The synthetic host consumes raw ABI bytes/positions, not the adapter's
    // field accessors or slot constants. Function signatures follow the prose.
    using Object = captureabi::CapturePlugin;
    using Entry = void (*)();
    const Entry *table = nullptr;
    std::memcpy(&table, &plugin, 8);
    std::array<std::uint8_t, 29> priorities{};
    std::memcpy(priorities.data(), reinterpret_cast<const std::byte *>(&plugin) + 8, priorities.size());
    check(table && std::is_standard_layout<captureabi::CapturePlugin>::value &&
              !std::is_polymorphic<captureabi::CapturePlugin>::value,
          "the explicit record has no compiler-inserted destructor/vtable prefix");
    if (!table)
        return failures + 1;
    for (unsigned slot = 0; slot < 30; ++slot)
        check(table[slot] != nullptr, "all thirty reached callback positions exist");
    reinterpret_cast<void (*)(Object *)>(table[0])(&plugin);
    check(probe.initializations == 1 && probe.ticks == 0,
          "slot0 initializes the factory result rather than deleting it");
    unsigned admitted = 0;
    constexpr std::uintptr_t context = 0xfedcba9876543210ull;
    for (unsigned pass = 0; pass < 5; ++pass)
        for (unsigned event = 0; event < 29; ++event)
            if (priorities[event] == pass) {
                ++admitted;
                check(event == 3 && pass == 0, "capture policy admits only tick at priority0");
                if (event == 3)
                    reinterpret_cast<void (*)(Object *, std::uintptr_t)>(table[4])(&plugin, context);
            }
    check(admitted == 1 && probe.ticks == 1 && probe.context == context,
          "native byte-priority routing forwards the opaque tick context and the correct user state");
    using Q = std::uintptr_t;
    constexpr Q opaque = ~Q(0);
    for (const auto slot : {1, 3})
        check(reinterpret_cast<int (*)(Object *, Q)>(table[slot])(&plugin, opaque) == 0,
              "one-argument interception events return a defined neutral EAX");
    check(reinterpret_cast<int (*)(Object *, Q, Q)>(table[2])(&plugin, opaque, 0) == 0,
          "two-argument interception event passes through");
    check(reinterpret_cast<int (*)(Object *, Q, std::uint32_t, Q, Q)>(table[7])(&plugin, opaque, 0x87654321, 0,
                                                                                opaque) == 0,
          "the five-position call, including its stacked argument, returns neutral");
    for (const auto slot : {5, 6, 8, 9, 20, 21, 24})
        reinterpret_cast<void (*)(Object *, Q)>(table[slot])(&plugin, opaque);
    for (const auto slot : {11, 12, 13, 14, 15, 16, 17, 18, 19, 25, 26})
        reinterpret_cast<void (*)(Object *, Q, Q)>(table[slot])(&plugin, opaque, 0);
    reinterpret_cast<void (*)(Object *, Q, std::uint32_t, std::uint32_t)>(table[10])(&plugin, opaque, 0x89abcdef,
                                                                                     0x76543210);
    for (std::uint32_t discriminator = 0; discriminator < 3; ++discriminator)
        reinterpret_cast<void (*)(Object *, Q, Q, std::uint32_t)>(table[22])(&plugin, opaque, 0, discriminator);
    reinterpret_cast<void (*)(Object *, Q, Q, Q)>(table[23])(&plugin, opaque, 0, opaque);
    std::array<std::uint8_t, 3> bytes{{0x36, 0xa5, 0x79}};
    std::int32_t integer = -87654321;
    reinterpret_cast<void (*)(Object *, Q, Q, std::uint8_t *)>(table[27])(&plugin, opaque, 0, &bytes[1]);
    reinterpret_cast<void (*)(Object *, Q, std::int32_t *)>(table[28])(&plugin, opaque, &integer);
    reinterpret_cast<void (*)(Object *, Q, std::uint32_t, std::uint8_t *)>(table[29])(&plugin, opaque, 2, &bytes[1]);
    check(bytes == std::array<std::uint8_t, 3>{0x36, 0xa5, 0x79} && integer == -87654321,
          "ignored callbacks preserve caller-initialized byte and signed32 state, including adjacent bytes");
    check(probe.initializations == 1 && probe.ticks == 1, "unused callbacks do not activate capture hooks");
    captureabi::CapturePlugin inactive(nullptr, nullptr, nullptr);
    reinterpret_cast<void (*)(Object *)>(table[0])(&inactive);
    reinterpret_cast<void (*)(Object *, Q)>(table[4])(&inactive, opaque);
    std::cout << "Capture plugin ABI: " << (failures ? "FAIL" : "PASS") << '\n';
    return failures;
}
