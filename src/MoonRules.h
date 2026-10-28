// Copied unchanged from MoonandShadow/MoonLight/MoonRules.h (MIT, DieuDeGlace)
// so CubeVulkan's day/night and shadow fades match MoonandShadow.
#pragma once
// Pure day/night rules shared by MoonLight and SteamShadows (and their tests).
#include <cmath>
#include <algorithm>
// Shared with other mods through QueryMoonLight(); keep the layout stable.
struct MoonState {
    unsigned version = 1;
    float weight = 0, direction[3]{}, shadowStrength = .20f;
};
namespace moonrules {
inline float smooth(float x) {
    x = std::clamp(x, 0.f, 1.f);
    return x * x * (3 - 2 * x);
}
// Moon for a game time in milliseconds (24 h cycle). The moon rises at 18:00
// and sets at 06:00; `weight` fades in/out over the first/last two hours, and
// `direction` is a unit vector towards the moon.
inline MoonState state(float milliseconds, float shadowStrength = .20f) {
    MoonState s;
    s.shadowStrength = shadowStrength;
    if (!std::isfinite(milliseconds))
        return s;
    float hour = std::fmod(milliseconds / 3600000.f, 24.f);
    if (hour < 0)
        hour += 24;
    s.weight = hour >= 18 ? smooth((hour - 18) / 2) : hour <= 6 ? smooth((6 - hour) / 2) : 0;
    float angle = ((hour < 18 ? hour + 24 : hour) - 18) * (3.141592653589793f / 12);
    s.direction[0] = std::cos(angle) * .94f;
    s.direction[1] = std::cos(angle) * .341174f;
    s.direction[2] = std::sin(angle);
    float n = std::sqrt(s.direction[0] * s.direction[0] + s.direction[1] * s.direction[1] +
                        s.direction[2] * s.direction[2]);
    for (float &v : s.direction)
        v /= n;
    return s;
}
// Unit direction a fraction `t` of the way from unit `a` to unit `b` along the
// sky. Nearly opposite directions pass overhead instead of through zero.
inline void blendDirection(float out[3], const float a[3], const float b[3], float t) {
    auto slerp = [](float *o, const float *x, const float *y, float u) {
        float d = std::clamp(x[0] * y[0] + x[1] * y[1] + x[2] * y[2], -1.f, 1.f);
        float angle = std::acos(d), s = std::sin(angle);
        float wx = s > 1e-4f ? std::sin((1 - u) * angle) / s : 1 - u;
        float wy = s > 1e-4f ? std::sin(u * angle) / s : u;
        for (int i = 0; i < 3; i++)
            o[i] = wx * x[i] + wy * y[i];
    };
    float mid[3] = {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
    float n = std::sqrt(mid[0] * mid[0] + mid[1] * mid[1] + mid[2] * mid[2]);
    if (n < .1f) { // Opposite: go through straight up.
        mid[0] = mid[1] = 0;
        mid[2] = 1;
    } else
        for (float &v : mid)
            v /= n;
    if (t < .5f)
        slerp(out, a, mid, t * 2);
    else
        slerp(out, mid, b, t * 2 - 1);
}
// Night version of the native light buffer (12 floats): [0..2] front colour,
// [4..6] back colour, [8..10] light direction, turned towards the moon as it
// fades in (no jump at moonrise or moonset).
inline void lighting(float out[12], const float native[12], MoonState state, float amount) {
    std::copy(native, native + 12, out);
    float w = state.weight * amount;
    const float front[] = {.32f, .38f, .5f}, back[] = {.07f, .085f, .12f};
    for (int i = 0; i < 3; i++) {
        out[i] = std::clamp(out[i] + front[i] * w, 0.f, 1.f);
        out[4 + i] = std::clamp(out[4 + i] + back[i] * w, 0.f, 1.f);
    }
    float n = std::sqrt(out[8] * out[8] + out[9] * out[9] + out[10] * out[10]);
    if (n > .001f) {
        for (int i = 8; i < 11; i++)
            out[i] /= n;
        float nativeDirection[3] = {out[8], out[9], out[10]};
        if (state.weight > 0)
            blendDirection(out + 8, nativeDirection, state.direction, state.weight);
    } else if (state.weight > 0)
        std::copy(state.direction, state.direction + 3, out + 8);
}
// Darkens and cools a cloud's material colour at night.
inline void cloud(float out[4], const float native[4], float weight, float nightScale) {
    std::copy(native, native + 4, out);
    const float tint[] = {.88f, .94f, 1.f};
    for (int i = 0; i < 3; i++)
        out[i] *= 1 - weight + weight * nightScale * tint[i];
}
} // namespace moonrules

namespace moonrules {
struct ShadowCycle {
    float direction[3]{}, strength = 0;
};
// Shadow strength factor for a light at elevation sin(angle) `z`: no shadows
// while the light is within about 4.6 degrees of the horizon (z < 0.08, about
// 18 game minutes either side of 06:00 and 18:00), full from about 17.5
// degrees (z = 0.30) up. Shadows fade out before sunset, stay gone through
// dusk/dawn while the light switches between sun and moon, then fade back in.
inline float horizonFade(float z) {
    return smooth((z - .08f) / .22f);
}
// Which light casts shadows: the sun while it is above the horizon, otherwise
// the moon; strength fades out near the horizon.
inline ShadowCycle shadows(float milliseconds, float dayStrength, float nightStrength) {
    auto moon = state(milliseconds, nightStrength);
    ShadowCycle out;
    bool day = moon.direction[2] < 0;
    for (int i = 0; i < 3; i++)
        out.direction[i] = day ? -moon.direction[i] : moon.direction[i];
    float horizon = horizonFade(out.direction[2]);
    out.strength = day ? dayStrength * horizon : nightStrength * moon.weight * horizon;
    return out;
}
} // namespace moonrules
