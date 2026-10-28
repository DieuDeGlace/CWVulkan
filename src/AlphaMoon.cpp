#include "AlphaMoon.h"
#include "AlphaLook.h"
#include <algorithm>
#include <cmath>

namespace alpha {
namespace {
// 0 below `edge0`, 1 above `edge1`, an S-curve between.
float ease(float edge0, float edge1, float x) {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return t * t * (3 - 2 * t);
}

// The moon's colours at full strength, before the settings scale them: a cool blue-white
// for the light from the moon, a dimmer blue for the faces turned away from it, and a deep
// blue for the ambient floor.
const glm::vec3 MOON_FRONT(.55f, .65f, .9f);
const glm::vec3 MOON_BACK(.12f, .15f, .24f);
const glm::vec3 NIGHT_AMBIENT(.16f, .24f, .55f);

// The moon (and its shadows) fade in over its first stretch above the horizon.
constexpr float RISE = .25f; // height of the moon's direction (sine of its elevation)
} // namespace

glm::vec3 moonDirection(double timeOfDayMs) { return -sunDirection(timeOfDayMs); }

float moonStrength(double timeOfDayMs) {
    // The darker the day, the stronger the moon; then held back while it is near the horizon.
    float dark = 1 - sunlight(timeOfDayMs);
    float above = ease(0, RISE, moonDirection(timeOfDayMs).z);
    return ease(0, 1, dark) * above;
}

MoonLight moonLight(double timeOfDayMs, const MoonSettings &settings) {
    MoonLight m;
    if (!settings.enabled)
        return m;
    m.strength = moonStrength(timeOfDayMs);
    m.front = MOON_FRONT * settings.brightness * m.strength;
    m.back = MOON_BACK * settings.brightness * m.strength;
    m.ambient = NIGHT_AMBIENT * settings.nightAmbient * m.strength;
    return m;
}

glm::vec3 moonOnVertex(const MoonLight &moon, glm::vec3 normal, glm::vec3 sunDirection, float bakedLight) {
    float towards = std::max(glm::dot(normal, -sunDirection), 0.f);
    float away = std::max(glm::dot(normal, sunDirection), 0.f);
    return (towards * moon.front + away * moon.back + moon.ambient) * bakedLight;
}

ShadowLight shadowLight(double timeOfDayMs, float sunShadow, const MoonSettings &settings) {
    ShadowLight s;
    glm::vec3 sun = sunDirection(timeOfDayMs);
    if (sun.z >= 0 || !settings.enabled) {
        // By day the sun casts, fading out as it nears the horizon (Alpha's sun has no
        // shadows of its own; these are CubeVulkan's).
        s.direction = sun;
        s.strength = sunShadow * ease(0, .2f, sun.z);
        return s;
    }
    // By night the moon casts, fading in the same way as it climbs: at the moment the sun
    // touches the horizon both are 0.
    s.direction = -sun;
    s.strength = settings.shadowStrength * ease(0, RISE, -sun.z);
    s.moon = true;
    return s;
}

float moonDiscVisibility(double timeOfDayMs) {
    float dark = 1 - sunlight(timeOfDayMs);
    float height = moonDirection(timeOfDayMs).z;
    return ease(.2f, .7f, dark) * ease(-.05f, .05f, height);
}

float moonPhase(int worldDay) {
    constexpr int CYCLE = 8;
    int day = ((worldDay % CYCLE) + CYCLE) % CYCLE;
    return (float)day / CYCLE;
}

} // namespace alpha
