#pragma once
#include <algorithm>
#include <cmath>
#include <limits>

namespace encounters::health {
inline constexpr unsigned Named = 1u << 9;
inline constexpr unsigned Mini = 1u << 17;
inline constexpr unsigned Boss = 1u << 18;
inline constexpr double GuardianMultiplier = 3.0;

// Steam's maximum-HP formula applies these flags independently before mod
// callbacks. A named boss therefore receives 10 * 15 times ordinary HP.
inline constexpr double nativeBossMultiplier(unsigned flags) {
    return ((flags & Named) ? 10.0 : 1.0) * ((flags & Mini) ? 1.5 : 1.0) *
           ((flags & Boss) ? 15.0 : 1.0);
}

inline float normalizedMaximumHP(float reportedHP, unsigned flags, bool guardian) {
    if (!guardian)
        return reportedHP;
    if (!std::isfinite(reportedHP) || reportedHP <= 0.f)
        return 1.f;

    // Replace only the native boss multiplier. Numeric progression and other
    // multiplicative HP callbacks remain part of the ordinary creature's HP.
    double corrected = double(reportedHP) * GuardianMultiplier / nativeBossMultiplier(flags);
    return float(std::clamp(corrected, double(std::numeric_limits<float>::min()),
                            double(std::numeric_limits<float>::max())));
}
} // namespace encounters::health
