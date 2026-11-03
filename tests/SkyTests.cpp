// Checks of the sky against docs/specs/sky-and-clouds.md: Alpha's cloud cells, noise and gates
// (§A.5) and star size (§A.3); Steam's palette at its key points (§B.3), its easing (§B.4), the
// sun (§B.5), the stars (§B.6) and the cloud cells (§B.7).
#include "AlphaLook.h"
#include "Clouds.h"
#include "GameMath.h"
#include "SteamSky.h"
#include "worldgen/Noise.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <bit>
#include <string>

namespace {
int failures = 0;
#define SKY_CHECK(x)                                                                               \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x);                              \
            failures++;                                                                            \
        }                                                                                          \
    } while (0)

bool near(double a, double b, double tolerance = 1e-4) { return std::abs(a - b) <= tolerance; }
bool near(glm::vec3 a, glm::vec3 b, float tolerance = 1e-4f) { return glm::all(glm::lessThanEqual(glm::abs(a - b), glm::vec3(tolerance))); }
double at(double hours) { return hours * 3600000.0; }

// §A.5's lattice value, written out here on its own from the spec's formula.
double specLattice(int32_t a, int32_t b) {
    uint32_t k = (uint32_t)a + 57u * (uint32_t)b;
    k = (k << 13) ^ k;
    uint32_t v = k * (k * k * 60493u + 19990303u) + 3523796237u;
    return 1.0 - (double)(v & 0x7fffffffu) / 1073741824.0;
}

void alphaCloudCells() {
    // C's division towards zero, and the staggered columns.
    SKY_CHECK(clouds::cellOf(0) == 0 && clouds::cellOf(199.9) == 0 && clouds::cellOf(-199.9) == 0);
    SKY_CHECK(clouds::cellOf(200) == 1 && clouds::cellOf(-200) == -1 && clouds::cellOf(-399) == -1 && clouds::cellOf(-400) == -2);
    SKY_CHECK(clouds::cellBase(2, 0) == glm::dvec2(400, 0));
    SKY_CHECK(clouds::cellBase(3, 2) == glm::dvec2(600, 500));
    SKY_CHECK(clouds::cellBase(-1, 0) == glm::dvec2(-200, -100)); // C's remainder: -1
    SKY_CHECK(clouds::cellBase(-2, -1) == glm::dvec2(-400, -200));

    // Independently rechecked native constant/return vector, not a formula copy.
    SKY_CHECK(std::bit_cast<uint32_t>(clouds::alphaNoise(0, 0)) == 0xbe9046e8u);
    // The noise: the lattice value at integer points, the cosine halfway between, within -1..1.
    bool lattice = true, inRange = true;
    for (int a = -20; a <= 20; a += 3)
        for (int b = -20; b <= 20; b += 7) {
            lattice = lattice && near(clouds::alphaNoise(a, b), (float)specLattice(a, b), 1e-6);
            float n = clouds::alphaNoise(a + .37, b + .81);
            inRange = inRange && n >= -1.0001f && n <= 1.0001f;
        }
    SKY_CHECK(lattice && inRange);
    SKY_CHECK(near(clouds::alphaNoise(3.5, 4), (specLattice(3, 4) + specLattice(4, 4)) / 2, 1e-6));
    SKY_CHECK(near(clouds::alphaNoise(3, 4.5), (specLattice(3, 4) + specLattice(3, 5)) / 2, 1e-6));

    // The gate at the player.
    SKY_CHECK(clouds::alphaCloudsShown(.02f, 0) && !clouds::alphaCloudsShown(.01f, 0));
    SKY_CHECK(clouds::alphaCloudsShown(.5f, .009f) && !clouds::alphaCloudsShown(.5f, .01f));

    // A wet world 50 blocks up: each cell's cloud from the formulas, for a few cells.
    clouds::AlphaWorld wet;
    wet.moisture = [](int32_t, int32_t) { return 1.f; };
    wet.mountain = [](int32_t, int32_t) { return 0.f; };
    wet.ground = [](int32_t, int32_t) { return 50.f; };
    int cloudy = 0, cells = 0;
    for (int32_t cx = -6; cx <= 6; cx++)
        for (int32_t cy = -6; cy <= 6; cy++) {
            cells++;
            const auto c = clouds::alphaCloud(cx, cy, wet);
            const bool expected = (specLattice(cx, cy) + 1) * .5 < .8;
            SKY_CHECK(c.has_value() == expected);
            if (!c)
                continue;
            cloudy++;
            const glm::dvec2 base = clouds::cellBase(cx, cy);
            SKY_CHECK(c->base.x == base.x && c->base.y == base.y);
            SKY_CHECK(near(c->base.z, 50 + 170 + 50 * specLattice(17 * cx, 19 * cy), 1e-3));
            SKY_CHECK(near(c->scale, 7 + 2 * specLattice(15 * cx, 23 * cy), 1e-4));
            SKY_CHECK(c->turnDegrees == (float)((cx + cy) * 90));
            SKY_CHECK(c->base.z - 50 >= 120 - 1e-3 && c->base.z - 50 <= 220 + 1e-3 && c->scale >= 5 - 1e-4 && c->scale <= 9 + 1e-4);
            SKY_CHECK(!c->dark);
        }
    SKY_CHECK(cloudy > 0 && cloudy < cells);

    // Native altitude converts each float term to fixed64 separately. One base
    // lands halfway between fixed units; the final result must retain truncation.
    auto precise = wet;
    std::string heightOrder;
    precise.mountain = [&](int32_t, int32_t) {
        heightOrder += 'M';
        return .234567f;
    };
    precise.ground = [&](int32_t, int32_t) {
        heightOrder += 'H';
        return 50.000003814697265625f;
    };
    auto zero = wet;
    zero.ground = [](int32_t, int32_t) { return 0.f; };
    const auto reference = clouds::alphaCloud(0, 0, zero);
    const auto staged = clouds::alphaCloud(0, 0, precise);
    SKY_CHECK(reference && staged && heightOrder == "MH");
    SKY_CHECK(reference && staged && (staged->base.z - reference->base.z) * 65536.0 == 4814058.0);
    auto tinyNegative = zero;
    tinyNegative.ground = [](int32_t, int32_t) { return -.0000001f; };
    const auto tinyCloud = clouds::alphaCloud(0, 0, tinyNegative);
    SKY_CHECK(reference && tinyCloud && tinyCloud->base.z == reference->base.z);
    auto high = zero;
    high.ground = [](int32_t, int32_t) { return 70000.125f; };
    const auto highCloud = clouds::alphaCloud(0, 0, high);
    SKY_CHECK(reference && highCloud && (highCloud->base.z - reference->base.z) * 65536.0 == 4587528192.0);

    // Mountains lift a cloud 100 blocks; ground below the sea counts as the sea.
    clouds::AlphaWorld mountain = wet;
    mountain.mountain = [](int32_t, int32_t) { return 1.f; };
    clouds::AlphaWorld sea = wet;
    sea.ground = [](int32_t, int32_t) { return -300.f; };
    for (int32_t cx = -6; cx <= 6; cx++) {
        const auto flat = clouds::alphaCloud(cx, 1, wet);
        if (!flat)
            continue;
        SKY_CHECK(near(clouds::alphaCloud(cx, 1, mountain)->base.z, flat->base.z + 100, 1e-3));
        SKY_CHECK(near(clouds::alphaCloud(cx, 1, sea)->base.z, flat->base.z - 50, 1e-3));
    }

    // The coverage test reads the moisture at the cell numbers; the base point must be wet too.
    clouds::AlphaWorld dry = wet;
    dry.moisture = [](int32_t, int32_t) { return 0.f; };
    clouds::AlphaWorld dryAtCloud = wet; // wet only near the origin, where the cell numbers point
    dryAtCloud.moisture = [](int32_t x, int32_t y) { return std::abs(x) < 20 && std::abs(y) < 20 ? 1.f : .29f; };
    clouds::AlphaWorld justWetEnough = wet;
    justWetEnough.moisture = [](int32_t x, int32_t y) { return std::abs(x) < 20 && std::abs(y) < 20 ? 1.f : .3f; };
    bool anyDry = false, anyDryAtCloud = false, sameWhenJustWet = true;
    for (int32_t cx = 1; cx <= 6; cx++)
        for (int32_t cy = -6; cy <= 6; cy++) {
            anyDry = anyDry || clouds::alphaCloud(cx, cy, dry).has_value();
            anyDryAtCloud = anyDryAtCloud || clouds::alphaCloud(cx, cy, dryAtCloud).has_value();
            sameWhenJustWet = sameWhenJustWet && clouds::alphaCloud(cx, cy, justWetEnough).has_value() ==
                                                     clouds::alphaCloud(cx, cy, wet).has_value();
        }
    SKY_CHECK(!anyDry && !anyDryAtCloud && sameWhenJustWet);
}

void alphaStarSize() {
    // A star straight ahead, 100 blocks out, through the 50 degree sky lens: its half-height is
    // size x (1 / tan 25 degrees) / 100 of the half-screen, about 2.14 x what a corner added
    // after the lens (size / distance) gave, and narrower across by the aspect.
    const float aspect = 16.f / 9;
    glm::mat4 lens = glm::perspectiveRH_ZO(glm::radians(alpha::SKY_LENS_DEGREES), aspect, .1f, 2000.f);
    lens[1][1] *= -1;
    const glm::mat4 view = glm::lookAtRH(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
    const float focal = 1 / std::tan(glm::radians(alpha::SKY_LENS_DEGREES / 2));
    for (float size : {.01f, .06f, .22f}) {
        const glm::vec2 half = alpha::starHalfSizeOnScreen({{0, 100, 0}, size}, view, lens);
        SKY_CHECK(near(half.y, size * focal / 100, 1e-6));
        SKY_CHECK(near(half.x, size * focal / 100 / aspect, 1e-6));
        SKY_CHECK(near(half.y / (size / 100), 2.1445, 1e-3));
    }
    // No minimum: the smallest star is a tenth of a pixel tall on a 900-pixel screen.
    const glm::vec2 smallest = alpha::starHalfSizeOnScreen({{0, 100, 0}, .01f}, view, lens);
    SKY_CHECK(smallest.y * 450 < .25f);
    // The real stars all fit: sizes 0.01..0.22.
    float largest = 0;
    for (const alpha::Star &s : alpha::stars())
        largest = std::max(largest, s.size);
    SKY_CHECK(largest <= .22f + 1e-6f && largest > .11f);
}

void steamPaletteKeys() {
    const steamsky::Region temperate; // W 0.5, D 0.5, 50 blocks up
    // Day (L above 0.75 and at it).
    for (float L : {1.f, .9f, .75f}) {
        steamsky::Palette p = steamsky::palette(L, temperate, false);
        SKY_CHECK(near(p.sky1, {0, .6f, 1}) && near(p.sky2, {0, 0, .8f}) && near(p.fog, {0, .1f, 1}) &&
                  near(p.sun, {1.5f, 1.3f, .4f}));
    }
    // Dusk B at L 0.6, halfway to day at 0.675.
    steamsky::Palette p = steamsky::palette(.6f, temperate, false);
    SKY_CHECK(near(p.sky1, {1, .95f, .5f}) && near(p.sky2, {.3f, .5f, 1}) && near(p.fog, {.7f, .6f, .3f}) &&
              near(p.sun, {1.5f, .85f, .25f}));
    p = steamsky::palette(.675f, temperate, false);
    SKY_CHECK(near(p.sky1, {.5f, .775f, .75f}) && near(p.sun, {1.5f, 1.075f, .325f}));
    // Dusk A at L 0.2, halfway to dusk B at 0.4.
    p = steamsky::palette(.2f, temperate, false);
    SKY_CHECK(near(p.sky1, {1, .5f, .1f}) && near(p.sky2, {.5f, .2f, .8f}) && near(p.fog, {1, .5f, .2f}) &&
              near(p.sun, {1.5f, .85f, .25f}));
    p = steamsky::palette(.4f, temperate, false);
    SKY_CHECK(near(p.sky1, {1, .725f, .3f}) && near(p.sun, {1.5f, .85f, .25f}));
    // Night until L 0.15, then a quick dusk: halfway at 0.175.
    for (float L : {0.f, .1f, .15f}) {
        p = steamsky::palette(L, temperate, false);
        SKY_CHECK(near(p.sky1, {0, .03f, .15f}) && near(p.sky2, {0, 0, 0}) && near(p.fog, {0, .1f, .1f}) &&
                  near(p.sun, {0, 0, 0}));
    }
    p = steamsky::palette(.175f, temperate, false);
    SKY_CHECK(near(p.sky1, {.5f, .265f, .125f}) && near(p.sun, {.75f, .425f, .125f}));
    // The palette's own daylight is Alpha's curve: noon, about 06:00 and midnight.
    SKY_CHECK(near(alpha::paletteDaylight(at(6)), .5862, 1e-4));
    // Under water the sky takes the upper colour.
    p = steamsky::palette(1, temperate, true);
    SKY_CHECK(near(p.sky1, p.sky2) && near(p.fog, p.sky2) && near(p.sky2, {0, 0, .8f}));

    // The region modifiers at full strength, at noon.
    steamsky::Region moist = temperate;
    moist.dryness = 1;
    p = steamsky::palette(1, moist, false);
    SKY_CHECK(near(p.sky1, {0, .6f, .8f}) && near(p.sky2, {0, .5f, .5f}) && near(p.fog, {0, .6f, .65f}));
    steamsky::Region shore = moist; // 10 blocks up: half the dryness, no longer moist
    shore.ground = 10;
    SKY_CHECK(near(steamsky::palette(1, shore, false).sky1, {0, .6f, 1}));
    steamsky::Region desert = temperate;
    desert.warmth = 1, desert.dryness = 0;
    p = steamsky::palette(1, desert, false);
    SKY_CHECK(near(p.sky1, {.6f, .8f, 1}) && near(p.sky2, {.1f, .4f, 1}) && near(p.fog, {.8f, .5f, .7f}));
    steamsky::Region lava = temperate;
    lava.lava = 1;
    SKY_CHECK(near(steamsky::palette(1, lava, false).sky1, {1, 0, 0}));
    p = steamsky::palette(0, lava, false);
    SKY_CHECK(near(p.sky1, {.06f, 0, .03f}) && near(p.sky2, {0, 0, 0}) && near(p.fog, {.1f, 0, 0}));
    steamsky::Region forest = temperate;
    forest.forest = 1;
    p = steamsky::palette(1, forest, false);
    SKY_CHECK(near(p.sky1, {.2f, .6f, .4f}) && near(p.sky2, {0, .1f, .2f}) && near(p.fog, {.4f, .1f, .8f}));
    SKY_CHECK(near(steamsky::palette(0, forest, false).fog, {.1f, .15f, .2f}));
    steamsky::Region pink = temperate;
    pink.kind27 = 1;
    p = steamsky::palette(1, pink, false);
    SKY_CHECK(near(p.sky1, {1, .6f, 1}) && near(p.sky2, {0, .6f, 1}) && near(p.fog, {.6f, .3f, 1}));
    SKY_CHECK(near(steamsky::palette(.2f, pink, false).sky1, {1, .5f, .1f})); // dusk A's sky1 stays

    // Provider RGBA: place-27's dusk-B alpha and the sun's night fade are independent.
    pink.kind27 = .5f;
    SKY_CHECK(near(steamsky::palette(.6f, pink, false).sky1.a, .75));
    SKY_CHECK(near(steamsky::palette(.675f, pink, false).sky1.a, .875));
    SKY_CHECK(near(steamsky::palette(.175f, temperate, false).sun.a, .5));
    SKY_CHECK(steamsky::palette(0, temperate, false).sun.a == 0);
    // Far flight changes selected keys after place modifiers; fog receives the full saved RGBA.
    p = steamsky::palette(.6f, pink, false, {true, {}});
    SKY_CHECK(p.sky1 == glm::vec4(0, 0, 0, 1) && p.sky2 == glm::vec4(0, 0, 0, 1));
    SKY_CHECK(near(p.fog.a, .75) && near(p.fog, {.9f, .575f, .75f}));
    p = steamsky::palette(.2f, pink, false, {true, {}});
    SKY_CHECK(near(p.sky1, {1, .5f, .1f}) && near(p.fog, {1, .5f, .2f}));
    p = steamsky::palette(0, temperate, false, {true, {}});
    SKY_CHECK(near(p.fog, {0, .03f, .15f}) && p.fog.a == 1);
    // Camera water replacement precedes effect13; its age uses an upper-only clamp.
    p = steamsky::palette(.6f, pink, true, {true, 58500.f});
    SKY_CHECK(near(p.sky1, {.45f, .5f, .5f}) && p.sky1.a == 1 && p.fog == p.sky1);
    SKY_CHECK(p.sky2 == glm::vec4(0, 0, 0, 1) && p.sun.a == 1);
    p = steamsky::palette(1, temperate, false, {false, 63000.f});
    SKY_CHECK(near(p.sky1, {-.9f, .2f, 1}));

    // Horizontal distance thresholds are strict and ignore altitude, in native fixed coordinates.
    const std::array<int32_t, 2> anchor{-20, 10};
    std::array<int64_t, 3> physical{980LL * 65536, 10LL * 65536, 9000000};
    std::array<int64_t, 3> from{(16384LL - 20) * 65536, 10LL * 65536, -9000000};
    SKY_CHECK(!steamsky::farFlight(0x9c, physical, anchor, from));
    ++physical[0];
    SKY_CHECK(!steamsky::farFlight(0x9c, physical, anchor, from));
    ++from[0];
    SKY_CHECK(steamsky::farFlight(0x9c, physical, anchor, from));
    SKY_CHECK(!steamsky::farFlight(0x9d, physical, anchor, from));

    // Native construction, first update and both four-component transition branches.
    steamsky::EasedPalette eased;
    const steamsky::Palette initial = eased.value();
    const steamsky::Palette noon = steamsky::palette(1, temperate, false), night = steamsky::palette(0, temperate, false);
    SKY_CHECK(initial.sky1 == glm::vec4(0, 0, 1, 1) && initial.sky2 == initial.sky1 && initial.fog == initial.sky1);
    SKY_CHECK(initial.sun == glm::vec4(0));
    eased.update(noon, 0);
    SKY_CHECK(eased.value().sky1 == initial.sky1 && eased.value().fog == initial.fog && eased.value().sun == initial.sun);
    const float skyStep = gamemath::blendFactor(16, .001f), fogStep = gamemath::blendFactor(16, .0005f);
    const float sunStep = gamemath::blendFactor(16, .002f);
    eased.update(noon, 16);
    SKY_CHECK(eased.value().sky1 == (noon.sky1 - initial.sky1) * skyStep + initial.sky1);
    SKY_CHECK(eased.value().fog == (noon.fog - initial.fog) * fogStep + initial.fog);
    SKY_CHECK(eased.value().sun == noon.sun * sunStep);
    const auto before = eased.value();
    const glm::vec4 interior(.2f, .4f, .6f, 0);
    eased.update(noon, 16, interior);
    SKY_CHECK(eased.value().sky1 == (interior - before.sky1) * skyStep + before.sky1);
    SKY_CHECK(eased.value().sky2 == (interior - before.sky2) * skyStep + before.sky2);
    SKY_CHECK(eased.value().fog == (interior - before.fog) * fogStep + before.fog);
    SKY_CHECK(eased.value().sun == (glm::vec4(0) - before.sun) * fogStep + before.sun);
    SKY_CHECK(eased.value().fog.a < 1 && eased.value().sky1.a < 1);
    const auto indoors = eased.value();
    eased.update(night, 16); // target cleared: ordinary branch resumes, retaining the previous state.
    SKY_CHECK(eased.value().sky1 == (night.sky1 - indoors.sky1) * skyStep + indoors.sky1);
    SKY_CHECK(eased.value().sun == (night.sun - indoors.sun) * sunStep + indoors.sun);
    const auto unchanged = eased.value();
    eased.update(noon, -1);
    SKY_CHECK(eased.value().fog == unchanged.fog && eased.value().sun == unchanged.sun);
    steamsky::EasedPalette rounded, capped, atCap;
    rounded.update(noon, 9.9999999); // scene narrows the interval to float10 before the helper.
    SKY_CHECK(rounded.value().fog == (noon.fog - initial.fog) * gamemath::blendFactor(10, .0005f) + initial.fog);
    capped.update(noon, 2000);
    atCap.update(noon, 1000);
    SKY_CHECK(capped.value().sky1 == atCap.value().sky1 && capped.value().sun == atCap.value().sun);
}

void steamSunAndStars() {
    // §B.5: the sun's path.
    SKY_CHECK(near(steamsky::sunDirection(at(0)), {0, 0, -1}));
    SKY_CHECK(near(steamsky::sunDirection(at(6)), glm::normalize(glm::vec3(1, -.4f * std::sin(glm::radians(45.f)), 0))));
    SKY_CHECK(near(steamsky::sunDirection(at(12)), glm::normalize(glm::vec3(0, -.4f, 1))));
    SKY_CHECK(steamsky::sunDirection(at(18)).x < -.9f && steamsky::sunDirection(at(18)).y < 0);
    // Its place on the screen: straight ahead is the screen's centre, shifted with the tilt.
    glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(60.f), 16.f / 9, .1f, 1000.f);
    projection[1][1] *= -1;
    const glm::mat4 view = glm::lookAtRH(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0, 0, 1));
    const glm::vec4 ahead = steamsky::sunPosition({1, 0, 0}, view, projection, 16.f / 9, .25f);
    SKY_CHECK(near(ahead.x, 0, 1e-5) && near(ahead.y, .25f, 1e-5) && near(ahead.z, 16.f / 9) && ahead.w == 1);
    const glm::vec4 above = steamsky::sunPosition(glm::normalize(glm::vec3(1, 0, .1f)), view, projection, 16.f / 9, 0);
    SKY_CHECK(above.y > 0 && above.w == 1);
    SKY_CHECK(steamsky::sunPosition({-1, 0, 0}, view, projection, 16.f / 9, 0).w == 0);

    // §B.6: the star value, the switch and the fade.
    SKY_CHECK(near(steamsky::starValue(at(12), 0), 1) && near(steamsky::starValue(at(0), 0), .05f));
    SKY_CHECK(near(steamsky::starValue(at(12), 1), .25f));
    SKY_CHECK(steamsky::starsShown(.39f) && !steamsky::starsShown(.4f));
    SKY_CHECK(near(steamsky::starOpacity(.05f), 1) && near(steamsky::starOpacity(.35f), .5f) && near(steamsky::starOpacity(.4f), 0));
    // The dome: 50 rings, 50 places on the horizon down to 1 near the zenith, the even places
    // of the generator's random number holding a star 1..3 blocks big.
    int places = 0;
    for (int r = 0; 57 * r < 2850; r++)
        places += (int)(50 - 7 * std::sqrt((float)r));
    const std::vector<steamsky::Star> dome = steamsky::starDome();
    SKY_CHECK(places == 855 && dome.size() > 300 && dome.size() < 555);
    bool scales = true;
    for (const auto &s : dome)
        scales = scales && s.scale >= 1 && s.scale < 3 && s.tiltDegrees >= 0 && s.tiltDegrees < 90;
    SKY_CHECK(scales);
    int firstRing = 0;
    for (int j = 0; j < 50; j++)
        firstRing += (worldgen::random((uint32_t)j) & 1u) ? 0 : 1;
    SKY_CHECK(dome.size() >= (size_t)firstRing && dome[0].tiltDegrees == 0);
    // A star's matrix puts it on the dome: 64 R - 10 blocks out, at its tilt above the horizon.
    const steamsky::Star star{30, 45, 2};
    const glm::vec3 placed = glm::vec3(steamsky::starMatrix(star, 10) * glm::vec4(0, 0, 0, 1));
    SKY_CHECK(near(glm::length(placed), 630, 1e-2) && near(placed.z, 630 * std::sin(glm::radians(45.f)), 1e-2));
    SKY_CHECK(near(glm::length(glm::vec3(steamsky::starMatrix(star, 10)[0])), 2, 1e-5));
}

void steamCloudCells() {
    clouds::SteamWorld world;
    world.dryness = [](int32_t, int32_t) { return 1.f; };
    world.ground = [](int32_t, int32_t) { return 30.25f; };
    world.forest = [](int32_t x, int32_t) { return x < 0 ? .31f : .3f; };
    int cloudy = 0;
    for (int32_t cx = -4; cx <= 4; cx++)
        for (int32_t cy = -4; cy <= 4; cy++) {
            const glm::dvec2 base = clouds::cellBase(cx, cy);
            const float n = worldgen::simplex(base.x, base.y);
            const auto c = clouds::steamCloud(cx, cy, world);
            SKY_CHECK(c.has_value() == ((n + 1) * .5f <= .5f));
            if (!c)
                continue;
            cloudy++;
            const double rise = std::trunc((200 + 30 * worldgen::simplex(17.0 * cx, 19.0 * cy)) * 65536.0) / 65536.0;
            SKY_CHECK(c->base.x == base.x && c->base.y == base.y && near(c->base.z, 30.25 + rise, 1e-6));
            SKY_CHECK(near(c->scale, 7 + 2 * worldgen::simplex(15.0 * cx, 23.0 * cy), 1e-5));
            SKY_CHECK(c->turnDegrees == (float)((cx + cy) * 90) && c->dark == (cx < 0));
        }
    SKY_CHECK(cloudy > 0);
    // Dry regions have none; low ground counts as sea level.
    clouds::SteamWorld desert = world;
    desert.dryness = [](int32_t, int32_t) { return 0.f; };
    clouds::SteamWorld sea = world;
    sea.ground = [](int32_t, int32_t) { return -12.f; };
    bool anyDesert = false, seaLevel = true;
    for (int32_t cx = -4; cx <= 4; cx++)
        for (int32_t cy = -4; cy <= 4; cy++) {
            anyDesert = anyDesert || clouds::steamCloud(cx, cy, desert).has_value();
            const auto c = clouds::steamCloud(cx, cy, sea);
            seaLevel = seaLevel && (!c || (c->base.z >= 170 && c->base.z <= 230));
        }
    SKY_CHECK(!anyDesert && seaLevel);
    // Reach and the fade near the camera.
    SKY_CHECK(clouds::steamReach(400) == 4 && clouds::steamReach(199) == 2);
    SKY_CHECK(clouds::steamOpacity(250) == 1 && clouds::steamOpacity(180) == 1 && near(clouds::steamOpacity(130), .5f));
    SKY_CHECK(clouds::steamOpacity(80) == 0 && near(clouds::steamOpacity(85), .05f) && clouds::steamOpacity(80.5) < clouds::STEAM_HIDDEN_BELOW);
}
} // namespace

int skyTests() {
    failures = 0;
    alphaCloudCells();
    alphaStarSize();
    steamPaletteKeys();
    steamSunAndStars();
    steamCloudCells();
    return failures;
}
