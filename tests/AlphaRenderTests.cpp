// Checks of the Alpha look's CPU maths against docs/specs/alpha-rendering.md: the daylight
// curves (2.1, 2.2), the sun direction (2.3), the palette's worked values (3.8) and the light
// flood (15, item 3).
#include "AlphaLight.h"
#include "AlphaLook.h"
#include "AlphaMoon.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
int failures = 0;
#define ALPHA_CHECK(x)                                                                             \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

bool near(float a, float b, float tolerance = 1e-4f) { return std::abs(a - b) <= tolerance; }
// The spec's tables are rounded to four places.
bool near(glm::vec3 a, glm::vec3 b) { return near(a.r, b.r, 1e-4f) && near(a.g, b.g, 1e-4f) && near(a.b, b.b, 1e-4f); }
double at(double hours) { return hours * 3600000.0; }

void daylightCurves() {
    // 2.1: L at noon, 09:00, 06:00, 03:00, midnight, and symmetric afternoons.
    ALPHA_CHECK(near(alpha::paletteDaylight(at(12)), 1));
    ALPHA_CHECK(near(alpha::paletteDaylight(at(9)), .9390f));
    ALPHA_CHECK(near(alpha::paletteDaylight(at(15)), .9390f));
    ALPHA_CHECK(near(alpha::paletteDaylight(at(6)), .5862f));
    ALPHA_CHECK(near(alpha::paletteDaylight(at(18)), .5862f));
    ALPHA_CHECK(near(alpha::paletteDaylight(at(3)), .1117f));
    ALPHA_CHECK(near(alpha::paletteDaylight(at(0)), 0));
    // 2.2: S at noon, 09:00, 06:00, 03:00, 01:00, midnight.
    ALPHA_CHECK(near(alpha::sunlight(at(12)), 1));
    ALPHA_CHECK(near(alpha::sunlight(at(9)), .9806f));
    ALPHA_CHECK(near(alpha::sunlight(at(6)), .7242f));
    // (The spec's table gives 0.1494; (1 - 0.75^4)^5 is 0.14920.)
    ALPHA_CHECK(near(alpha::sunlight(at(3)), .1492f));
    ALPHA_CHECK(near(alpha::sunlight(at(1)), .0022f));
    ALPHA_CHECK(near(alpha::sunlight(at(0)), 0));
    // 8.3: stars switch on at S = 0.5, about 04:48 and 19:12.
    ALPHA_CHECK(alpha::starsVisible(alpha::sunlight(at(4.7)), false));
    ALPHA_CHECK(!alpha::starsVisible(alpha::sunlight(at(4.9)), false));
    ALPHA_CHECK(alpha::starsVisible(alpha::sunlight(at(19.3)), false));
    ALPHA_CHECK(!alpha::starsVisible(alpha::sunlight(at(0)), true));
    // 2.3: the light direction.
    auto dir = [](double h, glm::vec3 want) { return glm::length(alpha::sunDirection(at(h)) - want) < 1e-4f; };
    ALPHA_CHECK(dir(12, {0, 0, 1}));
    ALPHA_CHECK(dir(6, {0, -1, 0}));
    ALPHA_CHECK(dir(18, {0, 1, 0}));
    ALPHA_CHECK(dir(0, {0, 0, -1}));
}

void paletteTable() {
    const alpha::ZoneFactors temperate{.5f, .5f, 0, 0};
    // 3.8, noon.
    alpha::Palette noon = alpha::palette(alpha::paletteDaylight(at(12)), temperate, false);
    ALPHA_CHECK(near(noon.sky1, {.1f, .6f, 1}));
    ALPHA_CHECK(near(noon.sky2, {.01f, .05f, 1}));
    ALPHA_CHECK(near(noon.fog, {.01f, .1f, 1}));
    ALPHA_CHECK(near(noon.front, {1.1f, 1.1f, .9f}));
    ALPHA_CHECK(near(noon.back, {.5f, .6f, .8f}));
    ALPHA_CHECK(near(noon.ambient, {.6f, .6f, .4f}));
    // 06:00.
    alpha::Palette dawn = alpha::palette(alpha::paletteDaylight(at(6)), temperate, false);
    ALPHA_CHECK(near(dawn.sky1, {1, .8586f, .4862f}));
    ALPHA_CHECK(near(dawn.sky2, {.2207f, .0931f, .7793f}));
    ALPHA_CHECK(near(dawn.fog, {.9655f, .8517f, .7862f}));
    ALPHA_CHECK(near(dawn.front, {1.0172f, .9345f, .6517f}));
    ALPHA_CHECK(near(dawn.back, {.5828f, .5172f, .9655f}));
    ALPHA_CHECK(near(dawn.ambient, {.6f, .6f, .4f}));
    // Midnight.
    alpha::Palette night = alpha::palette(alpha::paletteDaylight(at(0)), temperate, false);
    ALPHA_CHECK(near(night.sky1, {.05f, 0, .1f}));
    ALPHA_CHECK(near(night.sky2, {0, 0, .1f}));
    ALPHA_CHECK(near(night.fog, {0, 0, .1f}));
    ALPHA_CHECK(near(night.front, {.01f, .02f, .01f}));
    ALPHA_CHECK(near(night.back, {0, .02f, .03f}));
    // Key boundaries: L = 0.75 is still the day's, 0.6 dusk B, 0.4 dusk A.
    ALPHA_CHECK(near(alpha::palette(.75f, temperate, false).sky1, {.1f, .6f, 1}));
    ALPHA_CHECK(near(alpha::palette(.6f, temperate, false).sky1, {1, .9f, .5f}));
    ALPHA_CHECK(near(alpha::palette(.4f, temperate, false).sky2, {.5f, 0, .5f}));
    // Modifiers at full strength: fire turns the day sky red; cold zones a blue ambient;
    // under water the sky is a dim blue and the fog stays.
    alpha::Palette fire = alpha::palette(1, {.5f, .5f, 0, 1}, false);
    ALPHA_CHECK(near(fire.sky1, {1, 0, 0}) && near(fire.fog, {.3f, 0, 0}) && near(fire.front, {1, .8f, .1f}));
    ALPHA_CHECK(near(alpha::palette(1, {0, .5f, 0, 0}, false).ambient, {.3f, .4f, .6f}));
    alpha::Palette water = alpha::palette(.5f, temperate, true);
    ALPHA_CHECK(near(water.sky1, {.05f, .075f, .5f}) && near(water.sky2, water.sky1));
    ALPHA_CHECK(near(water.fog, alpha::palette(.5f, temperate, false).fog));
    // Hot and dry: the sandy haze at full weight.
    ALPHA_CHECK(near(alpha::palette(1, {1, 0, 0, 0}, false).fog, {.8f, .7f, .8f}));
    // The sun sprite's colour at noon and midnight.
    ALPHA_CHECK(glm::length(alpha::sunSpriteColour(1) - glm::vec4(1, 1, 200 / 255.f, 1)) < 1e-5f);
    ALPHA_CHECK(glm::length(alpha::sunSpriteColour(0) - glm::vec4(1, 50 / 255.f, 0, 1)) < 1e-5f);
    // 2 x 1500 stars, all above the horizon on the 100-block sphere.
    auto stars = alpha::stars();
    bool onSphere = stars.size() == 3000;
    for (auto &s : stars)
        onSphere = onSphere && near(glm::length(s.position), 100, .01f) && s.position.z >= 0 && s.size >= .01f && s.size <= .2201f;
    ALPHA_CHECK(onSphere);
}

void lightFlood() {
    using alphalight::BlockKind;
    // A 5 x 1 x 6 slab: solid ground at z = 0; a roof at z = 3 over x = 0..2; open elsewhere.
    alphalight::LightVolume v({0, 0, 0}, {5, 1, 6});
    for (int x = 0; x < 5; x++)
        for (int z = 1; z < 6; z++)
            v.setKind({x, 0, z}, z == 3 && x <= 2 ? BlockKind::Solid : BlockKind::Open);
    v.flood();
    // Open columns are sky-lit to the ground.
    ALPHA_CHECK(v.light({4, 0, 1}) == 255 && v.light({3, 0, 2}) == 255);
    ALPHA_CHECK(v.light({0, 0, 5}) == 255);
    // Under the roof: 216 beside sky-lit air, then 85 % per step (183, 155).
    ALPHA_CHECK(v.light({2, 0, 2}) == 216 && v.light({2, 0, 1}) == 216);
    ALPHA_CHECK(v.light({1, 0, 2}) == 183);
    ALPHA_CHECK(v.light({0, 0, 2}) == 155);

    // A sealed two-block cave settles at 5 * 85 / 100 = 4 (an open neighbour counts at least 5).
    alphalight::LightVolume cave({0, 0, 0}, {4, 3, 3});
    cave.setKind({1, 1, 1}, BlockKind::Open);
    cave.setKind({2, 1, 1}, BlockKind::Open);
    cave.flood();
    ALPHA_CHECK(cave.light({1, 1, 1}) == 4);
    alphalight::CornerLight sealed = alphalight::cornerLight(cave, {1, 1, 1}, 4);
    ALPHA_CHECK(sealed.light < .05f && sealed.open < .15f);
    // A glowing neighbour lights it as the sky would.
    cave.setKind({1, 1, 0}, BlockKind::Glow);
    cave.flood();
    ALPHA_CHECK(cave.light({1, 1, 1}) == 216);

    // A column covered above the box gets no sky light.
    alphalight::LightVolume shaft({0, 0, 0}, {1, 1, 3});
    for (int z = 0; z < 3; z++)
        shaft.setKind({0, 0, z}, BlockKind::Open);
    shaft.setRoofed(0, 0);
    shaft.flood();
    ALPHA_CHECK(shaft.light({0, 0, 2}) == 4 && shaft.light({0, 0, 0}) == 4);

    // 9.3: a top face out in the open is fully lit and open; one in the sealed cave is dark.
    alphalight::LightVolume field({-4, -4, 0}, {9, 9, 3});
    for (int x = -4; x <= 4; x++)
        for (int y = -4; y <= 4; y++)
            for (int z = 1; z < 3; z++)
                field.setKind({x, y, z}, BlockKind::Open);
    field.flood();
    alphalight::CornerLight open = alphalight::cornerLight(field, {0, 0, 1}, 4);
    ALPHA_CHECK(near(open.light, 1) && near(open.open, 1));
    // 9.4: fully open keeps the colour; closed in it darkens towards c - 0.9 luminance.
    glm::vec3 grass(.2f, .6f, .1f);
    ALPHA_CHECK(near(alphalight::occludedColour(grass, 1), grass));
    float lum = .3f * .2f + .59f * .6f + .11f * .1f;
    ALPHA_CHECK(near(alphalight::occludedColour(grass, 0), glm::clamp(grass - .9f * lum, 0.f, 1.f)));
    // 9.5: water colour from its light.
    ALPHA_CHECK(near(alphalight::waterColour(1), {.3f, .4f, 1}) && near(alphalight::waterColour(0), {.1f, .2f, 1}));
    // 10.3: a point light at its reach gives nothing; on a facing surface at half reach, 9/16.
    ALPHA_CHECK(near(alpha::pointLightShare({0, 0, -20}, {0, 0, 1}, 20), 0));
    ALPHA_CHECK(near(alpha::pointLightShare({0, 0, -10 * std::sqrt(2.f)}, {0, 0, 1}, 20), .25f));
    ALPHA_CHECK(near(alpha::pointLightShare({0, 0, 10}, {0, 0, 1}, 20), .3f * .5625f));
    // 7.1: the fog closes in smoothly.
    alpha::FogDistance fog;
    fog.reset(100);
    fog.update(200, 1000);
    ALPHA_CHECK(near(fog.value(), 100 + 100 * (1 - (float)std::pow(.999, 1000)), 1e-3f));
}
// CubeVulkan's own moon (src/AlphaMoon.h): no spec values, only the behaviour it promises.
void moon() {
    const alpha::MoonSettings settings;
    // M is 0 at noon (and all day while the moon is down), and at its maximum, 1, at midnight.
    ALPHA_CHECK(alpha::moonStrength(at(12)) == 0);
    ALPHA_CHECK(alpha::moonStrength(at(9)) == 0 && alpha::moonStrength(at(15)) == 0);
    ALPHA_CHECK(near(alpha::moonStrength(at(0)), 1));
    float highest = 0;
    for (int minute = 0; minute < 24 * 60; minute++)
        highest = std::max(highest, alpha::moonStrength(at(minute / 60.0)));
    ALPHA_CHECK(near(highest, alpha::moonStrength(at(0))));
    // Dusk and dawn are continuous: no jump between one game second and the next, and M
    // climbs steadily through dusk and falls through dawn.
    float largestStep = 0, previous = alpha::moonStrength(at(16));
    bool rising = true, falling = true;
    for (int second = 1; second <= 4 * 3600; second++) {
        float m = alpha::moonStrength(at(16) + second * 1000.0);
        largestStep = std::max(largestStep, std::abs(m - previous));
        rising = rising && m >= previous;
        previous = m;
    }
    previous = alpha::moonStrength(at(4));
    for (int second = 1; second <= 4 * 3600; second++) {
        float m = alpha::moonStrength(at(4) + second * 1000.0);
        largestStep = std::max(largestStep, std::abs(m - previous));
        falling = falling && m <= previous;
        previous = m;
    }
    ALPHA_CHECK(largestStep < 1e-3f && rising && falling);
    ALPHA_CHECK(near(alpha::moonStrength(at(18)), 0) && near(alpha::moonStrength(at(6)), 0));
    ALPHA_CHECK(alpha::moonStrength(at(21)) > .5f && alpha::moonStrength(at(3)) > .5f);

    // The moon is always opposite the sun: overhead at midnight.
    for (double h : {0.0, 3.0, 6.0, 9.5, 12.0, 18.0, 21.0})
        ALPHA_CHECK(glm::length(alpha::moonDirection(at(h)) + alpha::sunDirection(at(h))) < 1e-6f);
    ALPHA_CHECK(glm::length(alpha::moonDirection(at(0)) - glm::vec3(0, 0, 1)) < 1e-4f);

    // Moonlight follows the baked light: none at all with no light, almost none on a corner
    // of the sealed cave, its full colour on open ground facing the moon.
    alpha::MoonLight midnight = alpha::moonLight(at(0), settings);
    const glm::vec3 up(0, 0, 1), sun = alpha::sunDirection(at(0));
    ALPHA_CHECK(alpha::moonOnVertex(midnight, up, sun, 0) == glm::vec3(0));
    alphalight::LightVolume cave({0, 0, 0}, {4, 3, 3});
    cave.setKind({1, 1, 1}, alphalight::BlockKind::Open);
    cave.setKind({2, 1, 1}, alphalight::BlockKind::Open);
    cave.flood();
    float caveLight = alphalight::cornerLight(cave, {1, 1, 1}, 4).light;
    glm::vec3 inCave = alpha::moonOnVertex(midnight, up, sun, caveLight);
    glm::vec3 inOpen = alpha::moonOnVertex(midnight, up, sun, 1);
    ALPHA_CHECK(inCave.b < .05f * inOpen.b);
    ALPHA_CHECK(near(inOpen, midnight.front + midnight.ambient));
    ALPHA_CHECK(inOpen.b > inOpen.r); // cool blue-white
    // Switched off, or at noon, the moon adds nothing.
    alpha::MoonSettings off = settings;
    off.enabled = false;
    ALPHA_CHECK(alpha::moonOnVertex(alpha::moonLight(at(0), off), up, sun, 1) == glm::vec3(0));
    ALPHA_CHECK(alpha::moonOnVertex(alpha::moonLight(at(12), settings), up, alpha::sunDirection(at(12)), 1) == glm::vec3(0));

    // The shadow light hands over from the sun to the moon with no pop: its strength changes
    // smoothly through the whole day, it is the sun's while the sun is up and the moon's while
    // it is down, its light is always above the horizon, and wherever the caster changes the
    // strength on both sides is (almost) 0.
    float strongestStep = 0, lastStrength = alpha::shadowLight(0, .9f, settings).strength;
    bool lastMoon = alpha::shadowLight(0, .9f, settings).moon, switchesAtZero = true, aboveHorizon = true;
    int switches = 0;
    for (int second = 1; second <= 24 * 3600; second++) {
        alpha::ShadowLight s = alpha::shadowLight(second * 1000.0, .9f, settings);
        strongestStep = std::max(strongestStep, std::abs(s.strength - lastStrength));
        aboveHorizon = aboveHorizon && s.direction.z >= -1e-6f;
        if (s.moon != lastMoon) {
            switches++;
            switchesAtZero = switchesAtZero && s.strength < 1e-4f && lastStrength < 1e-4f;
        }
        lastStrength = s.strength;
        lastMoon = s.moon;
    }
    ALPHA_CHECK(strongestStep < 1e-3f && switchesAtZero && aboveHorizon && switches == 2);
    ALPHA_CHECK(!alpha::shadowLight(at(12), .9f, settings).moon && near(alpha::shadowLight(at(12), .9f, settings).strength, .9f));
    alpha::ShadowLight night = alpha::shadowLight(at(0), .9f, settings);
    ALPHA_CHECK(night.moon && near(night.strength, settings.shadowStrength) &&
                glm::length(night.direction - glm::vec3(0, 0, 1)) < 1e-4f);
    // Without the moon the nights cast nothing, as in Alpha.
    ALPHA_CHECK(!alpha::shadowLight(at(0), .9f, off).moon && alpha::shadowLight(at(0), .9f, off).strength == 0);

    // The disc shows at night and not by day; the phase runs round an eight-day cycle.
    ALPHA_CHECK(alpha::moonDiscVisibility(at(0)) == 1 && alpha::moonDiscVisibility(at(12)) == 0);
    ALPHA_CHECK(alpha::moonPhase(0) == 0 && alpha::moonPhase(4) == .5f && alpha::moonPhase(9) == alpha::moonPhase(1) &&
                alpha::moonPhase(-1) == alpha::moonPhase(7));
}
} // namespace

int alphaRenderingTests() {
    failures = 0;
    daylightCurves();
    paletteTable();
    lightFlood();
    moon();
    return failures;
}
