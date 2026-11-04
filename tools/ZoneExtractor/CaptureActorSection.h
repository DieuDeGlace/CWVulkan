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

// Only the separate client World: the host registry also has writers under a
// different outer guard (steam-world-actor-protection.md). The active callback
// must retain Game/World lifetimes. This is not an asynchronous capture API or a
// transaction across every nested creature allocation/field.
class ClientCreatureCapture {
    std::uintptr_t worldAddress;
    WorldActorLease held;
    std::vector<std::uintptr_t> values;
    bool readable = false;

    template <class Read> static std::uintptr_t resolveWorld(std::uintptr_t game, Read read) {
        std::uintptr_t world = 0;
        readClientWorld(game, world, read);
        return world;
    }

  public:
    template <class Read>
    ClientCreatureCapture(std::uintptr_t game, Read read) : worldAddress(resolveWorld(game, read)), held(worldAddress) {
        std::uintptr_t current = 0;
        if (held && readClientWorld(game, current, read) && current == worldAddress)
            readable = readRegisteredCreatures(worldAddress, values, read);
    }
    bool ready() const { return readable; }
    std::uintptr_t world() const { return readable ? worldAddress : 0; }
    const std::vector<std::uintptr_t> &addresses() const { return values; }
};
} // namespace captureabi
