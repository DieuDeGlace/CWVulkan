#pragma once
// Machine contract only: steam-mod-loader-capture-abi.md §§3–6. This is an
// explicit callback record, not an SDK class or an unload/ownership protocol.
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace captureabi {
using CaptureEntry = void (*)();
struct CapturePlugin {
    using Initialize = void (*)(void *);
    using Tick = void (*)(void *, std::uintptr_t);

    const CaptureEntry *entries;
    std::array<std::uint8_t, 29> priorities;
    // Project-owned data begins after the loader's observed prefix.
    void *user;
    Initialize initialize;
    Tick tick;

    CapturePlugin(void *user, Initialize initialize, Tick tick);
};
static_assert(sizeof(void *) == 8 && sizeof(CaptureEntry) == 8, "The supported loader ABI is AMD64");
static_assert(std::is_standard_layout<CapturePlugin>::value && offsetof(CapturePlugin, entries) == 0 &&
              offsetof(CapturePlugin, priorities) == 8);

namespace plugindetail {
using Q = std::uintptr_t;
using U32 = std::uint32_t;
inline void initialize(CapturePlugin *self) {
    if (self->initialize)
        self->initialize(self->user);
}
inline void tick(CapturePlugin *self, Q context) {
    if (self->tick)
        self->tick(self->user, context);
}
inline int passOne(CapturePlugin *, Q) { return 0; }
inline int passTwo(CapturePlugin *, Q, Q) { return 0; }
inline int passFour(CapturePlugin *, Q, U32, Q, Q) { return 0; }
inline void ignoreOne(CapturePlugin *, Q) {}
inline void ignoreTwo(CapturePlugin *, Q, Q) {}
inline void ignoreThree(CapturePlugin *, Q, Q, Q) {}
inline void ignoreTwoWords(CapturePlugin *, Q, U32, U32) {}
inline void ignoreDiscriminator(CapturePlugin *, Q, Q, U32) {}
inline void preserveByte(CapturePlugin *, Q, Q, std::uint8_t *) {}
inline void preserveInteger(CapturePlugin *, Q, std::int32_t *) {}
inline void preserveClassifiedByte(CapturePlugin *, Q, U32, std::uint8_t *) {}

// Keep every reached position, including disabled events. No native callback
// name or pointed-to record schema is inferred from opaque register arguments.
inline const std::array<CaptureEntry, 30> entries{{
    reinterpret_cast<CaptureEntry>(initialize),             // 0x00
    reinterpret_cast<CaptureEntry>(passOne),                // 0x08
    reinterpret_cast<CaptureEntry>(passTwo),                // 0x10
    reinterpret_cast<CaptureEntry>(passOne),                // 0x18
    reinterpret_cast<CaptureEntry>(tick),                   // 0x20
    reinterpret_cast<CaptureEntry>(ignoreOne),              // 0x28
    reinterpret_cast<CaptureEntry>(ignoreOne),              // 0x30
    reinterpret_cast<CaptureEntry>(passFour),               // 0x38
    reinterpret_cast<CaptureEntry>(ignoreOne),              // 0x40
    reinterpret_cast<CaptureEntry>(ignoreOne),              // 0x48
    reinterpret_cast<CaptureEntry>(ignoreTwoWords),         // 0x50
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x58
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x60
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x68
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x70
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x78
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x80
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x88
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x90
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0x98
    reinterpret_cast<CaptureEntry>(ignoreOne),              // 0xa0
    reinterpret_cast<CaptureEntry>(ignoreOne),              // 0xa8
    reinterpret_cast<CaptureEntry>(ignoreDiscriminator),    // 0xb0
    reinterpret_cast<CaptureEntry>(ignoreThree),            // 0xb8
    reinterpret_cast<CaptureEntry>(ignoreOne),              // 0xc0
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0xc8
    reinterpret_cast<CaptureEntry>(ignoreTwo),              // 0xd0
    reinterpret_cast<CaptureEntry>(preserveByte),           // 0xd8
    reinterpret_cast<CaptureEntry>(preserveInteger),        // 0xe0
    reinterpret_cast<CaptureEntry>(preserveClassifiedByte), // 0xe8
}};
} // namespace plugindetail

inline CapturePlugin::CapturePlugin(void *user, Initialize initialize, Tick tick)
    : entries(plugindetail::entries.data()), user(user), initialize(initialize), tick(tick) {
    // Capture policy, not a recovered SDK default: only tick participates, in
    // priority pass0. Other bytes intentionally match none of the five passes.
    priorities.fill(255);
    priorities[3] = 0;
}
} // namespace captureabi
