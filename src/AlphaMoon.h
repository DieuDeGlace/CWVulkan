#pragma once
// CubeVulkan's moon for the Alpha look. Cube World Alpha has no moon: its nights are black
// apart from lamps, the sky and the stars, because every sun and sky term is scaled by the
// sunlight strength S (AlphaLook.h). This is CubeVulkan's own addition, not part of the
// Alpha spec, and it can be switched off ([Render] Moon=0) to get Alpha's nights back.
//
// The moon sits opposite the sun and lights the world in the same way the sun does: a
// directional term with a weaker back light, multiplied by the light baked into each vertex
// (so caves and interiors stay dark), plus a small cool ambient floor. At night the shadow
// cascades follow the moon and take away only the moonlight.
#include <glm/glm.hpp>

namespace alpha {

// [Render] Moon, MoonBrightness, MoonShadowStrength and NightAmbient, as fractions.
struct MoonSettings {
    bool enabled = true;
    float brightness = .30f;    // scales the moonlight colour
    float shadowStrength = .6f; // share of the moonlight a moon shadow takes away
    float nightAmbient = .30f;  // scales the night ambient colour
};

// Towards the moon from a surface: always opposite the sun, so it rises at 18:00, is
// overhead at midnight and sets at 06:00.
glm::vec3 moonDirection(double timeOfDayMs);

// The moon's strength M (0..1). It grows as the sunlight S fades, eased so dusk and dawn
// blend in smoothly, and is 0 while the moon is below the horizon. 0 at noon, 1 at midnight.
float moonStrength(double timeOfDayMs);

// What the moon adds to the Alpha vertex light this frame. Every colour is already scaled by
// M and the settings; the shader multiplies each by the vertex's baked light.
struct MoonLight {
    float strength = 0;      // M
    glm::vec3 front{0};      // on faces turned towards the moon
    glm::vec3 back{0};       // on faces turned away from it
    glm::vec3 ambient{0};    // on every face: the night floor
};
MoonLight moonLight(double timeOfDayMs, const MoonSettings &settings);

// The moonlight one vertex receives (CPU form of the Alpha vertex shader's moon terms):
// `normal` is the face normal, `sunDirection` towards the sun (the moon is opposite), and
// `bakedLight` the vertex's baked light times the light where a model stands (0..1).
glm::vec3 moonOnVertex(const MoonLight &moon, glm::vec3 normal, glm::vec3 sunDirection, float bakedLight);

// The light the shadow cascades follow: the sun by day, the moon by night. Each one's
// shadows fade out towards the horizon, so the switch at sunset and sunrise happens while
// both strengths are 0 and nothing pops.
struct ShadowLight {
    glm::vec3 direction{0, 0, 1}; // towards the light
    float strength = 0;           // 0..1 of that light's own term taken away in shadow
    bool moon = false;            // the moon casts (the shader then shadows the moonlight)
};
// `sunShadow` is the full daytime strength ([Shadows] StrengthPercent).
ShadowLight shadowLight(double timeOfDayMs, float sunShadow, const MoonSettings &settings);

// How visible the moon's disc is (0..1): it fades in as the sky darkens and fades out as it
// sinks below the horizon.
float moonDiscVisibility(double timeOfDayMs);

// The moon's phase for a world day, 0..1 round an eight-day cycle: 0 and 1 new, 0.5 full.
float moonPhase(int worldDay);

} // namespace alpha
