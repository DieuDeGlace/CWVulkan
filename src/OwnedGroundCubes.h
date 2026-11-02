#pragma once
// The per-frame owned-item cube list, from steam-owned-ground-cubes.md.
#include "GroundItemScene.h"
#include <array>
#include <optional>
#include <vector>

namespace ownedground {
struct Source {
    std::array<int64_t, 3> position{};
    glm::vec4 first{1}, second{1};
    uint32_t clock = 0;
    float radius = 0, size = .1f;
    int32_t count = 8;
};
// Called after ordinary ground-record/resource admission. These are the extra producer gates.
std::optional<Source> source(const groundscene::Record &, const nativesaves::Item &adapted, bool personalData, int64_t localOwner,
                             float worldMs, glm::vec4 rarityColor);
struct Cube {
    glm::mat4 model{1}; // native corner-origin WORLD, including the current fixed XY render offset
    glm::vec4 material{1};
};
// Full allocation dimensions, not occupied bounds. No additional per-cube admission.
std::vector<Cube> prepare(const Source &, glm::ivec3 header, std::array<int64_t, 2> renderOffset);
} // namespace ownedground
