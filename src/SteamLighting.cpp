#include "SteamLighting.h"
#include "GameMath.h"
#include <algorithm>
#include <cmath>

namespace steamlighting {
namespace {
constexpr float DAY_MS = 86400000.0f;
constexpr double PI = 3.14159265358979323846;
glm::vec4 weighted(glm::vec4 old, glm::vec4 target, float weight) {
    return target * weight + old * (1.0f - weight);
}
struct Endpoints {
    glm::vec4 low, middle, high;
    glm::vec4 sample(float day) const {
        if (day <= .5f) {
            const float weight = 2 * day;
            return low * (1 - weight) + middle * weight;
        }
        float inverse = 1 - 2 * (day - .5f);
        inverse *= inverse;
        inverse *= inverse;
        const float weight = 1 - inverse;
        return high * weight + middle * (1 - weight);
    }
    void blend(const Endpoints &target, float weight) {
        low = weighted(low, target.low, weight);
        middle = weighted(middle, target.middle, weight);
        high = weighted(high, target.high, weight);
    }
};
}

float frameMilliseconds(double elapsedMs) { return (float)std::min(elapsedMs, 1000.0); }

float dayCoordinate(float timeMs) {
    const float x = 2 * timeMs / DAY_MS - 1;
    return std::pow(1 - std::pow(std::abs(x), 3.0f), 4.0f);
}

float collectorDaylight(float timeMs, float forest) {
    const float x = 2 * timeMs / DAY_MS - 1;
    const float day = std::clamp(2 * std::pow(1 - x * x, 4.0f), .05f, 1.0f);
    return day * (1 - .75f * forest); // deliberately no second clamp
}

glm::vec3 sunDirection(float timeMs) {
    const float angle = (float)((double)(timeMs + timeMs) * PI / (double)DAY_MS);
    return glm::normalize(glm::vec3(std::sin(angle), -.4000000059604645f * std::sin(.5f * angle), -std::cos(angle)));
}

Colours targets(float timeMs, const Environment &environment) {
    Endpoints front{{.04f, .05f, .1f, 1}, {.65f, .6f, .15f, 1}, {.75f, .7f, .3f, 1}};
    Endpoints ambient{{.05f, .075f, .1f, 1}, {.15f, .2f, .75f, 1}, {.25f, .3f, .7f, 1}};
    const Endpoints back{{.05f, .05f, .3f, 1}, {.25f, .25f, .25f, 1}, {.5f, .5f, .5f, 1}};
    const auto &region = environment.region;
    float dryness = region.dryness;
    if (region.ground < 20)
        dryness *= std::max(region.ground / 20, 0.0f);
    if (environment.blockFlag20) {
        const bool cool = environment.structureKind == 0x2b || environment.structureKind == 0x2c;
        const glm::vec4 colour = cool ? glm::vec4(.1f, .3f, .4f, 1) : glm::vec4(.25f, .1f, .5f, 1);
        ambient.high = colour;
        ambient.middle = colour * .9f;
        front.high = glm::vec4(1.5f, 1.5f, 1.5f, 1) - colour;
        front.middle = glm::vec4(1) - colour * .9f;
    }
    if (dryness < .3f) {
        const float weight = 1 - dryness / .3f;
        ambient.high += (glm::vec4(.3f, .3f, .2f, 1) - ambient.high) * weight;
        front.high = glm::vec4(1) - ambient.high;
    }
    if (region.lava > .001f) {
        front.blend({{1, .8f, .1f, 1}, {1, 1, 0, 1}, {1.1f, 1, .1f, 1}}, region.lava);
        ambient.blend({{.1f, .05f, 0, 1}, {.3f, .2f, .1f, 1}, {.4f, .3f, .2f, 1}}, region.lava);
    }
    if (region.forest > .001f) {
        const float weight = region.forest;
        front.middle = weighted(front.middle, {.2f, .4f, .4f, 1}, weight);
        front.high = weighted(front.high, {.2f, .2f, .9f, 1}, weight);
        ambient.middle = weighted(ambient.middle, {.2f, .2f, .3f, 1}, weight);
        ambient.high = weighted(ambient.high, {.2f, .2f, .3f, 1}, weight);
        ambient.low = weighted(ambient.low, {.1f, .1f, .2f, 1}, weight);
        ambient.low = weighted(ambient.low, {0, .01f, .01f, 1}, weight);
    }
    if (region.kind27 > .001f) {
        front.middle = weighted(front.middle, {.9f, .7f, .6f, 1}, region.kind27);
        front.high = weighted(front.high, {.9f, .7f, .6f, 1}, region.kind27);
        ambient.middle = weighted(ambient.middle, {.3f, .5f, .5f, 1}, region.kind27);
        ambient.high = weighted(ambient.high, {.3f, .5f, .5f, 1}, region.kind27);
    }
    const float day = dayCoordinate(timeMs);
    Colours result{front.sample(day), back.sample(day), ambient.sample(day)};
    result.ambient.w = day;
    if (environment.effect13RemainingMs) {
        const float weight = std::min(1.0f, (60000 - *environment.effect13RemainingMs) / 3000);
        result.front = weighted(result.front, {.05f, .1f, 1, 1}, weight);
        result.back = weighted(result.back, {.05f, .1f, 1, 1}, weight);
        result.ambient = weighted(result.ambient, {.01f, .05f, .2f, 1}, weight);
    }
    return result;
}

glm::vec4 material(bool underwater) { return underwater ? glm::vec4(.5f, .5f, 1, 1) : glm::vec4(1); }

void EasedColours::reset() {
    current = {{.7f, .7f, .3f, 1}, {.5f, .5f, .5f, 1}, {.3f, .3f, .7f, 1}};
}

void EasedColours::update(const Colours &target, float elapsedMs) {
    // Invalid CVTTSD2SI input produces a nonpositive native step count. Avoid
    // undefined C++ conversion while retaining that no-update result.
    if (!std::isfinite(elapsedMs) || elapsedMs <= 0 || (double)elapsedMs * 100 >= 2147483648.0)
        return;
    const float weight = gamemath::blendFactor((double)elapsedMs, .0010000000474974513f);
    current.front += (target.front - current.front) * weight;
    current.back += (target.back - current.back) * weight;
    current.ambient += (target.ambient - current.ambient) * weight;
}
} // namespace steamlighting
