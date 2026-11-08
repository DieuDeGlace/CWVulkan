#pragma once
// Steam World clock, docs/specs/steam-world-clock-units-and-update-20261108.md.
// References and actor views are borrowed only for the synchronous call. The scan
// finishes before reset can destroy actors; no actor pointer survives a reset.
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace nativeai { struct Creature; }
namespace nativeclock {
struct World {
    int32_t &day;
    float &timeOfDayMs;
    float &elapsedMs;
    const bool &client; // re-read for every strict crossing
    bool named;
    std::vector<int32_t> &notices;
    std::span<nativeai::Creature *const> actors;
    uint8_t *local = nullptr;
    std::function<float()> maxHealth; // fresh local-player reference on each call
    std::function<void()> reset;     // actor cleanup, then retained metadata reset
};
void advance(World &world, float stepMs);
} // namespace nativeclock
