#pragma once
#include <cstdint>
#include <functional>

// Independent steam-creature-linked-placement.md, finite ordinary distinct actors.
namespace linkedplacement {
struct World {
    std::function<uint8_t *(int64_t)> byId;
    std::function<int(const uint8_t *)> duration;
    std::function<void(int64_t)> focusFailure;
};
// Pass B. Resolve and edit current live records; no outer HP or visibility gate.
void update(World &world, uint8_t *creature, float frameMs);
} // namespace linkedplacement
