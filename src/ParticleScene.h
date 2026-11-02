#pragma once
// Particle WORLD placement from steam-particle-bursts.md section7.2.
#include "NativeParticles.h"
struct DrawItem;
namespace particlescene {
struct Coordinates {
    std::array<int64_t, 2> homeOffset{};
    float homeZ = 0;
    // Present for Steam: the current installed scene's negative fixed look-at XY.
    std::optional<std::array<int64_t, 2>> renderOffset;
};
DrawItem draw(const nativeparticles::Particle &, uint32_t mesh, glm::ivec3 dimensions, const Coordinates &);
} // namespace particlescene
