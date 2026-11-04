#pragma once
// Native lock identities/order: steam-loaded-zone-container.md §§4–5 and
// steam-terrain-buffer-resources.md §5. World lifetime is a caller prerequisite.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "NativeLoadedZones.h"
#include "NativeTerrainBuffers.h"
#include <windows.h>

namespace captureabi {
class WorldBufferLease {
    CRITICAL_SECTION *zones = nullptr, *mesh = nullptr;
    static CRITICAL_SECTION *section(std::uintptr_t world, std::size_t offset) {
        constexpr auto maximum = std::numeric_limits<std::uintptr_t>::max();
        if (!world || offset > maximum - world || sizeof(CRITICAL_SECTION) - 1 > maximum - (world + offset))
            return nullptr;
        return reinterpret_cast<CRITICAL_SECTION *>(world + offset);
    }

  public:
    explicit WorldBufferLease(std::uintptr_t world) {
        auto *owner = section(world, WORLD_ZONES_SECTION);
        auto *publication = section(world, WORLD_MESH_SECTION);
        if (!owner || !publication || !TryEnterCriticalSection(owner))
            return;
        if (!TryEnterCriticalSection(publication)) {
            LeaveCriticalSection(owner);
            return;
        }
        zones = owner;
        mesh = publication;
    }
    ~WorldBufferLease() {
        if (mesh)
            LeaveCriticalSection(mesh);
        if (zones)
            LeaveCriticalSection(zones);
    }
    WorldBufferLease(const WorldBufferLease &) = delete;
    WorldBufferLease &operator=(const WorldBufferLease &) = delete;
    explicit operator bool() const { return mesh != nullptr; }
};
} // namespace captureabi
