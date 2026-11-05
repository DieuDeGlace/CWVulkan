#pragma once
// Original normal-use cleanup: steam-temporary-zone-and-role-lifetimes.md.
#include <cstdint>

namespace captureabi {
using DestroyRegionRoles = void (*)(void *);
constexpr uintptr_t DESTROY_REGION_ROLES_RVA = 0x82450;
class RegionRolesLease {
    void *value;
    DestroyRegionRoles dispose;

  public:
    // Adopt only a successfully constructed destination, retaining its storage
    // until this lease ends. The native cleanup does not free that storage.
    RegionRolesLease(void *record, DestroyRegionRoles destroy) : value(record), dispose(destroy) {}
    ~RegionRolesLease() { dispose(value); }
    RegionRolesLease(const RegionRolesLease &) = delete;
    RegionRolesLease &operator=(const RegionRolesLease &) = delete;
};
} // namespace captureabi
