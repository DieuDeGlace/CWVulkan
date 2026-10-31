#pragma once
// Independent gameplay-camera §§5–6. Positions remain in native fixed units.
#include "TerrainRay.h"
#include <optional>

namespace cameraposition {
using Point = std::array<int64_t, 3>;
struct Mount {
    float height = 0, offsetZ = 0;
    bool alive = false;
};
struct Subject {
    Point physical{};
    std::array<float, 3> size{1, 1, 2}, velocity{};
    float stepOffset = 0;
    uint8_t ability = 0;
    bool creationLayerShown = false;
    std::optional<Mount> mount;
};
struct State {
    Point lookAt{}, eye{};
    float distance = 8; // Already eased toward the separate requested distance.
};
// The native candidate/probe phase can cancel an invalid mount on the subject.
Point candidate(Subject &subject, const terrainray::World &world);
// Retained anchor phase, distinct from current camera-angle/distance smoothing.
void follow(State &state, const Subject &subject, const Point &candidate, double frameMs);
// Runs candidate, retained anchor, orbit and all eight-corner terrain checks.
void update(State &state, Subject &subject, const std::array<float, 3> &forward, double frameMs, const terrainray::World &world);
} // namespace cameraposition
