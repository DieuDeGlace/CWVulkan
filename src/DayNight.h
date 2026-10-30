#pragma once
// The Steam look's lights by time of day. The sun follows the Steam release's path
// (docs/specs/sky-and-clouds.md §B.5, SteamSky.h); the sky's colours are the Steam palette's,
// worked out by the renderer (SteamSky.h). Steam world's front/back/ambient use
// SteamLighting's native palette state. These legacy preview terms and the optional
// moon additions remain available separately.
//
// The moon is CubeVulkan's own addition, not the game's (the Steam release has none): the
// user's MoonandShadow mod (MoonRules.h) gives its path, its moonlight and when shadows switch
// between sun and moon.
#include "MoonRules.h"
#include "SteamSky.h"
#include <glm/glm.hpp>
#include <optional>

struct Lighting {
    glm::vec3 lightDir{0, 0, 1};      // Towards the shadow-casting light (sun or moon).
    float shadowStrength = 0;         // 0 = no shadows (night without moon, horizon).
    glm::vec3 front{1}, back{.3f}, ambient{.3f}; // Native-style light terms.
    float daylight = 1;               // 0 night .. 1 day.
    float hour = 12;                  // Time of day (0..24) the light is for.
    float moonWeight = 0;
    float timeMs = 43200000;
    glm::vec3 moonFront{0}, moonBack{0}; // optional additions, separate from native palette
    std::optional<float> effect13RemainingMs;
    glm::vec3 sunDir{0, 0, 1}, moonDir{0, 0, -1}; // Sky positions (always set).
    // Optional screen-space sky captured from the original menu draw. The
    // regular world clock leaves these zero and keeps the normal sky path.
    glm::vec4 menuSky[6]{}; // sky1, sky2, fog, sun colour, sun position, gradient/enabled
};

struct DayNightSettings {
    float dayStrength = .9f;   // SteamShadows StrengthPercent / 100
    float nightStrength = .6f; // MoonLight ShadowStrengthPercent / 100
    float moonBrightness = .04f;
};

// How bright the region the player is in makes its daylight (CubeVulkan's stand-in).
struct RegionLook {
    float daylight = 1;
};

inline Lighting lightingAt(float hour, const DayNightSettings &s, const RegionLook &region = {}) {
    Lighting l;
    l.hour = hour;
    float ms = hour * 3600000.f;
    l.timeMs = ms;
    auto moon = moonrules::state(ms, s.nightStrength);
    l.moonWeight = moon.weight;
    l.moonDir = glm::normalize(glm::vec3(moon.direction[0], moon.direction[1], moon.direction[2]));
    l.sunDir = steamsky::sunDirection(ms);
    // Shadows: the sun while it is up, the moon after (both are below the horizon at the
    // switch), each fading out towards the horizon as MoonandShadow's do.
    if (l.sunDir.z > 0) {
        l.lightDir = l.sunDir;
        l.shadowStrength = s.dayStrength * moonrules::horizonFade(l.sunDir.z);
    } else {
        l.lightDir = l.moonDir;
        l.shadowStrength = s.nightStrength * moon.weight * moonrules::horizonFade(l.moonDir.z);
    }
    float sunZ = l.sunDir.z;
    float day = glm::smoothstep(-.12f, .25f, sunZ);
    float dusk = glm::clamp(1 - std::abs(sunZ) / .3f, 0.f, 1.f);
    l.daylight = day;
    glm::vec3 dayFront(.98f, .94f, .84f), nightFront(.05f, .06f, .10f);
    l.front = glm::mix(nightFront, dayFront, day) + glm::vec3(.30f, .12f, -.05f) * dusk * day;
    l.back = glm::mix(glm::vec3(.05f, .06f, .10f), glm::vec3(.26f, .29f, .36f), day);
    l.ambient = glm::mix(glm::vec3(.10f, .12f, .20f), glm::vec3(.30f, .32f, .38f), day);
    // MoonLight: moonlit front/back terms (same tints as moonrules::lighting).
    float w = moon.weight * (1 - day);
    l.moonFront = glm::vec3(.32f, .38f, .5f) * w * (s.moonBrightness * 10);
    l.moonBack = glm::vec3(.07f, .085f, .12f) * w;
    l.front += l.moonFront;
    l.back += l.moonBack;
    l.front *= glm::mix(1.f, region.daylight, day);
    l.ambient *= glm::mix(1.f, region.daylight, day);
    return l;
}
