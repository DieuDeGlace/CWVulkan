#include "SteamSky.h"
#include "GameMath.h"
#include "worldgen/Noise.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace steamsky {
namespace {
constexpr double PI = 3.14159265358979323846;
constexpr double DAY_MS = 86400000.0;

// The three sky colours at one key of the day.
struct Key {
    glm::vec3 sky1, sky2, fog;
};
glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t) { return a * (1 - t) + b * t; }
Key lerp(const Key &a, const Key &b, float t) { return {lerp(a.sky1, b.sky1, t), lerp(a.sky2, b.sky2, t), lerp(a.fog, b.fog, t)}; }

// §B.3 key colours before the region modifiers.
const Key DAY{{0, .6f, 1}, {0, 0, .8f}, {0, .1f, 1}};
const Key DUSK_B{{1, .95f, .5f}, {.3f, .5f, 1}, {.7f, .6f, .3f}}; // L 0.6
const Key DUSK_A{{1, .5f, .1f}, {.5f, .2f, .8f}, {1, .5f, .2f}};  // L 0.2
const Key NIGHT{{0, .03f, .15f}, {0, 0, 0}, {0, .1f, .1f}};
const glm::vec3 DAY_SUN{1.5f, 1.3f, .4f}, DUSK_SUN{1.5f, .85f, .25f};

// Where the keys sit on the palette daylight L.
constexpr float DAY_FROM = .75f, DUSK_B_AT = .6f, DUSK_A_AT = .2f;
// Below dusk A the night fades in four times as fast: f is 0 until L = 0.15.
constexpr float NIGHT_STEEPNESS = 4;

// Region thresholds.
constexpr float SEA_LEVEL_BAND = 20;            // blocks: warmth and dryness fade out below this
constexpr float MOIST_FROM = .5f;
constexpr float HOT_FROM = .8f, DRY_BELOW = .4f;
constexpr float WEIGHT_FROM = .001f;             // lava, forest and kind 27 count above this

// §B.4 easing rates (the frame blend's c).
constexpr float FOG_RATE = .0005f, SUN_RATE = .002f;
// Long pauses are eased as one tenth of a second (the blend loops per hundredth of a ms).
constexpr double LONGEST_EASE_MS = 100;

// §B.5 sun path and SunPos.
constexpr float SUN_LEAN = -.4f;
constexpr float SUN_DISTANCE = 100;

// §B.6 stars.
constexpr float STAR_FLOOR = .05f, STARS_BELOW = .4f, STAR_FADE = 4, FOREST_DIMMING = .75f;
constexpr int RING_STRIDE = 57, RING_LIMIT = 2850;
constexpr int RING_STARS = 50;        // stars on the horizon ring, fewer higher up
constexpr float RING_THINNING = 7;    // n = trunc(50 - 7 sqrt(r))
constexpr float RING_TWIST = 13;      // degrees each ring is turned against the last
constexpr float TILT_PER_RING = 90.f / 50;
constexpr int DOME_UNIT = 64, DOME_INSET = 10;
} // namespace

Palette palette(float L, const Region &region, bool underwater) {
    Key day = DAY, duskB = DUSK_B, duskA = DUSK_A, night = NIGHT;
    float W = region.warmth, D = region.dryness;

    // 1. Near sea level warmth and dryness fade out.
    if (region.ground < SEA_LEVEL_BAND) {
        const float k = std::max(region.ground / SEA_LEVEL_BAND, 0.f);
        D *= k;
        W *= k;
    }
    // 2. Moist ground: a greener, less blue day.
    if (D > MOIST_FROM) {
        const float u = 2 * (D - MOIST_FROM);
        day.sky1 += glm::vec3(0, 0, -.2f) * u;
        day.sky2 += glm::vec3(0, .5f, -.3f) * u;
        day.fog += glm::vec3(0, .5f, -.35f) * u;
    }
    // 3. Hot and dry: a pale sky and a pink haze.
    if (W > HOT_FROM && D < DRY_BELOW) {
        const float k = std::min((1 - D / DRY_BELOW) * ((W - HOT_FROM) / (1 - HOT_FROM)) * 4, 1.f);
        day.sky1 = lerp(day.sky1, {.6f, .8f, 1}, k);
        day.sky2 = lerp(day.sky2, {.1f, .4f, 1}, k);
        day.fog = lerp(day.fog, {.8f, .5f, .7f}, k);
    }
    // 4. Lava: red skies at every time of day (the night's upper colour stays black).
    if (region.lava > WEIGHT_FROM) {
        const float k = region.lava;
        day = {lerp(day.sky1, {1, 0, 0}, k), lerp(day.sky2, {.4f, .2f, .2f}, k), lerp(day.fog, {.3f, 0, 0}, k)};
        duskB = {lerp(duskB.sky1, {.8f, .1f, 0}, k), lerp(duskB.sky2, {.2f, .2f, .2f}, k), lerp(duskB.fog, {.3f, 0, 0}, k)};
        duskA = {lerp(duskA.sky1, {.8f, .1f, 0}, k), lerp(duskA.sky2, {.2f, .2f, .2f}, k), lerp(duskA.fog, {.3f, .1f, 0}, k)};
        night.sky1 = lerp(night.sky1, {.06f, 0, .03f}, k);
        night.fog = lerp(night.fog, {.1f, 0, 0}, k);
    }
    // 5. Forest regions: green and purple hazes.
    if (region.forest > WEIGHT_FROM) {
        const float k = region.forest;
        day = {lerp(day.sky1, {.2f, .6f, .4f}, k), lerp(day.sky2, {0, .1f, .2f}, k), lerp(day.fog, {.4f, .1f, .8f}, k)};
        duskB = {lerp(duskB.sky1, {.1f, .6f, .3f}, k), lerp(duskB.sky2, {0, .05f, .1f}, k), lerp(duskB.fog, {.4f, .1f, .8f}, k)};
        duskA = {lerp(duskA.sky1, {.3f, .1f, .4f}, k), lerp(duskA.sky2, {.2f, .2f, .2f}, k), lerp(duskA.fog, {.4f, .2f, .5f}, k)};
        night.fog = lerp(night.fog, {.1f, .15f, .2f}, k);
    }
    // 6. Places of kind 27: pink and blue skies by day and at the later dusk.
    if (region.kind27 > WEIGHT_FROM) {
        const float k = region.kind27;
        day = {lerp(day.sky1, {1, .6f, 1}, k), lerp(day.sky2, {0, .6f, 1}, k), lerp(day.fog, {.6f, .3f, 1}, k)};
        duskB = {lerp(duskB.sky1, {.8f, .2f, 1}, k), lerp(duskB.sky2, {0, .5f, 1}, k), lerp(duskB.fog, {.6f, .3f, 1}, k)};
        duskA.sky2 = lerp(duskA.sky2, {.1f, .5f, .8f}, k);
    }
    // (7. The far flight's sky is not drawn: CubeVulkan's eagle flight does not report it yet.)

    // Interpolate over L.
    Key sky;
    Palette p;
    if (L > DAY_FROM) {
        sky = day;
        p.sun = DAY_SUN;
    } else if (L > DUSK_B_AT) {
        const float t = (L - DUSK_B_AT) / (DAY_FROM - DUSK_B_AT);
        sky = lerp(duskB, day, t);
        p.sun = lerp(DUSK_SUN, DAY_SUN, t);
    } else if (L > DUSK_A_AT) {
        const float t = (L - DUSK_A_AT) / (DUSK_B_AT - DUSK_A_AT);
        sky = lerp(duskA, duskB, t);
        p.sun = DUSK_SUN;
    } else {
        const float f = std::max(1 - (1 - L / DUSK_A_AT) * NIGHT_STEEPNESS, 0.f);
        sky = lerp(night, duskA, f);
        p.sun = DUSK_SUN * f;
    }
    p.sky1 = sky.sky1;
    p.sky2 = sky.sky2;
    p.fog = sky.fog;
    // Under water the whole sky takes the upper colour.
    if (underwater)
        p.sky1 = p.fog = p.sky2;
    return p;
}

void EasedPalette::update(const Palette &target, double elapsedMs) {
    if (!primed) {
        current = target;
        primed = true;
        return;
    }
    const double ms = std::clamp(elapsedMs, 0.0, LONGEST_EASE_MS);
    auto ease = [&](glm::vec3 &value, glm::vec3 to, float rate) {
        const glm::vec3 difference = to - value;
        value = difference * gamemath::blendFactor(ms, rate) + value;
    };
    current.sky1 = target.sky1;
    current.sky2 = target.sky2;
    ease(current.fog, target.fog, FOG_RATE);
    ease(current.sun, target.sun, SUN_RATE);
}

glm::vec3 sunDirection(double timeOfDayMs) {
    const float a = (float)(2 * timeOfDayMs * PI / DAY_MS);
    return glm::normalize(glm::vec3(std::sin(a), SUN_LEAN * std::sin(a / 2), -std::cos(a)));
}

glm::vec4 sunPosition(glm::vec3 sunDirection, const glm::mat4 &view, const glm::mat4 &projection, float aspect,
                      float gradientShift) {
    const glm::vec3 inView = glm::mat3(view) * (SUN_DISTANCE * sunDirection);
    const glm::vec4 clip = projection * glm::vec4(inView, 1);
    if (clip.w <= 0)
        return {0, 0, aspect, 0};
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    const bool visible = ndc.z >= 0 && ndc.z <= 1;
    // Vulkan's clip y points down the screen; the shader's y points up.
    return {ndc.x, -ndc.y + gradientShift, aspect, visible ? 1.f : 0.f};
}

float starValue(double timeOfDayMs, float forest) {
    const float x = (float)(2 * timeOfDayMs / DAY_MS) - 1;
    const float base = std::min(std::max(2 * std::pow(1 - x * x, 4.f), STAR_FLOOR), 1.f);
    return base * (1 - FOREST_DIMMING * forest);
}

bool starsShown(float value) { return value < STARS_BELOW; }

float starOpacity(float value) { return std::min((1 - value / STARS_BELOW) * STAR_FADE, 1.f); }

std::vector<Star> starDome() {
    std::vector<Star> stars;
    for (int ring = 0; RING_STRIDE * ring < RING_LIMIT; ring++) {
        const int count = (int)std::trunc(RING_STARS - RING_THINNING * std::sqrt((float)ring));
        for (int j = 0; j < count; j++) {
            // The generator's 0..100000 random number of the place: only even ones hold a star.
            uint32_t b = worldgen::random((uint32_t)(RING_STRIDE * ring + j));
            if (b & 1u)
                continue;
            b /= 2;
            Star s;
            s.turnDegrees = 360.f * j / count + RING_TWIST * ring;
            s.tiltDegrees = TILT_PER_RING * ring;
            s.scale = 1 + (float)(b % 1000) / 500;
            stars.push_back(s);
        }
    }
    return stars;
}

glm::mat4 starMatrix(const Star &star, int radiusUnits) {
    const float radius = (float)(DOME_UNIT * radiusUnits - DOME_INSET);
    glm::mat4 m = glm::rotate(glm::mat4(1), glm::radians(star.turnDegrees), glm::vec3(0, 0, 1));
    m = glm::rotate(m, glm::radians(star.tiltDegrees), glm::vec3(1, 0, 0));
    m = glm::translate(m, glm::vec3(0, radius, 0));
    return glm::scale(m, glm::vec3(star.scale));
}

} // namespace steamsky
