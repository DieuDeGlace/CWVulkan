#pragma once
// Observed client frame guard: steam-world-creature-views.md section6.
// World lifetime remains a caller precondition. This excludes only writers
// cooperating with the same guard; it is not a pin or universal snapshot lock.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "NativeCreatureViews.h"
#include <windows.h>

namespace captureabi {
class WorldActorLease {
    CRITICAL_SECTION *held = nullptr;

  public:
    explicit WorldActorLease(std::uintptr_t world) {
        constexpr auto maximum = std::numeric_limits<std::uintptr_t>::max();
        if (!world || world > maximum - WORLD_ACTOR_SECTION - (sizeof(CRITICAL_SECTION) - 1))
            return;
        auto *section = reinterpret_cast<CRITICAL_SECTION *>(world + WORLD_ACTOR_SECTION);
        if (TryEnterCriticalSection(section))
            held = section;
    }
    ~WorldActorLease() {
        if (held)
            LeaveCriticalSection(held);
    }
    WorldActorLease(const WorldActorLease &) = delete;
    WorldActorLease &operator=(const WorldActorLease &) = delete;
    explicit operator bool() const { return held != nullptr; }
};
} // namespace captureabi
