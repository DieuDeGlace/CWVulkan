#pragma once
#include <algorithm>
#include <cmath>

namespace encounters::difficulty {
// Native stars and Pyro region levels remain authoritative. Add a small,
// bounded encounter premium instead of replacing either progression system.
inline float health(unsigned level) {
    return 1.f + std::min(.40f, .02f * std::sqrt(float(std::max(1u, level) - 1)));
}
inline float damage(unsigned level) {
    return 1.f + std::min(.25f, .0125f * std::sqrt(float(std::max(1u, level) - 1)));
}
} // namespace encounters::difficulty
