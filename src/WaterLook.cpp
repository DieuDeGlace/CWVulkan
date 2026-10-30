#include "WaterLook.h"
#include "Config.h"
#include "Palette.h"
#include <algorithm>
#include <cctype>
namespace waterlook {
void Settings::read(const Config &config) {
    auto styleName = config.text("Water", "Style", "Omega");
    std::transform(styleName.begin(), styleName.end(), styleName.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    style = styleName == "native" ? Style::Native : Style::Omega;
    reflections = config.flag("Water", "Reflections", true);
    reflectionSteps = (int)config.number("Water", "ReflectionSteps", 24, 8, 48);
    underwaterVisibility = (float)config.number("Water", "UnderwaterVisibility", 80, 8, 256);
}
Submerged submerged(float sunlight) {
    const float brightness = .2f + .8f * std::clamp(sunlight, 0.f, 1.f);
    return {glm::vec3(.01f, .08f, .48f) * brightness, glm::vec3(.005f, .025f, .20f) * brightness, glm::vec3(.01f, .07f, .43f) * brightness,
            glm::vec3(.38f, .82f, 1.f)};
}
glm::vec3 steamShaderColour(glm::vec3 displayColour) {
    return palette::albedo(displayColour * 255.f, false) / 255.f;
}
glm::vec3 steamDisplayColour(glm::vec3 shaderColour) {
    auto colour = glm::clamp(shaderColour, 0.f, 1.f);
    const float grey = glm::dot(colour, glm::vec3(.299f, .587f, .114f)) * .35f;
    colour.g = colour.g * .65f + grey;
    colour.b = colour.b * .65f + grey;
    return colour * colour * (3.f - 2.f * colour);
}
} // namespace waterlook
