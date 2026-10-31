#pragma once
#include <cstdint>

// Independent steam-creature-drawn-interpolation.md, ordinary finite inputs.
namespace drawnstate {
// Pass A only. Caller visits every registered record before starting linked poses.
void update(uint8_t *record, float frameMs);
} // namespace drawnstate
