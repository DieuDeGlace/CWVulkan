#include "SteamLighting.h"
#include "GameMath.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

int steamLightingTests() {
    int failures = 0;
    auto check = [&](bool result, const char *message) {
        if (!result) { std::printf("FAIL Steam lighting: %s\n", message); ++failures; }
    };
    auto near = [](glm::vec4 a, glm::vec4 b) { return glm::all(glm::lessThanEqual(glm::abs(a - b), glm::vec4(.000001f))); };
    constexpr float noon = 43200000;
    steamlighting::Environment environment;
    const auto day = steamlighting::targets(noon, environment), night = steamlighting::targets(0, environment);
    check(near(day.front, {.75f, .7f, .3f, 1}) && near(day.back, {.5f, .5f, .5f, 1}) &&
          near(day.ambient, {.25f, .3f, .7f, 1}), "unmodified midday endpoints");
    check(near(night.front, {.04f, .05f, .1f, 1}) && near(night.back, {.05f, .05f, .3f, 1}) &&
          near(night.ambient, {.05f, .075f, .1f, 0}), "midnight endpoints and independent ambient alpha");
    check(steamlighting::collectorDaylight(0, 1) == .0125f && steamlighting::collectorDaylight(noon, 0) == 1,
          "collector curve has its own floor and no post-forest clamp");
    environment.region.dryness = 0;
    const auto dry = steamlighting::targets(noon, environment);
    check(near(dry.ambient, {.3f, .3f, .2f, 1}) && near(dry.front, {.7f, .7f, .8f, 0}),
          "dry high endpoint also recomputes front alpha");
    environment = {};
    environment.region.ground = -10;
    check(near(steamlighting::targets(noon, environment).front, dry.front), "low ground scales dryness first");
    environment = {};
    environment.blockFlag20 = true;
    environment.structureKind = 43;
    const auto cool = steamlighting::targets(noon, environment);
    check(near(cool.ambient, {.1f, .3f, .4f, 1}) && near(cool.front, {1.4f, 1.2f, 1.1f, 0}),
          "structure43 uses the cool flagged palette and1.5 front subtraction");
    environment.structureKind = 40;
    const auto other = steamlighting::targets(noon, environment);
    check(near(other.ambient, {.25f, .1f, .5f, 1}) && near(other.front, {1.25f, 1.4f, 1, 0}),
          "other flagged structures use the alternative palette");
    environment = {};
    environment.region.forest = .5f;
    check(near(steamlighting::targets(0, environment).ambient, {.0375f, .04875f, .08f, 0}),
          "forest low ambient retains both successive blends");
    environment.region.lava = environment.region.forest = environment.region.kind27 = 1;
    const auto ordered = steamlighting::targets(noon, environment);
    check(near(ordered.front, {.9f, .7f, .6f, 1}) && near(ordered.ambient, {.3f, .5f, .5f, 1}) &&
          near(ordered.back, day.back), "place blend follows lava/forest; back remains independent");
    environment = {};
    environment.effect13RemainingMs = 57000;
    const auto effect = steamlighting::targets(0, environment);
    check(near(effect.front, {.05f, .1f, 1, 1}) && near(effect.back, effect.front) &&
          near(effect.ambient, {.01f, .05f, .2f, 1}), "effect13 changes all RGBA light targets");
    environment.effect13RemainingMs = 63000;
    check(steamlighting::targets(0, environment).ambient.w == -1, "effect weight has no invented lower clamp");
    check(steamlighting::material(true) == glm::vec4(.5f, .5f, 1, 1) && steamlighting::material(false) == glm::vec4(1),
          "water changes scene material separately from light colors");
    check(glm::length(steamlighting::sunDirection(0) - glm::vec3(0, 0, -1)) < .000001f &&
          steamlighting::sunDirection(noon).z > .92f && steamlighting::sunDirection(noon).y < -.37f,
          "submitted directional light follows clock with native lean");
    const float frames[]{1, 16, 33};
    const uint32_t expected[]{0x3a844b9f, 0x3c81c455, 0x3d04489e};
    for (int i = 0; i < 3; ++i) {
        const float coefficient = gamemath::blendFactor(frames[i], .001f);
        uint32_t bits;
        std::memcpy(&bits, &coefficient, 4);
        check(bits == expected[i], "native smoothing coefficient float word");
    }
    check(steamlighting::frameMilliseconds(60000) == 1000 && steamlighting::frameMilliseconds(-4) == -4,
          "native wall-frame producer caps resume intervals without a lower clamp");
    const float quarterDay = steamlighting::dayCoordinate(21600000);
    const auto morning = steamlighting::targets(21600000, steamlighting::Environment{});
    check(quarterDay == 2401.0f / 4096 && morning.front.x > .70f && morning.front.x < .71f &&
          morning.back.x > .38f && morning.back.x < .39f, "upper day branch uses quartic interpolation");
    const auto early = steamlighting::targets(10800000, steamlighting::Environment{});
    check(early.ambient.w == 1874161.0f / 16777216 && early.front.x > .176f && early.front.x < .177f,
          "lower day branch interpolates the low and middle endpoints");
    steamlighting::EasedColours state;
    check(near(state.value().front, {.7f, .7f, .3f, 1}) && near(state.value().ambient, {.3f, .3f, .7f, 1}),
          "native constructor values, not immediate clock targets");
    state.update(night, 16);
    check(std::abs(state.value().ambient.w - .9841592907905579f) < .0000001f,
          "ambient alpha is smoothed from previous state");
    const auto previous = state.value();
    state.update(day, std::numeric_limits<float>::infinity());
    state.update(day, -1);
    check(state.value().ambient == previous.ambient, "invalid/nonpositive native step count leaves state alone");
    state.reset();
    check(state.value().ambient.w == 1, "reset restores constructor state");
    std::printf("Steam lighting: %s\n", failures ? "FAIL" : "PASS");
    return failures;
}
