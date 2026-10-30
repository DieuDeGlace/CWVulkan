#pragma once
// Alpha static draw staging, from specs/alpha-far-static-ordering.md.
#include <array>
#include <cstdint>

namespace alphascenery {
using Point = std::array<int64_t, 3>;
enum class Kind { Model, Object };
struct Placement {
    Kind kind = Kind::Model;
    Point point{}; // fixed render XY/game Z; models use the sprite's raised center
};
struct Fade {
    float squared = 0, end = 0, full = 0;
    bool afterWater() const;
    float opacity() const;
};
// Admission belongs to the caller. These rules add no new range/frustum rejection.
Fade sample(const Placement &, const Point &camera, float renderRadius, float fogRange);
} // namespace alphascenery
