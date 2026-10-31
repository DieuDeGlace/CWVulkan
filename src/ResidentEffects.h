#pragma once
// Independent steam-resident-effects-prerequisites.md §§3–6.
#include "NativeEffects.h"
#include <array>
#include <functional>

namespace residentfx {
class Request {
  public:
    void existing(int32_t mode) {
        if (mode == 0x8a)
            permission_ = false;
    }
    void created(int32_t mode) {
        if (uint8_t(mode) == 0x8a && permission_) {
            requested_ = true;
            permission_ = false;
        }
    }
    bool requested() const { return requested_; }

  private:
    bool permission_ = true, requested_ = false;
};
struct Candidate {
    std::array<int64_t, 2> center;
    std::array<int64_t, 3> position;
};
using Ground = std::function<float(int32_t x, int32_t y)>;
Candidate candidate(int32_t zoneX, int32_t zoneY, int32_t day, const Ground &ground);
bool suppressed(const Candidate &candidate, const std::list<nativeeffects::Effect> &live);
// Writes the specified semantic fields. Native leaves damage and padding unwritten;
// retain the caller's defined storage for those bytes instead of reading uninitialized C++.
void initialize(nativeeffects::Effect &record, const Candidate &candidate);
bool append(const Candidate &candidate, std::list<nativeeffects::Effect> &live, std::vector<nativeeffects::Effect> &outgoing);
} // namespace residentfx
