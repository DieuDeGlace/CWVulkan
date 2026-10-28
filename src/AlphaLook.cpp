#include "AlphaLook.h"
#include <algorithm>
#include <atomic>
#include <cmath>

namespace {
std::atomic<RenderStyle> currentStyle{RenderStyle::Alpha};
} // namespace

RenderStyle renderStyle() { return currentStyle.load(std::memory_order_relaxed); }
void setRenderStyle(RenderStyle style) { currentStyle.store(style, std::memory_order_relaxed); }

namespace alpha {
namespace {
constexpr double PI = 3.14159265358979323846;

// The palette's blends are written as two products and a sum (§3.4).
glm::vec3 blend(glm::vec3 a, float ka, glm::vec3 b, float kb) { return a * ka + b * kb; }

// One key colour and what fire ground turns it into (§3.2 step 4, §3.3).
glm::vec3 withFire(glm::vec3 key, glm::vec3 fire, float share) { return key + (fire - key) * share; }

// The three sky colours at one key of the day.
struct SkyKey {
    glm::vec3 sky1, sky2, fog;
};
} // namespace

float dayPosition(double timeOfDayMs) { return (float)(2 * timeOfDayMs / DAY_MS) - 1; }

float paletteDaylight(double timeOfDayMs) {
    float x = dayPosition(timeOfDayMs);
    float cube = (float)std::pow(std::abs((double)x), 3.0);
    return (float)std::pow((double)(1 - cube), 4.0);
}

float sunlight(double timeOfDayMs) {
    float x = dayPosition(timeOfDayMs);
    float fourth = (float)std::pow((double)x, 4.0);
    return (float)std::pow((double)(1 - fourth), 5.0);
}

glm::vec3 sunDirection(double timeOfDayMs) {
    float angle = (float)((double)((float)timeOfDayMs * 2.f) * PI / DAY_MS);
    return glm::normalize(glm::vec3(0, -std::sin(angle), -std::cos(angle)));
}

Palette palette(float L, const ZoneFactors &zone, bool underwater) {
    const float wm = zone.warmth, mo = zone.moisture, se = zone.sea, fi = zone.fire;

    // §3.2: the day keys and the zone modifiers, in order.
    SkyKey day{{.1f, .6f, 1}, {.01f, .05f, 1}, {.01f, .1f, 1}};
    if (wm > .6f && mo < .4f) { // hot and dry: a pale, sandy haze
        float w = (1 - mo / .4f) * ((wm - .6f) / .4f);
        day.fog = {.01f + .79f * w, .1f + .6f * w, 1 - .2f * w};
    }
    if (wm > .3f && mo > .6f) { // warm and wet: green haze and sky
        float m = std::min((wm - .3f) / .7f * 4, 1.f);
        float w = (1 - se) * ((mo - .6f) * m / .4f);
        day.fog = day.fog + (glm::vec3(0, .7f, .5f) - day.fog) * w;
        day.sky1 = {.1f + .4f * w, .6f + .3f * w, 1};
        day.sky2 = {.01f - .01f * w, .05f + .55f * w, 1 - .1f * w};
    }
    if (mo < .4f) { // dry: a paler sky
        float u = 1 - mo / .4f;
        day.sky1 = day.sky1 + (glm::vec3(.6f, .8f, 1) - day.sky1) * u;
        day.sky2 = day.sky2 + (glm::vec3(.1f, .4f, 1) - day.sky2) * u;
    }

    // §3.3: night and the two twilight keys; fire ground pulls every key towards red.
    SkyKey night{{.05f, 0, .1f}, {0, 0, .1f}, {0, 0, .1f}};
    SkyKey duskA{{1, .3f, .3f}, {.5f, 0, .5f}, {.5f, .2f, .6f}};
    SkyKey duskB{{1, .9f, .5f}, {.2f, .1f, .8f}, {1, .9f, .8f}};
    if (fi > 0) {
        day = {withFire(day.sky1, {1, 0, 0}, fi), withFire(day.sky2, {.4f, .2f, .2f}, fi),
               withFire(day.fog, {.3f, 0, 0}, fi)};
        duskA = {withFire(duskA.sky1, {.8f, .1f, 0}, fi), withFire(duskA.sky2, {.2f, .2f, .2f}, fi),
                 withFire(duskA.fog, {.3f, .1f, 0}, fi)};
        duskB = {withFire(duskB.sky1, {.8f, .1f, 0}, fi), withFire(duskB.sky2, {.2f, .2f, .2f}, fi),
                 withFire(duskB.fog, {.3f, 0, 0}, fi)};
        night.fog = withFire(night.fog, {.3f, .2f, .1f}, fi);
    }

    // §3.4: interpolate the keys over L.
    SkyKey sky;
    auto mixKeys = [&](const SkyKey &a, float ka, const SkyKey &b, float kb) {
        return SkyKey{blend(a.sky1, ka, b.sky1, kb), blend(a.sky2, ka, b.sky2, kb), blend(a.fog, ka, b.fog, kb)};
    };
    if (L > .75f)
        sky = day;
    else if (L > .6f) {
        float s = (L - .6f) / .15f;
        sky = mixKeys(day, s, duskB, 1 - s);
    } else if (L > .4f) {
        float r = (L - .4f) / .2f;
        sky = mixKeys(duskB, r, duskA, 1 - r);
    } else {
        float r = L / .4f;
        sky = mixKeys(duskA, r, night, 1 - r);
    }

    Palette p;
    p.sky1 = sky.sky1;
    p.sky2 = sky.sky2;
    p.fog = sky.fog;
    // §3.5: under water the whole sky becomes a dim blue; the fog colour stays.
    if (underwater) {
        glm::vec3 blue((float)(.1 * L), (float)(.15 * L), L);
        p.sky1 = p.sky2 = blue;
    }

    // §3.6: the sun (front) and back colours, keyed at L = 0, 0.5 and 1.
    glm::vec3 front0 = withFire({.01f, .02f, .01f}, {1, .8f, .1f}, fi);
    glm::vec3 front5 = withFire({1, .9f, .6f}, {.8f, .8f, 0}, fi);
    glm::vec3 front1 = withFire({1.1f, 1.1f, .9f}, {1, .8f, .1f}, fi);
    glm::vec3 back0 = withFire({0, .02f, .03f}, {.7f, .3f, .3f}, fi);
    glm::vec3 back5 = withFire({.6f, .5f, 1}, {.8f, .4f, .4f}, fi);
    glm::vec3 back1 = withFire({.5f, .6f, .8f}, {1, .5f, .5f}, fi);
    if (L <= .5f) {
        float a = 2 * L;
        p.front = blend(front0, 1 - a, front5, a);
        p.back = blend(back0, 1 - a, back5, a);
    } else {
        float a = (L - .5f) * 2;
        p.front = blend(front1, a, front5, 1 - a);
        p.back = blend(back1, a, back5, 1 - a);
    }

    // §3.7: ambient, cold zones towards (0.3, 0.4, 0.6).
    p.ambient = {.6f, .6f, .4f};
    if (wm < .2f) {
        float t = wm / .2f;
        p.ambient = {(1 - t) * .3f + t * .6f, (1 - t) * .4f + t * .6f, (1 - t) * .6f + t * .4f};
    }
    return p;
}

glm::vec4 material(float S, bool underwater) {
    return underwater ? glm::vec4(.4f, .5f, 1, S) : glm::vec4(1, 1, 1, S);
}

float fogGradientTranslation(float zenithDegrees) { return (zenithDegrees - 80) / -30.f; }

void FogDistance::update(float targetDistance, double elapsedMs) {
    double w = 1 - std::pow(.999, std::max(0.0, elapsedMs));
    distance = (float)(distance * (1 - w) + targetDistance * w);
}

std::vector<Star> stars() {
    // The C runtime's rand(): a linear congruential generator returning bits 16..30. Its
    // default seed is 1 (nothing seeds it before the stars are made: open point).
    uint32_t state = 1;
    auto rand = [&] {
        state = state * 214013u + 2531011u;
        return (int)((state >> 16) & 0x7fff);
    };
    std::vector<Star> out;
    out.reserve(3000);
    for (int i = 0; i < 3000; i++) {
        double azimuth = rand() * 2 * PI / 32767;
        double elevation = rand() * (PI / 2) / 32767;
        float size = (float)rand() * .1f / 32767 + .01f;
        float a = (float)azimuth, e = (float)elevation;
        glm::vec3 position(std::cos(a) * 100 * std::cos(e), std::sin(a) * 100 * std::cos(e), 100 * std::sin(e));
        if (rand() % 10 == 0)
            size *= 2;
        out.push_back({position, size});
    }
    return out;
}

glm::vec2 starHalfSizeOnScreen(const Star &star, const glm::mat4 &skyView, const glm::mat4 &skyLens) {
    const glm::vec3 centre = glm::mat3(skyView) * star.position;
    auto onScreen = [&](glm::vec3 p) {
        const glm::vec4 clip = skyLens * glm::vec4(p, 1);
        return glm::vec2(clip) / clip.w;
    };
    const glm::vec2 corner = onScreen(centre + glm::vec3(star.size, star.size, 0));
    return glm::abs(corner - onScreen(centre));
}

glm::vec4 sunSpriteColour(float S) {
    float green = std::min(50 + 250 * S, 255.f);
    int blue = (int)(200 * S * S) & 0xff;
    return glm::vec4(255, std::floor(green), (float)blue, 255) / 255.f;
}

float pointLightShare(glm::vec3 fromLight, glm::vec3 normal, float radius) {
    if (radius <= 0)
        return 0;
    float q = glm::dot(fromLight, fromLight);
    float facing = q > 0 ? std::clamp(-4 * glm::dot(normal, fromLight / std::sqrt(q)), .3f, 1.f) : 1.f;
    float fall = 1 - std::min(q / (radius * radius), 1.f);
    return facing * fall * fall;
}

} // namespace alpha
